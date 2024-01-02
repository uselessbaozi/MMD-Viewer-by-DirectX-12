// FinalWorkforGraphics.cpp : 定义应用程序的入口点。
//

#include "pch.h"
#include "FinalWorkforGraphics.h"
#include "D3DApp.h"
#include "d3dUtil.h"
#include "PeopleModel.h"

class Demo :public D3DApp
{
public:
	Demo(HINSTANCE hInstance);
	~Demo();

	virtual bool Initialize()override;

private:
	virtual void OnResize()override;
	virtual void Update()override;
	virtual void Draw()override;

	void UpdateCamera();
	void UpdateObjectCBs();
	void UpdateMainPassCB();
	void UpdateMaterialCB();
	void UpdateSkeletonVB();
	void UpdateReflectPassCB();

	virtual void OnMouseDown(WPARAM btnState, int x, int y)override;
	virtual void OnMouseUp(WPARAM btnState, int x, int y)override;
	virtual void OnMouseMove(WPARAM btnState, int x, int y)override;


	void BuildDescriptorHeaps();		// 创建描述堆符
	//void BuildConstantBuffers();		// 创建常量缓冲区
	void LoadTexture();
	void BuildRootSignature();			// 创建根签名
	void BuildShadersAndInputLayout();	// 创建着色器和输入布局，后者与着色器关联的输入有关
	void BuildBoxGeometry();			// 创建所需渲染物体的资源
	void BuildLandGeometry();
	void BuildWall();
	void BuildPeople();
	void BuildMaterial();
	void BuildPSO();					// 创建图形流水线，当这里发生错误时，表示之前创建的资源存在问题导致关联出现问题，可以通过输出的调试信息逐步查找错误

	void BuildFrameResources();
	void BuildRenderItems();
	void DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<d3dUtil::RenderItem*>& ritems);

	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();

private:
	std::vector<std::unique_ptr<d3dUtil::FrameResource>> mFrameResources;
	d3dUtil::FrameResource* mCurrFrameResource = nullptr;
	int mCurrFrameResourceIndex = 0;

	UINT mCbvSrvDescriptorSize = 0;

	CComPtr<ID3D12RootSignature> mRootSignature = nullptr;
	//CComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;

	CComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

	std::unordered_map<std::string, std::unique_ptr<d3dUtil::MeshGeometry>> mGeometries;
	std::unordered_map<std::string, CComPtr<ID3DBlob>> mShaders;
	std::unordered_map<std::string, CComPtr<ID3D12PipelineState>> mPSOs;

	std::unordered_map<std::string, std::unique_ptr<d3dUtil::Material>> mMaterials;
	std::unordered_map<std::string, std::unique_ptr<d3dUtil::Texture>> mTexture;

	std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
	std::vector<D3D12_INPUT_ELEMENT_DESC> mPeopleInputLayout;

	// List of all the render items.
	std::vector<std::unique_ptr<d3dUtil::RenderItem>> mAllRitems;

	// Render items divided by PSO.
	std::vector<d3dUtil::RenderItem*> mOpaqueRitems;
	std::vector<d3dUtil::RenderItem*> mAlphaTestRItems;
	std::vector<d3dUtil::RenderItem*> mTransRItems;
	std::vector<d3dUtil::RenderItem*> mMirror;
	std::vector<d3dUtil::RenderItem*> mReflect;
	std::vector<d3dUtil::RenderItem*> mShadow;
	std::vector<d3dUtil::RenderItem*> mPeopleRItems;

	d3dUtil::PassConstants mMainPassCB;
	d3dUtil::PassConstants mReflectPassCB;

	//UINT mPassCbvOffset = 0;

	DirectX::XMFLOAT3 mEyePos = { 0.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT4X4 mView = MathHelper::Identity4x4();
	DirectX::XMFLOAT4X4 mProj = MathHelper::Identity4x4();

	float mTheta = 1.5f * DirectX::XM_PI;
	float mPhi = 0.2f * DirectX::XM_PI;
	float mRadius = 15.0f;

	float mSunTheta = 1.25f * DirectX::XM_PI;
	float mSunPhi = DirectX::XM_PIDIV4;

	POINT mLastMousePos;

	d3dModel::PeopleModel mPeople;
	std::vector<d3dUtil::Vector2> mPeopleSkeleton;
};

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR lpCmdLine,
	_In_ int nCmdShow)
{
#if defined(DEBUG) | defined(_DEBUG)
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

	try
	{
		Demo theApp(hInstance);
		if (!theApp.Initialize())
			return 0;

		return theApp.Run();
	}
	catch (DxException& e)
	{
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		return 0;
	}
}

Demo::Demo(HINSTANCE hInstance) :D3DApp(hInstance)
{
}

Demo::~Demo()
{
}

