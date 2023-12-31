#pragma once
#include "framework.h"
#include "MathHelper.h"
#include "UploadBuffer.h"

const int gNumFrameResources = 3;
constexpr int MAXLIGHTS(16);

namespace d3dUtil
{
	struct Vector1
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 texC;

		Vector1() = default;
		Vector1(
			float px, float py, float pz,
			float nx, float ny, float nz,
			float u, float v) :
			pos(px, py, pz),
			normal(nx, ny, nz),
			texC(u, v) {}
		Vector1(
			const DirectX::XMFLOAT3 p,
			const DirectX::XMFLOAT3 n,
			const DirectX::XMFLOAT2 t
		) :
			pos(p),
			normal(n),
			texC(t) {}
	};

	struct Vector2
	{
		DirectX::XMFLOAT3 pos;
		DirectX::XMFLOAT3 normal;
		DirectX::XMFLOAT2 texC;

		float boneWeight[4] = { 0.0 };
		uint32_t boneIndices[4] = { 0 };
	};

	struct Light
	{
		DirectX::XMFLOAT3 Strength = { 0.5f, 0.5f, 0.5f };
		float FalloffStart = 1.0f;
		DirectX::XMFLOAT3 Direction = { 0.0f, -1.0f, 0.0f };
		float FalloffEnd = 10.0f;
		DirectX::XMFLOAT3 Position = { 0.0f, 0.0f, 0.0f };
		float SpotPower = 64.0f;
	};

	struct Texture
	{
		std::string name;
		std::wstring filename;
		int offset;

		CComPtr<ID3D12Resource> resourse;
		CComPtr<ID3D12Resource> uploadBuffer;
	};

	struct MeshData
	{
		std::vector<Vector1> vertex;
		std::vector<uint16_t> index;
	};

	// ÊÓ½Ç×ø±ê×ª»»¾ØÕó
	struct ObjectConstants
	{
		DirectX::XMFLOAT4X4 worldView = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
	};

	// ¸¨Öú¾ØÕó
	struct PassConstants
	{
		DirectX::XMFLOAT4X4 View = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 InvView = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 Proj = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 InvProj = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 ViewProj = MathHelper::Identity4x4();
		DirectX::XMFLOAT4X4 InvViewProj = MathHelper::Identity4x4();
		DirectX::XMFLOAT3 EyePosW = { 0.0f, 0.0f, 0.0f };
		float cbPerObjectPad1 = 0.0f;
		DirectX::XMFLOAT2 RenderTargetSize = { 0.0f, 0.0f };
		DirectX::XMFLOAT2 InvRenderTargetSize = { 0.0f, 0.0f };
		float NearZ = 0.0f;
		float FarZ = 0.0f;
		float offset1 = 0.0;
		float offset2 = 0.0;
		DirectX::XMFLOAT4 AmbientLight = { 0.0,0.0,0.0,0.0 };
		DirectX::XMFLOAT4 gFogColor = { 0.7f, 0.7f, 0.7f, 1.0f };
		float gFogStart = 5.0;
		float gFogRange = 150.0;
		DirectX::XMFLOAT2 cbPerObjectPad2;
		Light Lights[MAXLIGHTS];
	};

	struct SubmeshGeometry
	{
		UINT IndexCount = 0;
		UINT StartIndexLocation = 0;
		INT BaseVertexLocation = 0;

		// Bounding box of the geometry defined by this submesh. 
		// This is used in later chapters of the book.
		DirectX::BoundingBox Bounds;
	};

	struct MeshGeometry
	{
		// Give it a name so we can look it up by name.
		std::string Name;

		// System memory copies.  Use Blobs because the vertex/index format can be generic.
		// It is up to the client to cast appropriately.  
		Microsoft::WRL::ComPtr<ID3DBlob> VertexBufferCPU = nullptr;
		Microsoft::WRL::ComPtr<ID3DBlob> IndexBufferCPU = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferGPU = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferGPU = nullptr;

		Microsoft::WRL::ComPtr<ID3D12Resource> VertexBufferUploader = nullptr;
		Microsoft::WRL::ComPtr<ID3D12Resource> IndexBufferUploader = nullptr;

		// Data about the buffers.
		UINT VertexByteStride = 0;
		UINT VertexBufferByteSize = 0;
		DXGI_FORMAT IndexFormat = DXGI_FORMAT_R16_UINT;
		UINT IndexBufferByteSize = 0;

		// A MeshGeometry may store multiple geometries in one vertex/index buffer.
		// Use this container to define the Submesh geometries so we can draw
		// the Submeshes individually.
		std::unordered_map<std::string, SubmeshGeometry> DrawArgs;

		D3D12_VERTEX_BUFFER_VIEW VertexBufferView()const
		{
			D3D12_VERTEX_BUFFER_VIEW vbv;
			vbv.BufferLocation = VertexBufferGPU->GetGPUVirtualAddress();
			vbv.StrideInBytes = VertexByteStride;
			vbv.SizeInBytes = VertexBufferByteSize;

			return vbv;
		}

		D3D12_INDEX_BUFFER_VIEW IndexBufferView()const
		{
			D3D12_INDEX_BUFFER_VIEW ibv;
			ibv.BufferLocation = IndexBufferGPU->GetGPUVirtualAddress();
			ibv.Format = IndexFormat;
			ibv.SizeInBytes = IndexBufferByteSize;

			return ibv;
		}

		// We can free this memory after we finish upload to the GPU.
		void DisposeUploaders()
		{
			VertexBufferUploader = nullptr;
			IndexBufferUploader = nullptr;
		}
	};

	struct Material
	{
		std::string name;
		int matCBIndex = -1;
		int diffuseSrvHeapIndex = -1;
		int numFrameDirty = 3;

		DirectX::XMFLOAT4 diffuseAlbedo{ 1.0,1.0,1.0,1.0 };
		DirectX::XMFLOAT3 fresnelR0{ 0.01,0.01,0.01 };
		float roughness = 0.25;
		DirectX::XMFLOAT4X4 matTransform = MathHelper::Identity4x4();
	};

	struct MaterialConstants
	{
		DirectX::XMFLOAT4 diffuseAlbedo{ 1.0,1.0,1.0,1.0 };
		DirectX::XMFLOAT3 fresnelR0{ 0.01,0.01,0.01 };
		float roughness = 0.25;
		DirectX::XMFLOAT4X4 matTransform = MathHelper::Identity4x4();
	};

	struct RenderItem
	{
		RenderItem() = default;

		DirectX::XMFLOAT4X4 mWorld = MathHelper::Identity4x4();

		DirectX::XMFLOAT4X4 mTexTransform = MathHelper::Identity4x4();

		int mNumFramesDirty = gNumFrameResources;

		UINT mObjCBIndex = -1;
		UINT mMatCBIndex = -1;

		d3dUtil::MeshGeometry* mGeo = nullptr;
		d3dUtil::Material* mMat = nullptr;
		D3D12_PRIMITIVE_TOPOLOGY mPrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

		UINT mIndexCount = 0;
		UINT mStartIndexLocation = 0;
		int mBaseVertexLocation = 0;
	};

	struct FrameResource
	{
		FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT peopleVertexCount, UINT skeletonCount);
		FrameResource(const d3dUtil::FrameResource& rhs) = delete;
		FrameResource& operator=(const d3dUtil::FrameResource& rhs) = delete;
		~FrameResource();

		Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdListAlloc;

		std::unique_ptr<UploadBuffer<d3dUtil::PassConstants>> PassCB = nullptr;
		std::unique_ptr<UploadBuffer<d3dUtil::ObjectConstants>> ObjectCB = nullptr;
		std::unique_ptr<UploadBuffer<d3dUtil::MaterialConstants>> MaterialCB = nullptr;
		std::unique_ptr<UploadBuffer<Vector2>> SkeletonVB = nullptr;

		UINT64 Fence = 0;
	};
}

class d3dUtilStatic
{
public:
	static UINT CalcConstantBufferByteSize(UINT byteSize);

	static CComPtr<ID3D12Resource> CreateDefaultBuffer(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const void* initData,
		UINT64 byteSize,
		CComPtr<ID3D12Resource>& uploadBuffer
	);

	static Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target
	);

	static std::unique_ptr<d3dUtil::Texture> LoadDDSTexture(
		ID3D12Device* device,
		ID3D12GraphicsCommandList* cmdList,
		const char* name,
		const wchar_t* filename
	);

	static d3dUtil::MeshData CreateGrid(float width, float depth, uint32_t m, uint32_t n);
	static d3dUtil::MeshData CreateBox(float width, float height, float depth, uint32_t numSubdivisions);
};
