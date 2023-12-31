#pragma once
#include "framework.h"
#include "d3dUtil.h"
using namespace std;
static int SkeletonIndexType;
static int ExtraVectorNum;
static int VertexIndexType;
static int TextureIndexType;
static int DeformationIndexType;
static int MaterialIndexType;
static int RigidIndexType;
namespace d3dModel
{
	struct Text
	{
		int size;
		std::string s;
	};
	static std::ifstream& operator>>(std::ifstream& in, Text& o);

	struct Vector4
	{
		float x;
		float y;
		float z;
		float w;
	};
	static ifstream& operator>>(ifstream& in, Vector4& o);

	struct Vector3
	{
		float x;
		float y;
		float z;
	};
	static ifstream& operator>>(ifstream& in, Vector3& o);

	struct Vector2
	{
		float x;
		float y;
	};
	static ifstream& operator>>(ifstream& in, Vector2& o);

	struct BDEF1
	{
		variant<uint8_t, uint16_t, uint32_t> index;
	};
	static ifstream& operator>>(ifstream& in, BDEF1& o);

	struct BDEF2
	{
		variant<uint8_t, uint16_t, uint32_t> index0;
		variant<uint8_t, uint16_t, uint32_t> index1;
		float weight;
	};
	static ifstream& operator>>(ifstream& in, BDEF2& o);

	struct BDEF4
	{
		variant<uint8_t, uint16_t, uint32_t> index[4];
		float weight[4];
	};
	static ifstream& operator>>(ifstream& in, BDEF4& o);

	struct Vertex
	{
		Vector3 position;
		Vector3 normal;
		Vector2 tex;
		vector<Vector4> extraVec;
		char weightType;
		variant<BDEF1, BDEF2, BDEF4> skeletonWeight;
		float edgeMagnification;
	};
	static ifstream& operator>>(ifstream& in, Vertex& o);

	struct Surface
	{
		variant<array<uint8_t, 3>, array<uint16_t, 3>, array<uint32_t, 3>> indices;
	};
	static ifstream& operator>>(ifstream& in, Surface& o);

	struct Texture
	{
		Text file;
	};
	static ifstream& operator>>(ifstream& in, Texture& o);

	struct Material
	{
		Text localName;
		Text globalName;
		Vector4 diffuse;
		Vector3 specular;
		float specularStrength;
		Vector3 ambient;
		char drawFlag;
		Vector4 edgeColor;
		float edgeRatio;
		variant<uint8_t, uint16_t, uint32_t> textureIndex;
		variant<uint8_t, uint16_t, uint32_t> textureExIndex;
		char mixMode;
		char textureReference;
		char textureEx;
		Text metadata;
		int surfaceNum;
	};
	static ifstream& operator>>(ifstream& in, Material& o);

	struct ParentSkeleton
	{
		variant<uint8_t, uint16_t, uint32_t> index;
		float weight;
	};
	static ifstream& operator>>(ifstream& in, ParentSkeleton& o);

	struct SkeletonFixed
	{
		Vector3 axis;
	};
	static ifstream& operator>>(ifstream& in, SkeletonFixed& o);

	struct SkeletonLocal
	{
		Vector3 xAxis;
		Vector3 zAxis;
	};
	static ifstream& operator>>(ifstream& in, SkeletonLocal& o);

	struct Exoskeleton
	{
		variant<uint8_t, uint16_t, uint32_t> index;
	};
	static ifstream& operator>>(ifstream& in, Exoskeleton& o);

	struct AngleLimit
	{
		Vector3 min;
		Vector3 max;
	};
	static ifstream& operator>>(ifstream& in, AngleLimit& o);

	struct IKLink
	{
		variant<uint8_t, uint16_t, uint32_t> index;
		char angleLimitFlag;
		AngleLimit angleLimit;
	};
	static ifstream& operator>>(ifstream& in, IKLink& o);

	struct IK
	{
		variant<uint8_t, uint16_t, uint32_t> index;
		int loopTimes;
		float angleLimit;
		int linkNum;
		vector<IKLink> link;
	};
	static ifstream& operator>>(ifstream& in, IK& o);

	struct Skeleton
	{
		Text localName;
		Text globalName;
		Vector3 position;
		variant<uint8_t, uint16_t, uint32_t> parentIndex;
		int calculateLevel;
		uint16_t flag;
		variant<Vector3, uint8_t, uint16_t, uint32_t> finalPosition;
		ParentSkeleton parentSkeleton;
		SkeletonFixed skeletonFixed;
		SkeletonLocal skeletonLocal;
		Exoskeleton exoSkeleton;
		IK ik;
	};
	static ifstream& operator>>(ifstream& in, Skeleton& o);

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
		void Initialize(const wchar_t* fileName);
		void LoadMotion(const wchar_t* fileName);
		int Load(std::unordered_map<std::string, std::unique_ptr<d3dUtil::MeshGeometry>>& geo, std::unordered_map<std::string, std::unique_ptr<d3dUtil::Material>>& mat, std::unordered_map<std::string, std::unique_ptr<d3dUtil::Texture>>& tex, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList);
		void Update(std::vector<d3dUtil::Vector2>& skeleton);

	private:
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