bool Demo::Initialize()
{
	if (!D3DApp::Initialize())
		return false;

	ThrowIfFailed(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

	mCbvSrvDescriptorSize = md3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	mPeople.Initialize(L"Model\\【芙宁娜】.pmx");
	mPeople.LoadMotion(L"Model\\动作 Sour.vmd");
	BuildRootSignature();
	BuildShadersAndInputLayout();
	LoadTexture();
	BuildDescriptorHeaps();
	BuildMaterial();
	BuildBoxGeometry();
	BuildLandGeometry();
	BuildWall();
	BuildPeople();
	BuildRenderItems();
	BuildFrameResources();
	//BuildConstantBuffers();
	BuildPSO();

	// 关闭命令列表并加载到命令队列
	ThrowIfFailed(mCommandList->Close());
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	FlushCommandQueue();

	return true;
}

void Demo::OnResize()
{
	D3DApp::OnResize();

	// 通过视窗大小调整视角坐标转换矩阵
	DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovLH(0.25f * DirectX::XM_PI, AspectRatio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&mProj, P);
}

void Demo::Update()
{
	UpdateCamera();

	mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % gNumFrameResources;
	mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

	// Has the GPU finished processing the commands of the current frame resource?
	// If not, wait until the GPU has completed commands up to this fence point.
	if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence)
	{
		HANDLE eventHandle = CreateEventEx(nullptr, false, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
		WaitForSingleObject(eventHandle, INFINITE);
		CloseHandle(eventHandle);
	}

	UpdateObjectCBs();
	UpdateMainPassCB();
	UpdateReflectPassCB();
	UpdateMaterialCB();
	UpdateSkeletonVB();
}

void Demo::Draw()
{
	auto cmdListAlloc(mCurrFrameResource->CmdListAlloc);

	// Reuse the memory associated with command recording.
	// We can only reset when the associated command lists have finished execution on the GPU.
	ThrowIfFailed(cmdListAlloc->Reset());

	ThrowIfFailed(mCommandList->Reset(
		cmdListAlloc.Get(),
		mPSOs["opaque"].Get()
	));

	mCommandList->RSSetViewports(1, &mScreenViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// Clear the back buffer and depth buffer.
	mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);

	mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

	/*ID3D12DescriptorHeap* descriptorHeaps[] = { mCbvHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);*/
	ID3D12DescriptorHeap* descriptorHeaps[] = { mSrvDescriptorHeap.Get() };
	mCommandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

	/*int passCbvIndex(mPassCbvOffset + mCurrFrameResourceIndex);
	auto passCbvHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(mCbvHeap->GetGPUDescriptorHandleForHeapStart());
	passCbvHandle.Offset(passCbvIndex, mCbvSrvUavDescriptorSize);
	mCommandList->SetGraphicsRootDescriptorTable(1, passCbvHandle);*/

	// Bind per-pass constant buffer.  We only need to do this once per-pass.
	auto passCB = mCurrFrameResource->PassCB->GetResource();
	mCommandList->SetGraphicsRootConstantBufferView(3, passCB->GetGPUVirtualAddress());
	DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

	mCommandList->SetPipelineState(mPSOs["alphaTest"].Get());
	DrawRenderItems(mCommandList.Get(), mAlphaTestRItems);

	mCommandList->OMSetStencilRef(1);
	mCommandList->SetPipelineState(mPSOs["markStencilMirror"].Get());
	DrawRenderItems(mCommandList.Get(), mMirror);

	auto tempAddr(passCB->GetGPUVirtualAddress());
	tempAddr += d3dUtilStatic::CalcConstantBufferByteSize(sizeof(d3dUtil::PassConstants));
	mCommandList->SetGraphicsRootConstantBufferView(3, tempAddr);
	mCommandList->SetPipelineState(mPSOs["drawStencilReflections"].Get());
	DrawRenderItems(mCommandList.Get(), mReflect);

	mCommandList->OMSetStencilRef(0);
	mCommandList->SetPipelineState(mPSOs["transparency"].Get());
	DrawRenderItems(mCommandList.Get(), mTransRItems);

	mCommandList->SetPipelineState(mPSOs["shadow"].Get());
	DrawRenderItems(mCommandList.Get(), mShadow);

	mCommandList->SetPipelineState(mPSOs["people"].Get());
	DrawRenderItems(mCommandList.Get(), mPeopleRItems);

	// Indicate a state transition on the resource usage.
	mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
		D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));

	// Done recording commands.
	ThrowIfFailed(mCommandList->Close());

	// Add the command list to the queue for execution.
	ID3D12CommandList* cmdsLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(cmdsLists), cmdsLists);

	// swap the back and front buffers
	ThrowIfFailed(mSwapChain->Present(0, 0));
	mCurrBackBuffer = (mCurrBackBuffer + 1) % SwapChainBufferCount;

	mCurrFrameResource->Fence = ++mCurrentFence;

	mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

void Demo::UpdateCamera()
{
	// Convert Spherical to Cartesian coordinates.
	mEyePos.x = mRadius * sinf(mPhi) * cosf(mTheta);
	mEyePos.z = mRadius * sinf(mPhi) * sinf(mTheta);
	mEyePos.y = mRadius * cosf(mPhi);

	// Build the view matrix.
	DirectX::XMVECTOR pos = DirectX::XMVectorSet(mEyePos.x, mEyePos.y, mEyePos.z, 1.0f);
	DirectX::XMVECTOR target = DirectX::XMVectorZero();
	DirectX::XMVECTOR up = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

	DirectX::XMMATRIX view = DirectX::XMMatrixLookAtLH(pos, target, up);
	DirectX::XMStoreFloat4x4(&mView, view);
}

void Demo::UpdateObjectCBs()
{
	auto currObjectCB = mCurrFrameResource->ObjectCB.get();
	for (auto& e : mAllRitems)
	{
		// Only update the cbuffer data if the constants have changed.  
		// This needs to be tracked per frame resource.
		if (e->mNumFramesDirty > 0)
		{
			DirectX::XMMATRIX world = DirectX::XMLoadFloat4x4(&e->mWorld),
				tex = DirectX::XMLoadFloat4x4(&e->mTexTransform);

			d3dUtil::ObjectConstants objConstants;
			XMStoreFloat4x4(&objConstants.worldView, XMMatrixTranspose(world));
			DirectX::XMStoreFloat4x4(&objConstants.TexTransform, DirectX::XMMatrixTranspose(tex));

			currObjectCB->CopyData(e->mObjCBIndex, objConstants);

			// Next FrameResource need to be updated too.
			e->mNumFramesDirty--;
		}
	}
}

