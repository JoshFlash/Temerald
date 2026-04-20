#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

namespace render {

using Microsoft::WRL::ComPtr;

static constexpr UINT k_framesInFlight = 2;

struct FrameContext
{
    ComPtr<ID3D12CommandAllocator> cmdAllocator;
    UINT64 fenceValue      = 0;
    UINT   backBufferIndex = 0;
};

class Device
{
public:
    void init(HWND hwnd, UINT width, UINT height);
    void shutdown();

    // Per-frame render interface
    void beginFrame(const float clearColor[4]);
    void endFrame();
    void resize(UINT width, UINT height);

    // Raw accessors for higher-level passes (tasks 5+)
    ID3D12Device*               d3dDevice()    const { return m_device.Get(); }
    ID3D12CommandQueue*         commandQueue() const { return m_commandQueue.Get(); }
    ID3D12GraphicsCommandList*  cmdList()      const { return m_cmdList.Get(); }
    IDXGISwapChain3*            swapChain()    const { return m_swapChain.Get(); }
    IDXGIFactory4*              factory()      const { return m_factory.Get(); }

    FrameContext& frame(UINT index) { return m_frames[index]; }
    UINT          frameIndex() const { return m_frameIndex; }

private:
    void createFactory();
    void createDevice();
    void createCommandQueue();
    void createSwapChain(HWND hwnd, UINT width, UINT height);
    void createFrameContexts();
    void createCommandList();
    void createRtvHeap();
    void createBackBufferRtvs();
    void waitForGpu();

    D3D12_CPU_DESCRIPTOR_HANDLE currentRtv() const;

    ComPtr<IDXGIFactory4>             m_factory;
    ComPtr<ID3D12Device>              m_device;
    ComPtr<ID3D12CommandQueue>        m_commandQueue;
    ComPtr<IDXGISwapChain3>           m_swapChain;
    ComPtr<ID3D12GraphicsCommandList> m_cmdList;
    ComPtr<ID3D12DescriptorHeap>      m_rtvHeap;
    ComPtr<ID3D12Resource>            m_backBuffers[k_framesInFlight];
    ComPtr<ID3D12Fence>               m_fence;

    HANDLE m_fenceEvent    = nullptr;
    UINT64 m_nextFenceValue = 1;
    UINT   m_rtvDescSize   = 0;

    FrameContext m_frames[k_framesInFlight];
    UINT         m_frameIndex = 0;
};

} // namespace render
