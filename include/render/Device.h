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

    ID3D12Device*        d3dDevice()    const { return m_device.Get(); }
    ID3D12CommandQueue*  commandQueue() const { return m_commandQueue.Get(); }
    IDXGISwapChain3*     swapChain()    const { return m_swapChain.Get(); }
    IDXGIFactory4*       factory()      const { return m_factory.Get(); }
    ID3D12Fence*         fence()        const { return m_fence.Get(); }

    FrameContext& frame(UINT index)   { return m_frames[index]; }
    UINT          frameIndex()  const { return m_frameIndex; }

private:
    void createFactory();
    void createDevice();
    void createCommandQueue();
    void createSwapChain(HWND hwnd, UINT width, UINT height);
    void createFrameContexts();

    ComPtr<IDXGIFactory4>      m_factory;
    ComPtr<ID3D12Device>       m_device;
    ComPtr<ID3D12CommandQueue> m_commandQueue;
    ComPtr<IDXGISwapChain3>    m_swapChain;
    ComPtr<ID3D12Fence>        m_fence;

    FrameContext m_frames[k_framesInFlight];
    UINT         m_frameIndex = 0;
};

} // namespace render