void Demo::UpdateMainPassCB()
{
	DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4(&mView);
	DirectX::XMMATRIX proj = DirectX::XMLoadFloat4x4(&mProj);

	DirectX::XMMATRIX viewProj = DirectX::XMMatrixMultiply(view, proj);
	DirectX::XMMATRIX invView = DirectX::XMMatrixInverse(&XMMatrixDeterminant(view), view);
	DirectX::XMMATRIX invProj = DirectX::XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	DirectX::XMMATRIX invViewProj = DirectX::XMMatrixInverse(&XMMatrixDeterminant(viewProj), viewProj);

	DirectX::XMStoreFloat4x4(&mMainPassCB.View, DirectX::XMMatrixTranspose(view));
	DirectX::XMStoreFloat4x4(&mMainPassCB.InvView, DirectX::XMMatrixTranspose(invView));
	DirectX::XMStoreFloat4x4(&mMainPassCB.Proj, DirectX::XMMatrixTranspose(proj));
	DirectX::XMStoreFloat4x4(&mMainPassCB.InvProj, DirectX::XMMatrixTranspose(invProj));
	DirectX::XMStoreFloat4x4(&mMainPassCB.ViewProj, DirectX::XMMatrixTranspose(viewProj));
	DirectX::XMStoreFloat4x4(&mMainPassCB.InvViewProj, DirectX::XMMatrixTranspose(invViewProj));
	mMainPassCB.EyePosW = mEyePos;
	mMainPassCB.RenderTargetSize = DirectX::XMFLOAT2((float)mClientWidth, (float)mClientHeight);
	mMainPassCB.InvRenderTargetSize = DirectX::XMFLOAT2(1.0f / mClientWidth, 1.0f / mClientHeight);
	mMainPassCB.NearZ = 1.0f;
	mMainPassCB.FarZ = 1000.0f;
	mMainPassCB.AmbientLight = DirectX::XMFLOAT4{ 0.25f, 0.25f, 0.35f, 1.0f };

	mMainPassCB.Lights[0].Direction = { 0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[0].Strength = { 0.6f, 0.6f, 0.6f };
	mMainPassCB.Lights[1].Direction = { -0.57735f, -0.57735f, 0.57735f };
	mMainPassCB.Lights[1].Strength = { 0.3f, 0.3f, 0.3f };
	mMainPassCB.Lights[2].Direction = { 0.0f, -0.707f, -0.707f };
	mMainPassCB.Lights[2].Strength = { 0.15f, 0.15f, 0.15f };

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(0, mMainPassCB);
}

void Demo::UpdateMaterialCB()
{
	auto currMaterialCB(mCurrFrameResource->MaterialCB.get());
	for (auto& iter : mMaterials)
	{
		auto mat(iter.second.get());
		if (mat->numFrameDirty > 0)
		{
			DirectX::XMMATRIX matTransform = DirectX::XMLoadFloat4x4(&mat->matTransform);

			d3dUtil::MaterialConstants matConstants;
			matConstants.diffuseAlbedo = mat->diffuseAlbedo;
			matConstants.fresnelR0 = mat->fresnelR0;
			matConstants.roughness = mat->roughness;
			XMStoreFloat4x4(&matConstants.matTransform, XMMatrixTranspose(matTransform));

			currMaterialCB->CopyData(mat->matCBIndex, matConstants);

			mat->numFrameDirty--;
		}
	}

}

void Demo::UpdateSkeletonVB()
{
	auto currSkeletonVB(mCurrFrameResource->SkeletonVB.get());

	mPeople.Update(mPeopleSkeleton);
	for (auto i = 0u; i < mPeopleSkeleton.size(); ++i)
	{
		currSkeletonVB->CopyData(i, mPeopleSkeleton[i]);
	}

	mGeometries["people"]->VertexBufferGPU = currSkeletonVB->GetResource();
}

void Demo::UpdateReflectPassCB()
{
	mReflectPassCB = mMainPassCB;

	DirectX::XMVECTOR mirrorPlane = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	DirectX::XMMATRIX R = DirectX::XMMatrixReflect(mirrorPlane);

	// Reflect the lighting.
	for (int i = 0; i < 3; ++i)
	{
		DirectX::XMVECTOR lightDir = DirectX::XMLoadFloat3(&mMainPassCB.Lights[i].Direction);
		DirectX::XMVECTOR reflectedLightDir = DirectX::XMVector3TransformNormal(lightDir, R);
		DirectX::XMStoreFloat3(&mReflectPassCB.Lights[i].Direction, reflectedLightDir);
	}

	auto currPassCB = mCurrFrameResource->PassCB.get();
	currPassCB->CopyData(1, mReflectPassCB);
}

void Demo::OnMouseDown(WPARAM btnState, int x, int y)
{
	mLastMousePos.x = x;
	mLastMousePos.y = y;

	SetCapture(mhMainWnd);
}

void Demo::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();
}

void Demo::OnMouseMove(WPARAM btnState, int x, int y)
{
	if ((btnState & MK_LBUTTON) != 0)
	{
		// Make each pixel correspond to a quarter of a degree.
		float dx = DirectX::XMConvertToRadians(0.25f * static_cast<float>(x - mLastMousePos.x));
		float dy = DirectX::XMConvertToRadians(0.25f * static_cast<float>(y - mLastMousePos.y));

		// Update angles based on input to orbit camera around box.（注：将增加改为减小）
		mTheta -= dx;
		mPhi -= dy;

		// Restrict the angle mPhi.
		mPhi = MathHelper::Clamp(mPhi, 0.1f, DirectX::XM_PI - 0.1f);
	}
	else if ((btnState & MK_RBUTTON) != 0)
	{
		// Make each pixel correspond to 0.005 unit in the scene.
		float dx = 0.005f * static_cast<float>(x - mLastMousePos.x);
		float dy = 0.005f * static_cast<float>(y - mLastMousePos.y);

		// Update the camera radius based on input.
		mRadius += dx - dy;

		// Restrict the radius.
		mRadius = MathHelper::Clamp(mRadius, 3.0f, 15.0f);
	}

	mLastMousePos.x = x;
	mLastMousePos.y = y;
}

