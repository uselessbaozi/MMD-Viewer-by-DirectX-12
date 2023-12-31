#ifndef NUM_DIR_LIGHTS
    #define NUM_DIR_LIGHTS 3
#endif

#ifndef NUM_POINT_LIGHTS
    #define NUM_POINT_LIGHTS 0
#endif

#ifndef NUM_SPOT_LIGHTS
    #define NUM_SPOT_LIGHTS 0
#endif


#include "LightUtil.hlsl"

Texture2D gDiffuseMap : register(t0);

SamplerState gsamPointWrap : register(s0);
SamplerState gsamPointClamp : register(s1);
SamplerState gsamLinearWrap : register(s2);
SamplerState gsamLinearClamp : register(s3);
SamplerState gsamAnisotropicWrap : register(s4);
SamplerState gsamAnisotropicClamp : register(s5);

cbuffer cbPerObject : register(b0)
{
    float4x4 worldView;
    float4x4 texTransform;
}

cbuffer cbMaterial : register(b1)
{
    float4 gDiffuseAlbedo;
    float3 gFresnelR0;
    float gRoughness;
    float4x4 gMatTransform;
};

cbuffer cbPass : register(b2)
{
    float4x4 gView;
    float4x4 gInvView;
    float4x4 gProj;
    float4x4 gInvProj;
    float4x4 gViewProj;
    float4x4 gInvViewProj;
    float3 gEyePosW;
    float cbPerObjectPad1;
    float2 gRenderTargetSize;
    float2 gInvRenderTargetSize;
    float gNearZ;
    float gFarZ;
    float4 gAmbientLight;
    
    Light gLight[MAXLINGTS];
};

cbuffer cbSkeleton : register(b3)
{
    float4x4 gSkeletonTrans[800];
}

struct VectorIn
{
    float3 posL : POSITION;
    float3 normalL : NORMAL;
    float2 texC : TEXCOORD;
};

struct VectorOut
{
    float4 posH : SV_POSITION;
    float3 posW : POSITION;
    float3 normalW : NORMAL;
    float2 texC : TEXCOORD;
};

VectorOut VS(VectorIn vIn)
{
    VectorOut vout = (VectorOut) 0.0f;
	
    // Transform to world space.
    float4 posW = mul(float4(vIn.posL, 1.0f), worldView);
    vout.posW = posW.xyz;

    // Assumes nonuniform scaling; otherwise, need to use inverse-transpose of world matrix.
    vout.normalW = mul(vIn.normalL, (float3x3) worldView);

    // Transform to homogeneous clip space.
    vout.posH = mul(posW, gViewProj);
    
    float4 texC = mul(float4(vIn.texC, 0.0, 1.0), texTransform);
    vout.texC = mul(texC, gMatTransform).xy;

    return vout;
}