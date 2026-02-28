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

// ================================================================
//  Константный буфер — расширен полями для текстурной анимации
// ================================================================
struct PerObjectCB
{
    XMFLOAT4X4 World;
    XMFLOAT4X4 WorldViewProj;

    XMFLOAT3 LightPosW;
    float     pad0 = 0;

    XMFLOAT3 EyePosW;
    float     pad1 = 0;

    XMFLOAT4 DiffuseColor;
    XMFLOAT4 SpecColorPower;   // xyz = specular color, w = shininess

    // ?? Текстурная анимация (UV offset + tiling) ??
    float UVOffsetX = 0.0f;   // сдвиг по U (анимация прокрутки)
    float UVOffsetY = 0.0f;   // сдвиг по V
    float UVTileX = 1.0f;   // тайлинг по U
    float UVTileY = 1.0f;   // тайлинг по V

    // ?? Флаг: использовать текстуру или цвет вершины ??
    int   UseTexture = 0;      // 1 = sample texture, 0 = use vertex color
    float pad2[3] = {};

    // Новые поля для отслеживающей точки
    float  SpotIntensity = 1.0f;      // интенсивность точки (0-2)
    float  SpotSpeed = 0.0f;           // текущая скорость для отладки
    float  SpotPosX = 0.5f;            // позиция точки по X (0-1)
    float  SpotPosY = 0.5f;            // позиция точки по Y (0-1)
};

// ================================================================
//  Рендеруемый объект
// ================================================================
struct RenderItem
{
    ComPtr<ID3D12Resource> VB;
    ComPtr<ID3D12Resource> IB;
    D3D12_VERTEX_BUFFER_VIEW VBV{};
    D3D12_INDEX_BUFFER_VIEW  IBV{};
    UINT IndexCount = 0;

    // Индекс в SRV-куче (?1 = нет текстуры)
    int SrvIndex = -1;

    Material material;
};

// ================================================================
//  D3DApp
// ================================================================
class D3DApp
{
public:
    explicit D3DApp(HWND hwnd);
    ~D3DApp() { if (mCbvMappedData) mConstBuffer->Unmap(0, nullptr); }

    void Draw();
    void UpdateCB(float dt);

    // Ввод
    void OnMouseDown(WPARAM btn, int x, int y) { mCamera.OnMouseDown(btn, x, y); }
    void OnMouseUp(WPARAM btn) { mCamera.OnMouseUp(btn); }
    void OnMouseMove(WPARAM btn, int x, int y) { mCamera.OnMouseMove(btn, x, y); }
    void OnMouseWheel(int delta) { mCamera.OnMouseWheel(delta); }

private:
    // ?? Инициализация ??????????????????????????????????????????
    void InitD3D();
    void CreateRTV();
    void CreateDepthStencil();
    void BuildRootSignature();
    void BuildPSO();
    void BuildGeometry();
    void BuildConstantBuffer();
    void BuildViewportScissor();

    // Параметры для зависимости скорости от расстояния
    float mBaseAnimationSpeed = 0.05f;     // базовая скорость анимации
    float mDistanceFactor = 2.0f;           // коэффициент влияния расстояния
    float mMinSpeed = 0.02f;                 // минимальная скорость
    float mMaxSpeed = 0.5f;                   // максимальная скорость
    float mReferenceDistance = 5.0f;      // эталонное расстояние (нормальное расстояние)

    // Параметры для отслеживающей точки
    float mSpotIntensity = 1.5f;        // яркость точки
    float mSpotPosX = 0.5f;             // позиция точки X (центр текстуры)
    float mSpotPosY = 0.5f;             // позиция точки Y
    bool  mSpotPulseEnabled = true;     // пульсация точки
    bool  mSpotTrailEnabled = true;     // след от движения

    // ?? Текстуры ???????????????????????????????????????????????
    void BuildTextures();
    int  LoadTextureDDS(const std::wstring& path);   // возвращает SRV-индекс
    int  LoadTextureWIC(const std::wstring& path);   // WIC-путь (png/jpg/bmp)

    // ?? Утилиты ????????????????????????????????????????????????
    void FlushCommandQueue();
    ComPtr<ID3DBlob> CompileShader(
        const wchar_t* filename,
        const char* entry,
        const char* target);

    // ?? Константы ??????????????????????????????????????????????
    static constexpr int SwapChainBufferCount = 2;

    HWND  m_hWnd = nullptr;
    int   mClientWidth = 1280;
    int   mClientHeight = 720;

    // ?? D3D12 объекты ??????????????????????????????????????????
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

    // ?? Текстуры: общая SRV-куча для всех текстур ?????????????
    static constexpr int MaxTextures = 64;
    ComPtr<ID3D12DescriptorHeap> mSrvHeap;       // CBV/SRV/UAV heap
    UINT mSrvDescriptorSize = 0;
    int  mNextSrvIndex = 1;  // 0 зарезервирован под CBV

    // Текстурные ресурсы (чтобы не удалились раньше времени)
    std::vector<ComPtr<ID3D12Resource>> mTextures;
    std::vector<ComPtr<ID3D12Resource>> mTextureUploads;

    // ?? Константный буфер ?????????????????????????????????????
    ComPtr<ID3D12Resource> mConstBuffer;
    UINT8* mCbvMappedData = nullptr;

    // ?? Сэмплер ???????????????????????????????????????????????
    ComPtr<ID3D12DescriptorHeap> mSamplerHeap;

    // ?? Геометрия (список объектов) ???????????????????????????
    std::vector<RenderItem> mRenderItems;

    // ?? Камера ????????????????????????????????????????????????
    OrbitalCamera mCamera{ 6.0f, 0.5f, 0.4f };

    // ?? Состояние анимации UV ?????????????????????????????????
    float mTotalTime = 0.0f;

    // ?? Viewport / Scissor ????????????????????????????????????
    D3D12_VIEWPORT mViewport{};
    D3D12_RECT     mScissor{};

    // ?? Fence ?????????????????????????????????????????????????
    ComPtr<ID3D12Fence> mFence;
    UINT64              mFenceValue = 0;
};