void Demo::BuildDescriptorHeaps()
{
	D3D12_DESCRIPTOR_HEAP_DESC srvHeadDesc{};
	srvHeadDesc.NumDescriptors = mTexture.size();
	srvHeadDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeadDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	ThrowIfFailed(md3dDevice->CreateDescriptorHeap(
		&srvHeadDesc,
		IID_PPV_ARGS(&mSrvDescriptorHeap)
	));

	CD3DX12_CPU_DESCRIPTOR_HANDLE hDescriptor(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MostDetailedMip = 0;
	srvDesc.Texture2D.MipLevels = -1;

	int texOffset(0);
	for (auto& iter : mTexture)
	{
		iter.second->offset = texOffset;
		srvDesc.Format = iter.second->resourse->GetDesc().Format;
		md3dDevice->CreateShaderResourceView(iter.second->resourse.Get(), &srvDesc, hDescriptor);
		hDescriptor.Offset(1, mCbvSrvDescriptorSize);
		texOffset += 1;
	}
}

void Demo::LoadTexture()
{
	auto woodCrate(d3dUtilStatic::LoadDDSTexture(
		md3dDevice.Get(),
		mCommandList.Get(),
		"woodCrate",
		L"Texture\\WoodCrate01.dds"
	));
	mTexture[woodCrate->name] = std::move(woodCrate);


	auto wireFence(d3dUtilStatic::LoadDDSTexture(
		md3dDevice.Get(),
		mCommandList.Get(),
		"wireFence",
		L"Texture\\WireFence.dds"
	));
	mTexture[wireFence->name] = std::move(wireFence);

	auto face(d3dUtilStatic::LoadDDSTexture(
		md3dDevice.Get(),
		mCommandList.Get(),
		"tex\\颜.png",
		L"Texture\\颜.dds"
	));
	mTexture[face->name] = std::move(face);

	auto body(d3dUtilStatic::LoadDDSTexture(
		md3dDevice.Get(),
		mCommandList.Get(),
		"tex\\体.png",
		L"Texture\\体.dds"
	));
	mTexture[body->name] = std::move(body);

	auto hair(d3dUtilStatic::LoadDDSTexture(
		md3dDevice.Get(),
		mCommandList.Get(),
		"tex\\髮.png",
		L"Texture\\髮.dds"
	));
	mTexture[hair->name] = std::move(hair);

	auto hair2(d3dUtilStatic::LoadDDSTexture(
		md3dDevice.Get(),
		mCommandList.Get(),
		"tex\\髮2.png",
		L"Texture\\髮2.dds"
	));
	mTexture[hair2->name] = std::move(hair2);

	auto spa_h(d3dUtilStatic::LoadDDSTexture(
		md3dDevice.Get(),
		mCommandList.Get(),
		"tex\\spa_h.png",
		L"Texture\\spa_h.dds"
	));
	mTexture[spa_h->name] = std::move(spa_h);
}

void Demo::BuildRootSignature()
{
	/*CD3DX12_DESCRIPTOR_RANGE cbvTable0;
	cbvTable0.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);

	CD3DX12_DESCRIPTOR_RANGE cbvTable1;
	cbvTable1.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);*/

	CD3DX12_DESCRIPTOR_RANGE texTable;
	texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	// Root parameter can be a table, root descriptor or root constants.
	CD3DX12_ROOT_PARAMETER slotRootParameter[4];

	// Create root CBVs.
	slotRootParameter[1].InitAsConstantBufferView(0);
	slotRootParameter[2].InitAsConstantBufferView(1);
	slotRootParameter[3].InitAsConstantBufferView(2);
	slotRootParameter[0].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

	auto staticSamplers = GetStaticSamplers();

	// A root signature is an array of root parameters.
	CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(4, slotRootParameter,
		(UINT)staticSamplers.size(), staticSamplers.data(),
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

	// create a root signature with a single slot which points to a descriptor range consisting of a single constant buffer
	CComPtr<ID3DBlob> serializedRootSig = nullptr;
	CComPtr<ID3DBlob> errorBlob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
		serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());

	if (errorBlob != nullptr)
	{
		::OutputDebugStringA((char*)errorBlob->GetBufferPointer());
	}
	ThrowIfFailed(hr);

	ThrowIfFailed(md3dDevice->CreateRootSignature(
		0,
		serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(),
		IID_PPV_ARGS(mRootSignature.GetAddressOf())));
}

void Demo::BuildShadersAndInputLayout()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
		"FOG", "1",
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const D3D_SHADER_MACRO defines[] =
	{
		"FOG", "1",
		NULL, NULL
	};

	mShaders["opaquePS"] = d3dUtilStatic::CompileShader(L"Shader\\PixelShader.hlsl", defines, "PS", "ps_5_1");
	mShaders["standardVS"] = d3dUtilStatic::CompileShader(L"Shader\\color.hlsl", nullptr, "VS", "vs_5_1");
	mShaders["alphaPS"] = d3dUtilStatic::CompileShader(L"Shader\\PixelShader.hlsl", alphaTestDefines, "PS", "ps_5_1");
	mShaders["PeopleVS"] = d3dUtilStatic::CompileShader(L"Shader\\VertexShader.hlsl", defines, "main", "vs_5_1");

	mPeopleInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "WEIGHTS", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BONEINDICES", 0, DXGI_FORMAT_R32G32B32A32_UINT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
	mInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};
}

void Demo::BuildBoxGeometry()
{
	auto box1Data(d3dUtilStatic::CreateBox(1.0, 1.0, 1.0, 3));
	auto box2Data(d3dUtilStatic::CreateBox(1.5, 0.5, 1.0, 3));

	UINT box1VertexOffset(0),
		box2VertexOffset(box1Data.vertex.size()),
		box1IndexOffset(0),
		box2IndexOffset(box1Data.index.size());

	d3dUtil::SubmeshGeometry box1;
	box1.IndexCount = box1Data.index.size();
	box1.StartIndexLocation = box1IndexOffset;
	box1.BaseVertexLocation = box1VertexOffset;
	d3dUtil::SubmeshGeometry box2;
	box2.IndexCount = box2Data.index.size();
	box2.StartIndexLocation = box2IndexOffset;
	box2.BaseVertexLocation = box2VertexOffset;

	auto totalVertexCount =
		box1Data.vertex.size() +
		box2Data.vertex.size();

	std::vector<d3dUtil::Vector1> vertices(totalVertexCount);

	UINT k(0);
	for (auto i = 0u; i < box1Data.vertex.size(); ++i, ++k)
	{
		vertices[k].pos = box1Data.vertex[i].pos;
		vertices[k].normal = box1Data.vertex[i].normal;
		vertices[k].texC = box1Data.vertex[i].texC;
	}
	for (auto i = 0u; i < box2Data.vertex.size(); ++i, ++k)
	{
		vertices[k].pos = box2Data.vertex[i].pos;
		vertices[k].normal = box2Data.vertex[i].normal;
		vertices[k].texC = box2Data.vertex[i].texC;
	}

	std::vector<uint16_t> indices;
	indices.insert(indices.end(), box1Data.index.begin(), box1Data.index.end());
	indices.insert(indices.end(), box2Data.index.begin(), box2Data.index.end());

	const UINT vbByteSize(vertices.size() * sizeof(d3dUtil::Vector1)),
		ibByteSize(indices.size() * sizeof(uint16_t));

	auto tempGeo(std::make_unique<d3dUtil::MeshGeometry>());
	tempGeo->Name = "boxesGeo";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &tempGeo->VertexBufferCPU));
	CopyMemory(tempGeo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &tempGeo->IndexBufferCPU));
	CopyMemory(tempGeo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	tempGeo->VertexBufferGPU = d3dUtilStatic::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		vertices.data(),
		vbByteSize,
		tempGeo->VertexBufferUploader
	);
	tempGeo->IndexBufferGPU = d3dUtilStatic::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		indices.data(),
		ibByteSize,
		tempGeo->IndexBufferUploader
	);

	tempGeo->VertexByteStride = sizeof(d3dUtil::Vector1);
	tempGeo->VertexBufferByteSize = vbByteSize;
	tempGeo->IndexBufferByteSize = ibByteSize;
	tempGeo->IndexFormat = DXGI_FORMAT_R16_UINT;

	tempGeo->DrawArgs["box1"] = box1;
	tempGeo->DrawArgs["box2"] = box2;

	mGeometries[tempGeo->Name] = std::move(tempGeo);
}

