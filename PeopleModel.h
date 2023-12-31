#pragma once
#include "framework.h"
#include "d3dUtil.h"
namespace d3dModel
{
	DirectX::XMFLOAT3 fbxV4xm3(const fbxsdk::FbxVector4& o);
	DirectX::XMFLOAT3 fbxV4xm3forPos(const fbxsdk::FbxVector4& o);
	DirectX::XMFLOAT2 fbxV2xm2(const fbxsdk::FbxVector2& o);
	DirectX::XMFLOAT4 fbxV4xm4forPos(const fbxsdk::FbxVector4& o);
	fbxsdk::FbxVector4 xm4fbxV4(const DirectX::XMFLOAT4& o);
	DirectX::XMFLOAT4 fbxD3xm4(const fbxsdk::FbxDouble3& o);
	DirectX::XMFLOAT3 fbxD3xm3(const fbxsdk::FbxDouble3& o);
	DirectX::XMFLOAT4 fbxD4xm4(const fbxsdk::FbxDouble4& o);
	DirectX::XMFLOAT4X4 fbxMAxmM(const fbxsdk::FbxAMatrix& o);

	struct BoneKeyFrame
	{
		std::string name;
		struct
		{
			uint32_t FrameTime;
			float Translation[3];
			float Rotation[4];
			uint8_t XCurve[16];
			uint8_t YCurve[16];
			uint8_t ZCurve[16];
			uint8_t RCurve[16];
		}FrameResource;
	};
	static std::ifstream& operator>>(std::ifstream& fin, BoneKeyFrame& result);
	struct BoneAnimation
	{
		DirectX::XMFLOAT3 translation;
		DirectX::XMFLOAT4 rotation;
		uint32_t frameTime;

		BoneAnimation() :translation(0.0, 0.0, 0.0), rotation(0.0, 0.0, 0.0, 1.0), frameTime(0) {}
		BoneAnimation(float t[3], float r[4], uint32_t f) :translation(t[0] * 0.04, t[1] * 0.04, t[2] * 0.04), rotation(-r[0], -r[1], r[2], r[3]), frameTime(f) {}
	};
	DirectX::XMMATRIX GetTransformMatrix(BoneAnimation& start, BoneAnimation& end, float percent, std::pair<DirectX::XMVECTOR, DirectX::XMVECTOR> & datum);

	class PeopleModel
	{
	public:
		PeopleModel();
		~PeopleModel();
		void Initialize(const char* fileName);
		void LoadMotion(const wchar_t* fileName);
		int Load(std::unordered_map<std::string, std::unique_ptr<d3dUtil::MeshGeometry>>& geo, std::unordered_map<std::string, std::unique_ptr<d3dUtil::Material>>& mat, std::unordered_map<std::string, std::unique_ptr<d3dUtil::Texture>>& tex, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
		void Update(std::vector<d3dUtil::Vector2>& skeleton);
	private:
		void ProcessNode(fbxsdk::FbxNode* node);
		void ProcessMesh(fbxsdk::FbxNode* node);
		void ProcessSkeleton(fbxsdk::FbxNode* node);
		void ProcessSkeletonHelper(fbxsdk::FbxNode* node, int parentIndex);

		std::string mFileName;
		int mMeshSize;
		int mIndexSize;
		std::vector<d3dUtil::Vector2> mMesh;
		std::vector<uint32_t> mIndexOffset;
		std::vector<uint32_t> mIndex;
		std::vector<std::unique_ptr<d3dUtil::Material>> mMaterial;
		std::vector<std::string> mMaterialByIndex;
		std::vector<std::string> mTextureName;
		std::vector<std::pair<std::string, int>> mSkeleton;
		std::unordered_map<std::string, int> mSkeletonWithIndex;
		std::vector<DirectX::XMFLOAT4X4> mSkeletonTransform;
		std::vector<DirectX::XMFLOAT4X4> mSkeletonTransformLink;
		std::vector<std::pair<DirectX::XMVECTOR, DirectX::XMVECTOR>> mSkeletonPoint;
		std::vector<std::vector<BoneAnimation>> mSkeletonMotion;
	};
}