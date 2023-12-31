#pragma once
#include "framework.h"
class MathHelper
{
public:
	static DirectX::XMFLOAT4X4 __stdcall Identity4x4();
	template <typename T>
	static T Clamp(const T& x, const T& low, const T& high);
	static DirectX::XMFLOAT3 __stdcall GetHillsNormal(float x, float z);
};

template<typename T>
inline T MathHelper::Clamp(const T& x, const T& low, const T& high)
{
	return x < low ? low : (x > high ? high : x);
}