void Demo::BuildLandGeometry()
{
	auto landData(d3dUtilStatic::CreateGrid(160.0, 160.0, 50, 50));

	d3dUtil::SubmeshGeometry land;
	land.IndexCount = landData.index.size();
	land.StartIndexLocation = 0;
	land.BaseVertexLocation = 0;

	std::vector<d3dUtil::Vector1> vertices(landData.vertex.size());
	for (auto i = 0u; i < landData.vertex.size(); ++i)
	{
		vertices[i].pos = landData.vertex[i].pos;
		vertices[i].pos.y = 0.3f * (landData.vertex[i].pos.z * sinf(0.1f * landData.vertex[i].pos.x) + landData.vertex[i].pos.x * cosf(0.1f * landData.vertex[i].pos.z));

		vertices[i].normal = MathHelper::GetHillsNormal(vertices[i].pos.x, vertices[i].pos.z);
		vertices[i].texC = landData.vertex[i].texC;
	}

	std::vector<uint16_t> indices;
	indices.insert(indices.end(), landData.index.begin(), landData.index.end());

	const UINT vbByteSize(vertices.size() * sizeof(d3dUtil::Vector1)),
		ibByteSize(indices.size() * sizeof(uint16_t));

	auto geo(std::make_unique<d3dUtil::MeshGeometry>());
	geo->Name = "land";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);
	geo->VertexBufferGPU = d3dUtilStatic::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		vertices.data(),
		vbByteSize,
		geo->VertexBufferUploader
	);
	geo->IndexBufferGPU = d3dUtilStatic::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		indices.data(),
		ibByteSize,
		geo->IndexBufferUploader
	);

	geo->VertexByteStride = sizeof(d3dUtil::Vector1);
	geo->VertexBufferByteSize = vbByteSize;
	geo->IndexBufferByteSize = ibByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;

	geo->DrawArgs["land"] = land;

	mGeometries[geo->Name] = std::move(geo);
}

