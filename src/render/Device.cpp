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
    LOG_INFO("render::Device initialised (%ux%u)", width, height);
}

void Device::shutdown()
{
    m_fence.Reset();
    for (auto& f : m_frames) f.cmdAllocator.Reset();
    m_swapChain.Reset();
    m_commandQueue.Reset();
    m_device.Reset();
    m_factory.Reset();
    LOG_INFO("render::Device shut down");
}

// ── Private ───────────────────────────────────────────────────────────────────

void Device::createFactory()
{
    UINT flags = 0;

#if defined(_DEBUG)
    // Enable the debug layer before creating the factory or device so that
    // the debug layer can track all D3D12 objects from the start.
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

    // Prefer the highest-performance hardware adapter via IDXGIFactory6.
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
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapter.Reset();
                continue;
            }
            if (SUCCEEDED(D3D12CreateDevice(
                    adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
                break;
            adapter.Reset();
        }
    }

    // Fall back to basic enumeration if IDXGIFactory6 is unavailable.
    if (!adapter)
    {
        for (UINT i = 0; SUCCEEDED(m_factory->EnumAdapters1(i, &adapter)); ++i)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);
            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                adapter.Reset();
                continue;
            }
            if (SUCCEEDED(D3D12CreateDevice(
                    adapter.Get(), D3D_FEATURE_LEVEL_12_0, __uuidof(ID3D12Device), nullptr)))
                break;
            adapter.Reset();
        }
    }

    // Last resort: WARP software renderer.
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
    // Break on D3D12 errors and warnings in debug builds.
    ComPtr<ID3D12InfoQueue> infoQueue;
    if (SUCCEEDED(m_device.As(&infoQueue)))
    {
        infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR,   TRUE);
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
    HR(m_factory->CreateSwapChainForHwnd(m_commandQueue.Get(), hwnd, &desc, nullptr, nullptr, &sc1));

    // Suppress the automatic Alt+Enter fullscreen toggle; we'll handle it ourselves later.
    HR(m_factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

    HR(sc1.As(&m_swapChain));
    m_frameIndex = m_swapChain->GetCurrentBackBufferIndex();
    LOG_INFO("Swap chain created (%ux%u, %u buffers)", width, height, k_framesInFlight);
}

void Device::createFrameContexts()
{
    // Shared fence with one value per frame slot; each slot stores the value
    // it was last signaled with so BeginFrame can wait for the right slot.
    HR(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence)));

    for (UINT i = 0; i < k_framesInFlight; ++i)
    {
        HR(m_device->CreateCommandAllocator(
            D3D12_COMMAND_LIST_TYPE_DIRECT,
            IID_PPV_ARGS(&m_frames[i].cmdAllocator)));

        m_frames[i].fenceValue      = 0;
        m_frames[i].backBufferIndex = 0;
    }

    LOG_INFO("%u frame contexts created", k_framesInFlight);
}

} // namespace render
