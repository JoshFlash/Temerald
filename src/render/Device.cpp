#include "render/Device.h"
#include "util/log.h"
#include "util/hr.h"

#include <dxgi1_6.h>

namespace render {

// ── Public ────────────────────────────────────────────────────────────────────

void Device::init(HWND hwnd, UINT width, UINT height)
{
    createFactory();
    createDevice();
    createCommandQueue();
    createSwapChain(hwnd, width, height);
    createFrameContexts();
    createRtvHeap();
    createBackBufferRtvs();
    createCommandList();
    LOG_INFO("render::Device initialised (%ux%u)", width, height);
}

void Device::shutdown()
{
    waitForGpu();

    if (m_fenceEvent) { CloseHandle(m_fenceEvent); m_fenceEvent = nullptr; }

    for (auto& bb : m_backBuffers) bb.Reset();
    m_rtvHeap.Reset();
    m_cmdList.Reset();
    m_fence.Reset();
    for (auto& f : m_frames) f.cmdAllocator.Reset();
    m_swapChain.Reset();
    m_commandQueue.Reset();
    m_device.Reset();
    m_factory.Reset();
    LOG_INFO("render::Device shut down");
}

void Device::beginFrame(const float clearColor[4])
{
    FrameContext& ctx = m_frames[m_frameIndex];

    // Wait until the GPU has finished with this slot's previous submission.
    if (m_fence->GetCompletedValue() < ctx.fenceValue)
    {
        HR(m_fence->SetEventOnCompletion(ctx.fenceValue, m_fenceEvent));
        WaitForSingleObject(m_fenceEvent, INFINITE);
    }

    ctx.backBufferIndex = m_swapChain->GetCurrentBackBufferIndex();

    HR(ctx.cmdAllocator->Reset());
    HR(m_cmdList->Reset(ctx.cmdAllocator.Get(), nullptr));

    // PRESENT → RENDER_TARGET
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = m_backBuffers[ctx.backBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_cmdList->ResourceBarrier(1, &barrier);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = currentRtv();
    m_cmdList->OMSetRenderTargets(1, &rtv, FALSE, nullptr);
    m_cmdList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void Device::endFrame()
{
    FrameContext& ctx = m_frames[m_frameIndex];

    // RENDER_TARGET → PRESENT
    D3D12_RESOURCE_BARRIER barrier{};
    barrier.Type                   = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Transition.pResource   = m_backBuffers[ctx.backBufferIndex].Get();
    barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
    barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PRESENT;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    m_cmdList->ResourceBarrier(1, &barrier);

    HR(m_cmdList->Close());

    ID3D12CommandList* lists[] = { m_cmdList.Get() };
    m_commandQueue->ExecuteCommandLists(1, lists);

    HR(m_swapChain->Present(1, 0)); // vsync

    ctx.fenceValue = m_nextFenceValue++;
    HR(m_commandQueue->Signal(m_fence.Get(), ctx.fenceValue));

    m_frameIndex = (m_frameIndex + 1) % k_framesInFlight;
}

void Device::resize(UINT width, UINT height)
{
    if (width == 0 || height == 0) return;

    waitForGpu();

    for (auto& bb : m_backBuffers) bb.Reset();

    HR(m_swapChain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0));

    createBackBufferRtvs();
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();

    LOG_INFO("Swap chain resized to %ux%u", width, height);
}

// ── Private ───────────────────────────────────────────────────────────────────

void Device::createFactory()
{
    UINT flags = 0;

#if defined(_DEBUG)
    ComPtr<ID3D12Debug1> debug;
    if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug))))
    {
        debug->EnableDebugLayer();
        debug->SetEnableGPUBasedValidation(TRUE);
        LOG_INFO("D3D12 debug layer + GPU-based validation enabled");
        flags |= DXGI_CREATE_FACTORY_DEBUG;
    }
    else
    {
        LOG_WARN("D3D12 debug interface unavailable; debug layer not enabled");
    }
#endif

    HR(CreateDXGIFactory2(flags, IID_PPV_ARGS(&m_factory)));
}