void Demo::BuildWall()
{
	using namespace d3dUtil;
	std::array<Vector1, 16> vertices = {
		Vector1(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 0
		Vector1(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vector1(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
		Vector1(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),
		Vector1(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f), // 4
		Vector1(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vector1(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
		Vector1(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),
		Vector1(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f,0.0f, 1.0f), // 8
		Vector1(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vector1(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
		Vector1(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),
		Vector1(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f), // 12
		Vector1(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
		Vector1(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
		Vector1(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)
	};
	std::array<uint16_t, 24> indices{
		0,1,2,
		0,2,3,

		4,5,6,
		4,6,7,

		8,9,10,
		8,10,11,

		12,13,14,
		12,14,15
	};

	d3dUtil::SubmeshGeometry mirror, wall;
	mirror.IndexCount = 6;
	mirror.StartIndexLocation = 18;
	mirror.BaseVertexLocation = 0;
	wall.IndexCount = 18;
	wall.StartIndexLocation = 0;
	wall.BaseVertexLocation = 0;

	const UINT vbByteSize(vertices.size() * sizeof(Vector1)),
		ibByteSize(indices.size() * sizeof(uint16_t));

	auto geo(std::make_unique<d3dUtil::MeshGeometry>());
	geo->Name = "wall";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
	CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);
	
	ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->IndexBufferCPU));
	CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

	geo->VertexBufferGPU = d3dUtilStatic::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		vertices.data(),
		vbByteSize,
		geo->VertexBufferUploader
	);
	geo->IndexBufferGPU = d3dUtilStatic::CreateDefaultBuffer(
		md3dDevice.Get(),
		mCommandList.Get(),
		indices.data(),
		ibByteSize,
		geo->IndexBufferUploader
	);

	geo->VertexBufferByteSize = vbByteSize;
	geo->VertexByteStride = sizeof(Vector1);
	geo->IndexBufferByteSize = ibByteSize;
	geo->IndexFormat = DXGI_FORMAT_R16_UINT;

	geo->DrawArgs["wall"] = wall;
	geo->DrawArgs["mirror"] = mirror;

	mGeometries[geo->Name] = std::move(geo);
}

void Demo::BuildPeople()
{
	auto sizeSkeleton = mPeople.Load(mGeometries, mMaterials, mTexture, md3dDevice.Get(), mCommandList.Get());
	mPeopleSkeleton.resize(sizeSkeleton);
}

void Demo::BuildMaterial()
{
	auto grass(std::make_unique<d3dUtil::Material>());
	grass->name = "grass";
	grass->matCBIndex = 0;
	grass->diffuseSrvHeapIndex = mTexture["wireFence"]->offset;
	grass->fresnelR0 = { 0.01f, 0.01f, 0.01f };
	grass->roughness = 0.125;

	auto water(std::make_unique<d3dUtil::Material>());
	water->name = "water";
	water->matCBIndex = 1;
	water->diffuseSrvHeapIndex = mTexture["woodCrate"]->offset;
	water->diffuseAlbedo = { 1.0,1.0,1.0,0.4 };
	water->fresnelR0 = { 0.1f, 0.1f, 0.1f };
	water->roughness = 0.0;

	auto stone0(std::make_unique<d3dUtil::Material>());
	stone0->name = "stone0";
	stone0->matCBIndex = 2;
	stone0->diffuseSrvHeapIndex = mTexture["woodCrate"]->offset;
	stone0->diffuseAlbedo = DirectX::XMFLOAT4(DirectX::Colors::LightSteelBlue);
	stone0->fresnelR0 = { 0.01f, 0.01f, 0.01f };
	stone0->roughness = 0.3;

	auto mirror(std::make_unique<d3dUtil::Material>());
	mirror->name = "mirror";
	mirror->matCBIndex = 3;
	mirror->diffuseSrvHeapIndex = mTexture["woodCrate"]->offset;
	mirror->diffuseAlbedo = { 1.0,1.0,1.0,0.3 };
	mirror->fresnelR0 = { 0.1f, 0.1f, 0.1f };
	mirror->roughness = 0.3;

	auto shadow(std::make_unique<d3dUtil::Material>());
	shadow->name = "shadow";
	shadow->matCBIndex = 4;
	shadow->diffuseSrvHeapIndex = mTexture["woodCrate"]->offset;
	shadow->diffuseAlbedo = DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	shadow->fresnelR0 = DirectX::XMFLOAT3(0.001f, 0.001f, 0.001f);
	shadow->roughness = 0.0f;

	mMaterials["grass"] = std::move(grass);
	mMaterials["water"] = std::move(water);
	mMaterials[stone0->name] = std::move(stone0);
	mMaterials[mirror->name] = std::move(mirror);
	mMaterials[shadow->name] = std::move(shadow);
}

void Demo::BuildPSO()
{
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaquePsoDesc;

	//
	// PSO for opaque objects.
	//
	ZeroMemory(&opaquePsoDesc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaquePsoDesc.InputLayout = { mInputLayout.data(), (UINT)mInputLayout.size() };
	opaquePsoDesc.pRootSignature = mRootSignature.Get();
	opaquePsoDesc.VS =
	{
		reinterpret_cast<BYTE*>(mShaders["standardVS"]->GetBufferPointer()),
		mShaders["standardVS"]->GetBufferSize()
	};
	opaquePsoDesc.PS =
	{
		reinterpret_cast<BYTE*>(mShaders["opaquePS"]->GetBufferPointer()),
		mShaders["opaquePS"]->GetBufferSize()
	};
	opaquePsoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaquePsoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	opaquePsoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaquePsoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaquePsoDesc.SampleMask = UINT_MAX;
	opaquePsoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaquePsoDesc.NumRenderTargets = 1;
	opaquePsoDesc.RTVFormats[0] = mBackBufferFormat;
	opaquePsoDesc.SampleDesc.Count = m4xMsaaState ? 4 : 1;
	opaquePsoDesc.SampleDesc.Quality = m4xMsaaState ? (m4xMsaaQuality - 1) : 0;
	opaquePsoDesc.DSVFormat = mDepthStencilFormat;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&opaquePsoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

	auto transparencyPsoDesc(opaquePsoDesc);

	D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
	transparencyBlendDesc.BlendEnable = true;
	transparencyBlendDesc.LogicOpEnable = false;
	transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
	transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
	transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	transparencyPsoDesc.BlendState.RenderTarget[0] = transparencyBlendDesc;

	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&transparencyPsoDesc, IID_PPV_ARGS(&mPSOs["transparency"])));

	auto alphaTestPsoDesc(opaquePsoDesc);
	alphaTestPsoDesc.PS = {
		reinterpret_cast<BYTE*>(mShaders["alphaPS"]->GetBufferPointer()),
		mShaders["alphaPS"]->GetBufferSize()
	};
	alphaTestPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&alphaTestPsoDesc, IID_PPV_ARGS(&mPSOs["alphaTest"])));

	auto peoplePsoDesc(opaquePsoDesc);
	peoplePsoDesc.InputLayout = { mPeopleInputLayout.data(),(UINT)mPeopleInputLayout.size() };
	peoplePsoDesc.VS = {
		reinterpret_cast<BYTE*>(mShaders["PeopleVS"]->GetBufferPointer()),
		mShaders["PeopleVS"]->GetBufferSize()
	};
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&peoplePsoDesc, IID_PPV_ARGS(&mPSOs["people"])));

	auto markMirrorPsoDesc(opaquePsoDesc);
	CD3DX12_BLEND_DESC mirrorBlendState(D3D12_DEFAULT);
	D3D12_DEPTH_STENCIL_DESC mirrorDSS;

	mirrorBlendState.RenderTarget[0].RenderTargetWriteMask = 0;

	mirrorDSS.DepthEnable = true;
	mirrorDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;
	mirrorDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	mirrorDSS.StencilEnable = true;
	mirrorDSS.StencilReadMask = 0xff;
	mirrorDSS.StencilWriteMask = 0xff;

	mirrorDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	mirrorDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	mirrorDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	mirrorDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
	mirrorDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;

	markMirrorPsoDesc.BlendState = mirrorBlendState;
	markMirrorPsoDesc.DepthStencilState = mirrorDSS;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&markMirrorPsoDesc, IID_PPV_ARGS(mPSOs["markStencilMirror"].GetAddressOf())));

	auto drawReflectionsPsoDesc(alphaTestPsoDesc);
	D3D12_DEPTH_STENCIL_DESC reflectionsDSS;

	reflectionsDSS.DepthEnable = true;
	reflectionsDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	reflectionsDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	reflectionsDSS.StencilEnable = true;
	reflectionsDSS.StencilReadMask = 0xff;
	reflectionsDSS.StencilWriteMask = 0xff;

	reflectionsDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	reflectionsDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
	reflectionsDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	drawReflectionsPsoDesc.DepthStencilState = reflectionsDSS;
	drawReflectionsPsoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	drawReflectionsPsoDesc.RasterizerState.FrontCounterClockwise = true;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&drawReflectionsPsoDesc, IID_PPV_ARGS(&mPSOs["drawStencilReflections"])));

	// We are going to draw shadows with transparency, so base it off the transparency description.
	D3D12_DEPTH_STENCIL_DESC shadowDSS;
	shadowDSS.DepthEnable = true;
	shadowDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	shadowDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	shadowDSS.StencilEnable = true;
	shadowDSS.StencilReadMask = 0xff;
	shadowDSS.StencilWriteMask = 0xff;

	shadowDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	// We are not rendering backfacing polygons, so these settings do not matter.
	shadowDSS.BackFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.BackFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
	shadowDSS.BackFace.StencilPassOp = D3D12_STENCIL_OP_INCR;
	shadowDSS.BackFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC shadowPsoDesc = transparencyPsoDesc;
	shadowPsoDesc.DepthStencilState = shadowDSS;
	ThrowIfFailed(md3dDevice->CreateGraphicsPipelineState(&shadowPsoDesc, IID_PPV_ARGS(&mPSOs["shadow"])));
}

