#include "pch.h"
#include "d3dUtil.h"
using namespace d3dUtil;

FrameResource::FrameResource(ID3D12Device* device, UINT passCount, UINT objectCount, UINT materialCount, UINT peopleVertexCount, UINT skeletonCount)
{
	ThrowIfFailed(device->CreateCommandAllocator(
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		IID_PPV_ARGS(CmdListAlloc.GetAddressOf())
	));

	PassCB = std::make_unique<UploadBuffer<PassConstants>>(device, passCount, true);
	ObjectCB = std::make_unique<UploadBuffer<ObjectConstants>>(device, objectCount, true);
	MaterialCB = std::make_unique<UploadBuffer<MaterialConstants>>(device, materialCount, true);
	SkeletonVB = std::make_unique<UploadBuffer<Vector2>>(device, skeletonCount, false);
}

FrameResource::~FrameResource()
{
}

static void Subdivide(MeshData& meshData)
{

}

MeshData d3dUtilStatic::CreateGrid(float width, float depth, uint32_t m, uint32_t n)
{
	MeshData resultMesh;

	uint32_t vertexCount(m * n);
	uint32_t faceCount((m - 1) * (n - 1) * 2);

	float halfWidth(0.5 * width),
		halfDepth(0.5 * width);

	float dx(width / (n - 1)),
		dz = (depth / (m - 1));

	float du(1.0 / (n - 1)),
		dv(1.0 / (m - 1));

	resultMesh.vertex.resize(vertexCount);
	for (auto i = 0u; i < m; ++i)
	{
		float z(halfDepth - i * dz);
		for (auto j = 0u; j < n; ++j)
		{
			float x(-halfWidth + j * dx);
			int vertexIndex(i * n + j);

			resultMesh.vertex[vertexIndex].pos = DirectX::XMFLOAT3(x, 0.0, z);
			resultMesh.vertex[vertexIndex].normal = DirectX::XMFLOAT3(0.0, 1.0, 0.0);
			resultMesh.vertex[vertexIndex].texC.x = j * du;
			resultMesh.vertex[vertexIndex].texC.y = i * dv;
		}
	}

	resultMesh.index.resize(faceCount * 3);

	auto k = 0u;
	for (auto i = 0u; i < m - 1; ++i)
	{
		for (auto j = 0u; j < n - 1; ++j)
		{
			resultMesh.index[k] = i * n + j;
			resultMesh.index[k + 1] = i * n + j + 1;
			resultMesh.index[k + 2] = (i + 1) * n + j;

			resultMesh.index[k + 3] = (i + 1) * n + j;
			resultMesh.index[k + 4] = i * n + j + 1;
			resultMesh.index[k + 5] = (i + 1) * n + j + 1;

			k += 6; // next quad
		}
	}

	return resultMesh;
}

MeshData d3dUtilStatic::CreateBox(float width, float height, float depth, uint32_t numSubdivisions)
{
	MeshData result;

	Vector1 v[24];

	float w2 = 0.5f * width;
	float h2 = 0.5f * height;
	float d2 = 0.5f * depth;

	// Fill in the front face Vector1 data.
	v[0] = Vector1(-w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f);
	v[1] = Vector1(-w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	v[2] = Vector1(+w2, +h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f);
	v[3] = Vector1(+w2, -h2, -d2, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f);

	// Fill in the back face Vector1 data.
	v[4] = Vector1(-w2, -h2, +d2, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f);
	v[5] = Vector1(+w2, -h2, +d2, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f);
	v[6] = Vector1(+w2, +h2, +d2, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f);
	v[7] = Vector1(-w2, +h2, +d2, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f);

	// Fill in the top face Vector1 data.
	v[8] = Vector1(-w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f);
	v[9] = Vector1(-w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f);
	v[10] = Vector1(+w2, +h2, +d2, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f);
	v[11] = Vector1(+w2, +h2, -d2, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the bottom face Vector1 data.
	v[12] = Vector1(-w2, -h2, -d2, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f);
	v[13] = Vector1(+w2, -h2, -d2, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f);
	v[14] = Vector1(+w2, -h2, +d2, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f);
	v[15] = Vector1(-w2, -h2, +d2, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f);

	// Fill in the left face Vector1 data.
	v[16] = Vector1(-w2, -h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[17] = Vector1(-w2, +h2, +d2, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[18] = Vector1(-w2, +h2, -d2, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[19] = Vector1(-w2, -h2, -d2, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	// Fill in the right face Vector1 data.
	v[20] = Vector1(+w2, -h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f);
	v[21] = Vector1(+w2, +h2, -d2, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f);
	v[22] = Vector1(+w2, +h2, +d2, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f);
	v[23] = Vector1(+w2, -h2, +d2, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f);

	result.vertex.assign(&v[0], &v[24]);

	//
	// Create the indices.
	//

	uint16_t i[36];

	// Fill in the front face index data
	i[0] = 0; i[1] = 1; i[2] = 2;
	i[3] = 0; i[4] = 2; i[5] = 3;

	// Fill in the back face index data
	i[6] = 4; i[7] = 5; i[8] = 6;
	i[9] = 4; i[10] = 6; i[11] = 7;

	// Fill in the top face index data
	i[12] = 8; i[13] = 9; i[14] = 10;
	i[15] = 8; i[16] = 10; i[17] = 11;

	// Fill in the bottom face index data
	i[18] = 12; i[19] = 13; i[20] = 14;
	i[21] = 12; i[22] = 14; i[23] = 15;

	// Fill in the left face index data
	i[24] = 16; i[25] = 17; i[26] = 18;
	i[27] = 16; i[28] = 18; i[29] = 19;

	// Fill in the right face index data
	i[30] = 20; i[31] = 21; i[32] = 22;
	i[33] = 20; i[34] = 22; i[35] = 23;

	result.index.assign(&i[0], &i[36]);

	// Put a cap on the number of subdivisions.
	numSubdivisions = std::min<uint16_t>(numSubdivisions, 6u);

	return result;
}
