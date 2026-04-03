#pragma once

#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>
#include <wrl/client.h>
#include <vector>
#include <string>

#include "Camera.h"
#include "ObjLoader.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")

using Microsoft::WRL::ComPtr;
using namespace DirectX;

struct PerObjectCB
{
    XMFLOAT4X4 World;
    XMFLOAT4X4 WorldViewProj;

    XMFLOAT3 LightPosW;
    float     pad0 = 0;

    XMFLOAT3 EyePosW;
    float     pad1 = 0;

    XMFLOAT4 DiffuseColor;
    XMFLOAT4 SpecColorPower;

    float UVOffsetX = 0.0f;   // сдвиг по U
    float UVOffsetY = 0.0f;   // сдвиг по V
    float UVTileX = 1.0f;   // тайлинг по U
    float UVTileY = 1.0f;   // тайлинг по V

    int   UseTexture = 0;      // 1 = sample texture, 0 = use vertex color
    float pad2[3] = {};
};

struct RenderItem
{
    ComPtr<ID3D12Resource> VB;
    ComPtr<ID3D12Resource> IB;
    D3D12_VERTEX_BUFFER_VIEW VBV{};
    D3D12_INDEX_BUFFER_VIEW  IBV{};
    UINT IndexCount = 0;

    int SrvIndex = -1;

    Material material;
};

class D3DApp
{
public:
    explicit D3DApp(HWND hwnd);
    ~D3DApp() { if (mCbvMappedData) mConstBuffer->Unmap(0, nullptr); }

    void Draw();
    void UpdateCB(float dt);

    void OnMouseDown(WPARAM btn, int x, int y) { mCamera.OnMouseDown(btn, x, y); }
    void OnMouseUp(WPARAM btn) { mCamera.OnMouseUp(btn); }
    void OnMouseMove(WPARAM btn, int x, int y) { mCamera.OnMouseMove(btn, x, y); }
    void OnMouseWheel(int delta) { mCamera.OnMouseWheel(delta); }

private:
    void InitD3D();
    void CreateRTV();
    void CreateDepthStencil();
    void BuildRootSignature();
    void BuildPSO();
    void BuildGeometry();
    void BuildConstantBuffer();
    void BuildViewportScissor();

    void BuildTextures();
    int  LoadTextureDDS(const std::wstring& path);   // возвращает SRV-индекс
    int  LoadTextureWIC(const std::wstring& path);   // WIC-путь (png/jpg/bmp)

    void FlushCommandQueue();
    ComPtr<ID3DBlob> CompileShader(
        const wchar_t* filename,
        const char* entry,
        const char* target);

    static constexpr int SwapChainBufferCount = 2;

    HWND  m_hWnd = nullptr;
    int   mClientWidth = 1280;
    int   mClientHeight = 720;

    ComPtr<ID3D12Device>              mDevice;
    ComPtr<ID3D12CommandQueue>        mCommandQueue;
    ComPtr<ID3D12CommandAllocator>    mCmdAllocator;
    ComPtr<ID3D12GraphicsCommandList> mCmdList;
    ComPtr<IDXGISwapChain3>           mSwapChain;

    ComPtr<ID3D12DescriptorHeap> mRTVHeap;
    ComPtr<ID3D12DescriptorHeap> mDSVHeap;
    ComPtr<ID3D12Resource>       mSwapChainBuffer[SwapChainBufferCount];
    ComPtr<ID3D12Resource>       mDepthStencilBuffer;

    UINT mRTVDescriptorSize = 0;
    int  mCurrBackBuffer = 0;

    ComPtr<ID3D12RootSignature>       mRootSig;
    ComPtr<ID3D12PipelineState>       mPSO;

    static constexpr int MaxTextures = 64;
    ComPtr<ID3D12DescriptorHeap> mSrvHeap;       // CBV/SRV/UAV heap
    UINT mSrvDescriptorSize = 0;
    int  mNextSrvIndex = 1;  // 0 зарезервирован под CBV

    std::vector<ComPtr<ID3D12Resource>> mTextures;
    std::vector<ComPtr<ID3D12Resource>> mTextureUploads;

    ComPtr<ID3D12Resource> mConstBuffer;
    UINT8* mCbvMappedData = nullptr;

    ComPtr<ID3D12DescriptorHeap> mSamplerHeap;

    std::vector<RenderItem> mRenderItems;

    OrbitalCamera mCamera{ 6.0f, 0.5f, 0.4f };

    float mTotalTime = 0.0f;

    D3D12_VIEWPORT mViewport{};
    D3D12_RECT     mScissor{};

    ComPtr<ID3D12Fence> mFence;
    UINT64              mFenceValue = 0;
};