void Demo::BuildFrameResources()
{
	for (int i = 0; i < gNumFrameResources; ++i)
	{
		mFrameResources.push_back(std::make_unique<d3dUtil::FrameResource>(
			md3dDevice.Get(), 2, mAllRitems.size(), mMaterials.size(),1, mPeopleSkeleton.size()
		));
	}
}

void Demo::BuildRenderItems()
{
	using namespace d3dUtil;
	auto boxRItem(std::make_unique<RenderItem>());
	DirectX::XMStoreFloat4x4(
		&boxRItem->mWorld,
		(DirectX::XMMatrixScaling(1.0f, 2.0f, 2.0f) *
			DirectX::XMMatrixTranslation(0.0f, 0.0f, -5.0f))
	);
	boxRItem->mObjCBIndex = 0;
	boxRItem->mGeo = mGeometries["boxesGeo"].get();
	boxRItem->mMat = mMaterials["grass"].get();
	boxRItem->mMatCBIndex = boxRItem->mMat->matCBIndex;
	boxRItem->mPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	boxRItem->mIndexCount = boxRItem->mGeo->DrawArgs["box1"].IndexCount;
	boxRItem->mStartIndexLocation = boxRItem->mGeo->DrawArgs["box1"].StartIndexLocation;
	boxRItem->mBaseVertexLocation = boxRItem->mGeo->DrawArgs["box1"].BaseVertexLocation;
	mAllRitems.push_back(std::move(boxRItem));

	auto vox2RItem(std::make_unique<RenderItem>());
	DirectX::XMStoreFloat4x4(
		&vox2RItem->mWorld,
		(DirectX::XMMatrixScaling(2.0f, 2.0f, 2.0f) *
			DirectX::XMMatrixTranslation(3.0f, -30.0f, 0.0f))
	);
	vox2RItem->mObjCBIndex = 1;
	vox2RItem->mGeo = mGeometries["boxesGeo"].get();
	vox2RItem->mMat = mMaterials["stone0"].get();
	vox2RItem->mMatCBIndex = vox2RItem->mMat->matCBIndex;
	vox2RItem->mPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	vox2RItem->mIndexCount = vox2RItem->mGeo->DrawArgs["box2"].IndexCount;
	vox2RItem->mStartIndexLocation = vox2RItem->mGeo->DrawArgs["box2"].StartIndexLocation;
	vox2RItem->mBaseVertexLocation = vox2RItem->mGeo->DrawArgs["box2"].BaseVertexLocation;
	mAllRitems.push_back(std::move(vox2RItem));

	auto landRItem(std::make_unique<RenderItem>());
	DirectX::XMStoreFloat4x4(
		&landRItem->mWorld,
		DirectX::XMMatrixTranslation(0.0, -10.0, 0.0)
	);
	DirectX::XMStoreFloat4x4(&landRItem->mTexTransform, DirectX::XMMatrixScaling(5.0f, 5.0f, 1.0f));
	landRItem->mObjCBIndex = 2;
	landRItem->mGeo = mGeometries["land"].get();
	landRItem->mMat = mMaterials["water"].get();
	landRItem->mMatCBIndex = landRItem->mMat->matCBIndex;
	landRItem->mPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	landRItem->mIndexCount = landRItem->mGeo->DrawArgs["land"].IndexCount;
	landRItem->mStartIndexLocation = landRItem->mGeo->DrawArgs["land"].StartIndexLocation;
	landRItem->mBaseVertexLocation = landRItem->mGeo->DrawArgs["land"].BaseVertexLocation;
	mAllRitems.push_back(std::move(landRItem));/*

	auto mirrorRItem(std::make_unique<RenderItem>());
	mirrorRItem->mWorld = MathHelper::Identity4x4();
	mirrorRItem->mObjCBIndex = 3;
	mirrorRItem->mGeo = mGeometries["wall"].get();
	mirrorRItem->mMat = mMaterials["mirror"].get();
	mirrorRItem->mMatCBIndex = mirrorRItem->mMat->matCBIndex;
	mirrorRItem->mPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	mirrorRItem->mIndexCount = mirrorRItem->mGeo->DrawArgs["mirror"].IndexCount;
	mirrorRItem->mStartIndexLocation = mirrorRItem->mGeo->DrawArgs["mirror"].StartIndexLocation;
	mirrorRItem->mBaseVertexLocation = mirrorRItem->mGeo->DrawArgs["mirror"].BaseVertexLocation;
	mAllRitems.push_back(std::move(mirrorRItem));
	
	auto wallRItem(std::make_unique<RenderItem>());
	wallRItem->mWorld = MathHelper::Identity4x4();
	wallRItem->mObjCBIndex = 4;
	wallRItem->mGeo = mGeometries["wall"].get();
	wallRItem->mMat = mMaterials["stone0"].get();
	wallRItem->mMatCBIndex = wallRItem->mMat->matCBIndex;
	wallRItem->mPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	wallRItem->mIndexCount = wallRItem->mGeo->DrawArgs["wall"].IndexCount;
	wallRItem->mStartIndexLocation = wallRItem->mGeo->DrawArgs["wall"].StartIndexLocation;
	wallRItem->mBaseVertexLocation = wallRItem->mGeo->DrawArgs["wall"].BaseVertexLocation;
	mAllRitems.push_back(std::move(wallRItem));*/

	//DirectX::XMVECTOR mirrorPlane = DirectX::XMVectorSet(0.0f, 0.0f, 1.0f, 0.0f); // xy plane
	//DirectX::XMMATRIX R = DirectX::XMMatrixReflect(mirrorPlane);
	//auto reflectBox(std::make_unique<RenderItem>());
	//*reflectBox = *mAllRitems[0];
	//DirectX::XMStoreFloat4x4(
	//	&reflectBox->mWorld,
	//	DirectX::XMLoadFloat4x4(&reflectBox->mWorld) * R
	//);
	//reflectBox->mObjCBIndex = 3;
	//mAllRitems.push_back(std::move(reflectBox));

	DirectX::XMVECTOR shadowPlane = DirectX::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // xz plane
	DirectX::XMVECTOR toMainLight = DirectX::operator-(DirectX::XMLoadFloat3(&mMainPassCB.Lights[2].Direction));
	DirectX::XMMATRIX S = DirectX::XMMatrixShadow(shadowPlane, toMainLight);
	DirectX::XMMATRIX shadowOffsetY = DirectX::XMMatrixTranslation(0.0f, 0.001f, 0.0f);
	auto shadowRItem(std::make_unique<RenderItem>());
	*shadowRItem = *mAllRitems[0];
	DirectX::XMStoreFloat4x4(&shadowRItem->mWorld, DirectX::XMLoadFloat4x4(&shadowRItem->mWorld) * S * shadowOffsetY);
	shadowRItem->mObjCBIndex = 3;
	shadowRItem->mMat = mMaterials["shadow"].get();
	shadowRItem->mMatCBIndex = shadowRItem->mMat->matCBIndex;
	mAllRitems.push_back(std::move(shadowRItem));

	int objCBIndex(4);
	for(auto& iter:mGeometries["people"]->DrawArgs)
	{
		auto peopleRItem(std::make_unique<RenderItem>());
		DirectX::XMStoreFloat4x4(
			&peopleRItem->mWorld,
			(DirectX::XMMatrixScaling(0.3, 0.3, 0.3) *
				DirectX::XMMatrixTranslation(0.0, 0.0, 0.0)
				));
		peopleRItem->mObjCBIndex = objCBIndex;
		objCBIndex++;
		peopleRItem->mGeo = mGeometries["people"].get();
		peopleRItem->mMat = mMaterials[iter.first].get();
		peopleRItem->mMatCBIndex = peopleRItem->mMat->matCBIndex;
		peopleRItem->mPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
		peopleRItem->mIndexCount = iter.second.IndexCount;
		peopleRItem->mStartIndexLocation = iter.second.StartIndexLocation;
		peopleRItem->mBaseVertexLocation = iter.second.BaseVertexLocation;
		mAllRitems.push_back(std::move(peopleRItem));
	}

	mAlphaTestRItems.push_back(mAllRitems[0].get());
	mOpaqueRitems.push_back(mAllRitems[1].get());
	mTransRItems.push_back(mAllRitems[2].get());
	mTransRItems.push_back(mAllRitems[3].get());/*
	mMirror.push_back(mAllRitems[3].get());
	mOpaqueRitems.push_back(mAllRitems[4].get());*/
	//mReflect.push_back(mAllRitems[3].get());
	mShadow.push_back(mAllRitems[3].get());
	for (int i = 4; i < objCBIndex; ++i)
	{
		mPeopleRItems.push_back(mAllRitems[i].get());
	}
}

