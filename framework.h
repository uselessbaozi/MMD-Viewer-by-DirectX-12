#pragma once

#include "targetver.h"
#define WIN32_LEAN_AND_MEAN             // 从 Windows 头文件中排除极少使用的内容
// Windows 头文件
#include <windows.h>
#include <windowsx.h>
#include <comdef.h>
#include <wrl/client.h>
template <class T>
using CComPtr = Microsoft::WRL::ComPtr<T>;

// D3D12 头文件
#include <d3d12.h>
#include "d3dx12.h"
#include <d3dcompiler.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>
#include <DirectXColors.h>
#include <DirectXCollision.h>
#include <DDSTextureLoader.h>
#include <ResourceUploadBatch.h>
#pragma comment(lib, "D3D12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

// C++ 标准库
#include <array>
#include <cmath>
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <unordered_map>
#include <fstream>
#include <variant>

// 其他宏
inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}
#ifndef ThrowIfFailed
#define ThrowIfFailed(x)												\
{																		\
	HRESULT hr__ = (x);												\
	std::wstring wfn = AnsiToWString(__FILE__);							\
	if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); }	\
}
#endif

#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0; } }
#endif