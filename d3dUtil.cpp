#include "d3dUtil.h"
#include "pch.h"
using namespace d3dUtil;

UINT d3dUtilStatic::CalcConstantBufferByteSize(UINT byteSize)
{
	return (byteSize + 255) & ~255;
}

CComPtr<ID3D12Resource> d3dUtilStatic::CreateDefaultBuffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const void* initData, UINT64 byteSize, CComPtr<ID3D12Resource>& uploadBuffer)
{
	CComPtr<ID3D12Resource> resultBuffer;

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(resultBuffer.GetAddressOf())
	));

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(byteSize),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(uploadBuffer.GetAddressOf())
	));

	D3D12_SUBRESOURCE_DATA subResourceData{};
	subResourceData.pData = initData;
	subResourceData.RowPitch = byteSize;
	subResourceData.SlicePitch = subResourceData.RowPitch;

	cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			resultBuffer.Get(),
			D3D12_RESOURCE_STATE_COMMON,
			D3D12_RESOURCE_STATE_COPY_DEST
		)
	);

	UpdateSubresources<1>(
		cmdList,
		resultBuffer.Get(),
		uploadBuffer.Get(),
		0, 0, 1,
		&subResourceData
	);

	cmdList->ResourceBarrier(
		1,
		&CD3DX12_RESOURCE_BARRIER::Transition(
			resultBuffer.Get(),
			D3D12_RESOURCE_STATE_COPY_DEST,
			D3D12_RESOURCE_STATE_GENERIC_READ
		)
	);

	return resultBuffer;
}

Microsoft::WRL::ComPtr<ID3DBlob> d3dUtilStatic::CompileShader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target)
{
	UINT compileFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)  
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	HRESULT hr;

	CComPtr<ID3DBlob> byteCode(nullptr);
	CComPtr<ID3DBlob> error;
	hr = D3DCompileFromFile(
		filename.c_str(),
		defines,
		D3D_COMPILE_STANDARD_FILE_INCLUDE,
		entrypoint.c_str(),
		target.c_str(),
		compileFlags,
		0,
		&byteCode,
		&error
	);

#if defined(DEBUG) || defined(_DEBUG)
	if (error != nullptr)
		OutputDebugStringA((char*)(error->GetBufferPointer()));
#endif

	ThrowIfFailed(hr);

	return byteCode;
}

std::unique_ptr<Texture> d3dUtilStatic::LoadDDSTexture(ID3D12Device* device, ID3D12GraphicsCommandList* cmdList, const char* name, const wchar_t* filename)
{
	auto result(std::make_unique<Texture>());
	result->name = name;
	result->filename = filename;

	std::unique_ptr<uint8_t[]> ddsData;
	std::vector<D3D12_SUBRESOURCE_DATA> subresources;
	ThrowIfFailed(DirectX::LoadDDSTextureFromFile(
		device,
		result->filename.c_str(),
		result->resourse.GetAddressOf(),
		ddsData,
		subresources
	));

	const UINT64 uploadBufferSize(GetRequiredIntermediateSize(
		result->resourse.Get(),
		0,
		static_cast<UINT>(subresources.size())
	));

	auto desc = CD3DX12_RESOURCE_DESC::Buffer(uploadBufferSize);

	ThrowIfFailed(device->CreateCommittedResource(
		&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&desc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(result->uploadBuffer.GetAddressOf())
	));

	UpdateSubresources(
		cmdList,
		result->resourse.Get(),
		result->uploadBuffer.Get(),
		0,
		0,
		static_cast<UINT>(subresources.size()),
		subresources.data()
	);

	auto barrier(CD3DX12_RESOURCE_BARRIER::Transition(
		result->resourse.Get(),
		D3D12_RESOURCE_STATE_COPY_DEST,
		D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
	));
	cmdList->ResourceBarrier(1, &barrier);

	return result;
}