void Demo::DrawRenderItems(ID3D12GraphicsCommandList* cmdList, const std::vector<d3dUtil::RenderItem*>& ritems)
{
	UINT objCBByteSize(d3dUtilStatic::CalcConstantBufferByteSize(sizeof(d3dUtil::ObjectConstants)));
	UINT matCBByteSize(d3dUtilStatic::CalcConstantBufferByteSize(sizeof(d3dUtil::MaterialConstants)));
	auto objectCB(mCurrFrameResource->ObjectCB->GetResource());
	auto materialCB(mCurrFrameResource->MaterialCB->GetResource());

	for (auto iter : ritems)
	{
		cmdList->IASetVertexBuffers(0, 1, &iter->mGeo->VertexBufferView());
		cmdList->IASetIndexBuffer(&iter->mGeo->IndexBufferView());
		cmdList->IASetPrimitiveTopology(iter->mPrimitiveType);

		D3D12_GPU_VIRTUAL_ADDRESS objCbAddress(objectCB->GetGPUVirtualAddress());
		D3D12_GPU_VIRTUAL_ADDRESS matCbAddress(materialCB->GetGPUVirtualAddress());
		objCbAddress += iter->mObjCBIndex * objCBByteSize;
		matCbAddress += iter->mMatCBIndex * matCBByteSize;

		CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
		tex.Offset(iter->mMat->diffuseSrvHeapIndex, mCbvSrvDescriptorSize);

		cmdList->SetGraphicsRootDescriptorTable(0, tex);
		cmdList->SetGraphicsRootConstantBufferView(1, objCbAddress);
		cmdList->SetGraphicsRootConstantBufferView(2, matCbAddress);

		cmdList->DrawIndexedInstanced(iter->mIndexCount, 1, iter->mStartIndexLocation, iter->mBaseVertexLocation, 0);
	}
}

std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Demo::GetStaticSamplers()
{
	const CD3DX12_STATIC_SAMPLER_DESC pointWrap(
		0, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC pointClamp(
		1, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_POINT, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearWrap(
		2, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC linearClamp(
		3, // shaderRegister
		D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP); // addressW

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(
		4, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,  // addressW
		0.0f,                             // mipLODBias
		8);                               // maxAnisotropy

	const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(
		5, // shaderRegister
		D3D12_FILTER_ANISOTROPIC, // filter
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressU
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressV
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,  // addressW
		0.0f,                              // mipLODBias
		8);                                // maxAnisotropy

	return {
		pointWrap, pointClamp,
		linearWrap, linearClamp,
		anisotropicWrap, anisotropicClamp };
}