void Device::createDevice()
{
    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(m_factory.As(&factory6)))
    {
        for (UINT i = 0;
             SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                 i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
             ++i)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { adapter.Reset(); continue; }
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0,
                    __uuidof(ID3D12Device), nullptr))) break;
            adapter.Reset();
        }
    }

    if (!adapter)
    {
        for (UINT i = 0; SUCCEEDED(m_factory->EnumAdapters1(i, &adapter)); ++i)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) { adapter.Reset(); continue; }
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0,
                    __uuidof(ID3D12Device), nullptr))) break;
            adapter.Reset();
        }
    }

    if (!adapter)
    {
        LOG_WARN("No hardware D3D12 adapter found — falling back to WARP");
        HR(m_factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)));
    }

    HR(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(&m_device)));

    DXGI_ADAPTER_DESC1 desc;
    adapter->GetDesc1(&desc);
    char name[128];
    WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, name, sizeof(name), nullptr, nullptr);
    LOG_INFO("D3D12 device created on: %s", name);

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(m_device.As(&infoQueue)))
    {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,      TRUE);
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
    }
#endif
}

void Device::createCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC desc{};
    desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
    desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
    HR(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)));
    LOG_INFO("Command queue created");
}

void Device::createSwapChain(HWND hwnd, UINT width, UINT height)
{
    DXGI_SWAP_CHAIN_DESC1 desc{};
    desc.BufferCount = k_framesInFlight;
    desc.Width       = width;
    desc.Height      = height;
    desc.Format      = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    desc.SwapEffect  = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    desc.SampleDesc  = { 1, 0 };

    ComPtr<IDXGISwapChain1> sc1;
    HR(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), hwnd, &desc,
        nullptr, nullptr, &sc1));
    HR(m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
    HR(sc1.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    LOG_INFO("Swap chain created (%ux%u, %u buffers)", width, height, k_framesInFlight);
}

void Device::createFrameContexts()
{
    HR(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));
    m_fenceEvent = CreateEventW(nullptr, FALSE, FALSE, nullptr);
    if (!m_fenceEvent) HR(HRESULT_FROM_WIN32(GetLastError()));

    for (UINT i = 0; i < k_framesInFlight; ++i)
    {
        HR(m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_frames[i].cmdAllocator)));
        m_frames[i].fenceValue      = 0;
        m_frames[i].backBufferIndex = 0;
    }
    LOG_INFO("%u frame contexts created", k_framesInFlight);
}

void Device::createCommandList()
{
    HR(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        m_frames[0].cmdAllocator.Get(), nullptr, IID_PPV_ARGS(&m_cmdList)));
    HR(m_cmdList->Close()); // starts closed; beginFrame will Reset() it
    LOG_INFO("Command list created");
}

void Device::createRtvHeap()
{
    D3D12_DESCRIPTOR_HEAP_DESC desc{};
    desc.Type           = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    desc.NumDescriptors = k_framesInFlight;
    desc.Flags          = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    HR(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvHeap)));
    m_rtvDescSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
}

void Device::createBackBufferRtvs()
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    for (UINT i = 0; i < k_framesInFlight; ++i)
    {
        HR(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&m_backBuffers[i])));
        m_device->CreateRenderTargetView(m_backBuffers[i].Get(), nullptr, handle);
        handle.ptr += m_rtvDescSize;
    }
}

void Device::waitForGpu()
{
    const UINT64 value = m_nextFenceValue++;
    HR(m_commandQueue->Signal(m_fence.Get(), value));
    HR(m_fence->SetEventOnCompletion(value, m_fenceEvent));
    WaitForSingleObject(m_fenceEvent, INFINITE);

    for (auto& f : m_frames) f.fenceValue = 0;
}

D3D12_CPU_DESCRIPTOR_HANDLE Device::currentRtv() const
{
    D3D12_CPU_DESCRIPTOR_HANDLE handle = m_rtvHeap->GetCPUDescriptorHandleForHeapStart();
    handle.ptr += static_cast<SIZE_T>(m_frames[m_frameIndex].backBufferIndex) * m_rtvDescSize;
    return handle;
}

} // namespace render
