#pragma once
#include "pch.h"

class App
{
public:
	App(HINSTANCE hInstance);
	App(const App& ref) = delete;
	App& operator=(const App& ref) = delete;
	~App();

public:
	static App* GetApp();

	HINSTANCE GethInstance()const;		// 获取程序句柄
	HWND GethMainWnd()const;			// 获取主窗口句柄
	float GetAspectRatio()const;		// 获取长宽比

public:
	virtual bool Initialize();
	virtual LRESULT MsgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	int Run();

protected:
	virtual void CreateRtvAndDsvDescriptorHeaps();	// 创建描述符堆
	virtual void OnResize();
	virtual void Update() = 0;
	virtual void Draw() = 0;

	virtual void OnMouseDown(WPARAM btnState, int x, int y) { }
	virtual void OnMouseUp(WPARAM btnState, int x, int y) { }
	virtual void OnMouseMove(WPARAM btnState, int x, int y) { }

protected:
	bool InitMainWnd();				// 初始化主窗口
	bool InitD3D();					// 初始化D3D12资源
	void CreateCommandObjects();	// 创建命令队列和命令列表
	void CreateSwapChain();			// 创建交换链

	void FlushCommandQueue();		// 等待GPU

	ID3D12Resource* CurrentBackBuffer()const;
	D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
	D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

	//void LogAdapters();				// 枚举所有适配器
	//void LogAdapterOutputs(IDXGIAdapter* adapter);
	//void LogOutputDisplayModes(IDXGIOutput* output, DXGI_FORMAT format);

protected:
	static App* mThisApp;

	HINSTANCE mhAppInst = nullptr;
	HWND mhMainWnd = nullptr;
	bool mAppPaused = false;
	bool mMinimized = false;
	bool mMaximized = false;
	bool mResizing = false;
	bool mFullscreenState = false;

	CComPtr<IDXGIFactory4> mdxgiFactory;
	CComPtr<IDXGISwapChain> mSwapChain;
	CComPtr<ID3D12Device> md3dDevice;

	CComPtr<ID3D12Fence> mFence;
	UINT64 mCurrentFence = 0;

	CComPtr<ID3D12CommandQueue> mCommandQueue;
	CComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
	CComPtr<ID3D12GraphicsCommandList> mCommandList;

	static const int SwapChainBufferCount = 2;
	int mCurrBackBuffer = 0;
	CComPtr<ID3D12Resource> mSwapChainBuffer[SwapChainBufferCount];
	CComPtr<ID3D12Resource> mDepthStencilBuffer;

	CComPtr<ID3D12DescriptorHeap> mRtvHeap;
	CComPtr<ID3D12DescriptorHeap> mDsvHeap;

	D3D12_VIEWPORT mScreenViewport;
	D3D12_RECT mScissorRect;

	UINT mRtvDescriptorSize = 0;
	UINT mDsvDescriptorSize = 0;
	UINT mCbvSrvUavDescriptorSize = 0;

	std::wstring mMainWndCaption = L"d3d12 App";
	D3D_DRIVER_TYPE md3dDriverType = D3D_DRIVER_TYPE_HARDWARE;
	DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
	int mClientWidth = 800;
	int mClientHeight = 600;
};