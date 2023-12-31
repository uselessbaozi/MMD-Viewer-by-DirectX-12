#include "pch.h"
#include "PeopleModel.h"
using namespace DirectX;

d3dModel::PeopleModel::PeopleModel()
{
}

d3dModel::PeopleModel::~PeopleModel()
{
}

void d3dModel::PeopleModel::Initialize(const wchar_t* fileName)
{
	std::ifstream model(fileName, std::ios::binary | std::ios::in);

	char signature[5];
	model.read(signature, sizeof(char) * 4);

	float version;
	model.read((char*)&version, sizeof(float));

	char globalInformationNum;
	model.read((char*)&globalInformationNum, sizeof(char));

	char stringCode;
	model.read((char*)&stringCode, sizeof(char));

	char extraVectorNum;
	model.read((char*)&extraVectorNum, sizeof(char));
	ExtraVectorNum = extraVectorNum;

	char vectorIndexSize;
	model.read((char*)&vectorIndexSize, sizeof(char));
	switch (vectorIndexSize)
	{
	case 1:
		VertexIndexType = 0;
		break;
	case 2:
		VertexIndexType = 1;
		break;
	case 4:
		VertexIndexType = 2;
		break;
	default:
		throw - 1;
	}

	char texIndexSize;
	model.read((char*)&texIndexSize, sizeof(char));
	switch (texIndexSize)
	{
	case 1:
		TextureIndexType = 0;
		break;
	case 2:
		TextureIndexType = 1;
		break;
	case 4:
		TextureIndexType = 2;
		break;
	default:
		throw - 1;
	}

	char matIndexSize;
	model.read((char*)&matIndexSize, sizeof(char));
	switch (matIndexSize)
	{
	case 1:
		MaterialIndexType = 0;
		break;
	case 2:
		MaterialIndexType = 1;
		break;
	case 4:
		MaterialIndexType = 2;
		break;
	default:
		throw - 1;
	}

	char skeletonIndexSize;
	model.read((char*)&skeletonIndexSize, sizeof(char));
	switch (skeletonIndexSize)
	{
	case 1:
		SkeletonIndexType = 0;
		break;
	case 2:
		SkeletonIndexType = 1;
		break;
	case 4:
		SkeletonIndexType = 2;
		break;
	default:
		throw - 1;
	}

	char deformationIndexSize;
	model.read((char*)&deformationIndexSize, sizeof(char));
	switch (deformationIndexSize)
	{
	case 1:
		DeformationIndexType = 0;
		break;
	case 2:
		DeformationIndexType = 1;
		break;
	case 4:
		DeformationIndexType = 2;
		break;
	default:
		throw - 1;
	}

	char rigidIndexSize;
	model.read((char*)&rigidIndexSize, sizeof(char));
	switch (rigidIndexSize)
	{
	case 1:
		RigidIndexType = 0;
		break;
	case 2:
		RigidIndexType = 1;
		break;
	case 4:
		RigidIndexType = 2;
		break;
	default:
		throw - 1;
	}

	Text modelLocalName;
	model >> modelLocalName;

	Text modelGlobalName;
	model >> modelGlobalName;

	Text modelLocalDescribe;
	model >> modelLocalDescribe;

	Text modelGlobalDescribe;
	model >> modelGlobalDescribe;

	int vertexNum;
	model.read((char*)&vertexNum, sizeof(int32_t));
	mMeshSize = vertexNum;

	for (auto i = 0u; i < vertexNum; ++i)
	{
		Vertex iter;
		model >> iter;

		d3dUtil::Vector2 x;
		x.pos = XMFLOAT3{ iter.position.x,iter.position.y,iter.position.z };
		x.normal = XMFLOAT3{ iter.normal.x,iter.normal.y,iter.normal.z };
		x.texC = XMFLOAT2{ iter.tex.x,iter.tex.y };
		switch (iter.skeletonWeight.index())
		{
		case 0:
		{
			auto& temp(get<BDEF1>(iter.skeletonWeight));
			visit([&x](const auto& value) { x.boneIndices[0] = value; }, temp.index);
			x.boneWeight[0] = 1.0;
			break;
		}
		case 1:
		{
			auto& temp(get<BDEF2>(iter.skeletonWeight));
			visit([&x](const auto& value) { x.boneIndices[0] = value; }, temp.index0);
			visit([&x](const auto& value) { x.boneIndices[1] = value; }, temp.index1);
			x.boneWeight[0] = temp.weight;
			x.boneWeight[1] = 1.0 - temp.weight;
			break;
		}
		case 2:
		{
			auto& temp(get<BDEF4>(iter.skeletonWeight));
			visit([&x](const auto& value) { x.boneIndices[0] = value; }, temp.index[0]);
			visit([&x](const auto& value) { x.boneIndices[1] = value; }, temp.index[1]);
			visit([&x](const auto& value) { x.boneIndices[2] = value; }, temp.index[2]);
			visit([&x](const auto& value) { x.boneIndices[3] = value; }, temp.index[3]);
			x.boneWeight[0] = temp.weight[0];
			x.boneWeight[1] = temp.weight[1];
			x.boneWeight[2] = temp.weight[2];
			x.boneWeight[3] = temp.weight[3];
			break;
		}
		default:
			break;
		}

		mMesh.push_back(x);
	}
	mMeshSize = mMesh.size();

	int surfaceNum;
	model.read((char*)&surfaceNum, sizeof(int32_t));
	surfaceNum = surfaceNum / 3;

	for (auto i = 0u; i < surfaceNum; ++i)
	{
		Surface iter;
		model >> iter;

		switch (iter.indices.index())
		{
		case 0:
		{
			auto& temp(get<array<uint8_t, 3>>(iter.indices));
			mIndex.push_back(temp[0]);
			mIndex.push_back(temp[1]);
			mIndex.push_back(temp[2]);
			break;
		}
		case 1:
		{
			auto& temp(get<array<uint16_t, 3>>(iter.indices));
			mIndex.push_back(temp[0]);
			mIndex.push_back(temp[1]);
			mIndex.push_back(temp[2]);
			break;
		}
		case 2:
		{
			auto& temp(get<array<uint32_t, 3>>(iter.indices));
			mIndex.push_back(temp[0]);
			mIndex.push_back(temp[1]);
			mIndex.push_back(temp[2]);
			break;
		}
		default:
			break;
		}
	}
	mIndexSize = mIndex.size();

	int textureNum;
	model.read((char*)&textureNum, sizeof(int32_t));

	for (auto i = 0u; i < textureNum; ++i)
	{
		Texture iter;
		model >> iter;
		mTextureName.push_back(iter.file.s);
	}

	int materialNum;
	model.read((char*)&materialNum, sizeof(int32_t));

	int indexOffset(0);
	bool mat_body_first_time(true);
	for (auto i = 0u; i < materialNum; ++i)
	{
		Material iter;
		model >> iter;

		auto x(std::make_unique<d3dUtil::Material>());
		x->name = iter.localName.s;
		if (x->name == "·þÊÎ" && mat_body_first_time)
		{
			x->name += "3";
			mat_body_first_time = false;
		}
		mMaterialByIndex.push_back(x->name);
		x->matCBIndex = i;
		x->diffuseAlbedo = XMFLOAT4(iter.diffuse.x, iter.diffuse.y, iter.diffuse.z, iter.diffuse.w);
		x->fresnelR0 = XMFLOAT3(0.0, 0.0, 0.0);
		x->roughness = 0.0;
		switch (iter.textureIndex.index())
		{
		case 0:
			x->diffuseSrvHeapIndex = get<uint8_t>(iter.textureIndex);
			break;
		case 1:
			x->diffuseSrvHeapIndex = get<uint16_t>(iter.textureIndex);
			break;
		case 2:
			x->diffuseSrvHeapIndex = get<uint32_t>(iter.textureIndex);
			break;
		default:
			break;
		}
		mMaterial.push_back(move(x));

		mIndexOffset.push_back(indexOffset);
		indexOffset += iter.surfaceNum;
	}

	int skeletonNum;
	model.read((char*)&skeletonNum, sizeof(int32_t));

	for (auto i = 0u; i < skeletonNum; ++i)
	{
		Skeleton iter;
		model >> iter;


	}

	model.close();
}

void d3dModel::PeopleModel::LoadMotion(const wchar_t* fileName)
{
	std::ifstream file(fileName, std::ios::binary | std::ios::in);

	char version[30], modelName1[10], modelName2[20];
	file.read(version, sizeof(char) * 30);
	if (version[21] != '0')
	{
		file.read(modelName1, sizeof(char) * 10);
	}
	else
	{
		file.read(modelName2, sizeof(char) * 20);
	}

	uint32_t boneKeyFrameNumber;
	BoneKeyFrame tempBoneKeyFrame;
	file.read((char*)&boneKeyFrameNumber, sizeof(uint32_t));
	if (boneKeyFrameNumber)
	{
		for (auto i = 0u; i < boneKeyFrameNumber; ++i)
		{
			file >> tempBoneKeyFrame;
			if (tempBoneKeyFrame.name == "")
				throw - 1;
			mSkeletonMotion[mSkeletonWithIndex[tempBoneKeyFrame.name]].push_back(BoneAnimation(tempBoneKeyFrame.FrameResource.Translation, tempBoneKeyFrame.FrameResource.Rotation, tempBoneKeyFrame.FrameResource.FrameTime));
		}
	}
}

int d3dModel::PeopleModel::Load(std::unordered_map<std::string, std::unique_ptr<d3dUtil::MeshGeometry>>& geo, std::unordered_map<std::string, std::unique_ptr<d3dUtil::Material>>& mat, std::unordered_map<std::string, std::unique_ptr<d3dUtil::Texture>>& tex, ID3D12Device* device, ID3D12GraphicsCommandList* cmdList)
{
	using namespace DirectX;
	std::vector<d3dUtil::SubmeshGeometry> meshGeos(mIndexOffset.size());
	for (auto i = 0u; i < mIndexOffset.size(); ++i)
	{
		auto& meshGeo(meshGeos[i]);
		if (i == mIndexOffset.size() - 1)
			meshGeo.IndexCount = mIndexSize - mIndexOffset[i];
		else
			meshGeo.IndexCount = mIndexOffset[i + 1] - mIndexOffset[i];
		meshGeo.StartIndexLocation = mIndexOffset[i];
		meshGeo.BaseVertexLocation = 0;
	}

	const UINT vbByteSize(mMesh.size() * sizeof(d3dUtil::Vector2)),
		ibByteSize(mIndex.size() * sizeof(uint32_t));

	auto tempGeo(std::make_unique<d3dUtil::MeshGeometry>());
	tempGeo->Name = "people";

	ThrowIfFailed(D3DCreateBlob(vbByteSize, &tempGeo->VertexBufferCPU));
	CopyMemory(tempGeo->VertexBufferCPU->GetBufferPointer(), mMesh.data(), vbByteSize);
	ThrowIfFailed(D3DCreateBlob(ibByteSize, &tempGeo->IndexBufferCPU));
	CopyMemory(tempGeo->IndexBufferCPU->GetBufferPointer(), mIndex.data(), ibByteSize);

	tempGeo->VertexBufferGPU = d3dUtilStatic::CreateDefaultBuffer(
		device,
		cmdList,
		mMesh.data(),
		vbByteSize,
		tempGeo->VertexBufferUploader
	);
	tempGeo->IndexBufferGPU = d3dUtilStatic::CreateDefaultBuffer(
		device,
		cmdList,
		mIndex.data(),
		ibByteSize,
		tempGeo->IndexBufferUploader
	);

	tempGeo->VertexByteStride = sizeof(d3dUtil::Vector2);
	tempGeo->VertexBufferByteSize = vbByteSize;
	tempGeo->IndexBufferByteSize = ibByteSize;
	tempGeo->IndexFormat = DXGI_FORMAT_R32_UINT;

	for (auto i = 0u; i < meshGeos.size(); ++i)
	{
		tempGeo->DrawArgs[mMaterialByIndex[i]] = meshGeos[i];
	}

	geo[tempGeo->Name] = std::move(tempGeo);

	auto matSize(mat.size());
	for (auto& iter : mMaterial)
	{
		iter->matCBIndex += matSize;
		iter->diffuseSrvHeapIndex = tex[mTextureName[iter->diffuseSrvHeapIndex]]->offset;
		mat[iter->name] = std::move(iter);
	}

	return mMeshSize;
}

void d3dModel::PeopleModel::Update(std::vector<d3dUtil::Vector2>& skeleton)
{
	static int frameTime(10), motionTime(-1);
	if (frameTime < 3)
	{
		++frameTime;
		return;
	}

	frameTime = 0;
	++motionTime;
	if (mSkeletonMotion.empty())
	{
		for (auto i = 0u; i < mMeshSize; ++i)
		{
			skeleton[i] = mMesh[i];
		}
		return;
	}

	if (mSkeletonMotion[0].empty())
	{
		mSkeletonTransform[0] = MathHelper::Identity4x4();
	}
	else
	{
		auto& motion(mSkeletonMotion[0]);
		if (motion[0].frameTime >= motionTime)
		{
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[0], GetTransformMatrix(motion[0], motion[0], 0.0, mSkeletonPoint[0]));
		}
		else if (motion[motion.size() - 1].frameTime <= motionTime)
		{
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[0], GetTransformMatrix(motion[motion.size() - 1], motion[motion.size() - 1], 0.0, mSkeletonPoint[0])/* *
				DirectX::XMLoadFloat4x4(&mSkeletonTransformLink[0])*/);
		}
		else
		{
			int startMotionTime(0), endMotionTime(0), startIndex(0), endIndex(0);
			for (auto j = 0u; j < motion.size(); ++j)
			{
				if (motionTime < motion[j].frameTime)
				{
					startIndex = j - 1;
					endIndex = j;
					startMotionTime = motion[j - 1].frameTime;
					endMotionTime = motion[j].frameTime;
					break;
				}
			}

			float percent(((static_cast<float>(motionTime)) - startMotionTime) / (endMotionTime - startMotionTime));
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[0], GetTransformMatrix(motion[startIndex], motion[endIndex], percent, mSkeletonPoint[0])/* *
				DirectX::XMLoadFloat4x4(&mSkeletonTransformLink[0])*/);
		}
	}
	for (auto i = 1u; i < mSkeletonTransform.size(); ++i)
	{
		auto& motion(mSkeletonMotion[i]);
		auto parentTrans(DirectX::XMLoadFloat4x4(&mSkeletonTransform[mSkeleton[i].second]));
		if (motion.empty())
		{
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[i],
				parentTrans/* *
				DirectX::XMLoadFloat4x4(&mSkeletonTransformLink[i])*/);
			continue;
		}
		if (motion[0].frameTime >= motionTime)
		{
			auto temp(GetTransformMatrix(motion[0], motion[0], 0.0, mSkeletonPoint[i]));
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[i],
				parentTrans * temp /* *
				DirectX::XMLoadFloat4x4(&mSkeletonTransformLink[i])*/);
		}
		else if (motion[motion.size() - 1].frameTime <= motionTime)
		{
			auto temp(GetTransformMatrix(motion[motion.size() - 1], motion[motion.size() - 1], 0.0, mSkeletonPoint[i]));
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[i],
				parentTrans * temp /* *
				DirectX::XMLoadFloat4x4(&mSkeletonTransformLink[i])*/);
		}
		else
		{
			int startMotionTime(0), endMotionTime(0), startIndex(0), endIndex(0);
			for (auto j = 0u; j < motion.size(); ++j)
			{
				if (motionTime < motion[j].frameTime)
				{
					startIndex = j - 1;
					endIndex = j;
					startMotionTime = motion[j - 1].frameTime;
					endMotionTime = motion[j].frameTime;
					break;
				}
			}

			float percent(((static_cast<float>(motionTime)) - startMotionTime) / (endMotionTime - startMotionTime));
			auto temp(GetTransformMatrix(motion[startIndex], motion[endIndex], percent, mSkeletonPoint[i]));
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[i],
				parentTrans * temp/* *
				DirectX::XMLoadFloat4x4(&mSkeletonTransformLink[i])*/);
		}
	}

	//for (auto i = 0u; i < mSkeleton.size(); ++i)
	//{
	//	auto transLoaded(XMLoadFloat4x4(&mSkeletonTransform[i])),
	//		globalLoaded(XMLoadFloat4x4(&mSkeletonTransformLink[i]));
	//	XMStoreFloat4x4(&mSkeletonTransform[i], transLoaded * globalLoaded);
	//}

	for (auto i = 0u; i < mMeshSize; ++i)
	{
		d3dUtil::Vector2 temp(mMesh[i]);
		DirectX::XMFLOAT3 pos(0, 0, 0), normal(0, 0, 0);
		auto loadedPos(XMLoadFloat3(&pos)), loadedNormal(XMLoadFloat3(&normal));
		for (auto j = 0u; j < 4; ++j)
		{
			loadedPos += temp.boneWeight[j] * XMVector3TransformCoord(DirectX::XMLoadFloat3(&temp.pos), XMLoadFloat4x4(&mSkeletonTransform[temp.boneIndices[j]]));
			loadedNormal += temp.boneWeight[j] * XMVector3TransformCoord(DirectX::XMLoadFloat3(&temp.normal), XMLoadFloat4x4(&mSkeletonTransform[temp.boneIndices[j]]));
		}
		XMStoreFloat3(&temp.pos, loadedPos);
		XMStoreFloat3(&temp.normal, loadedNormal);
		skeleton[i] = temp;
	}
}

std::ifstream& d3dModel::operator>>(std::ifstream& in, Text& o)
{
	in.read((char*)&o.size, sizeof(int32_t));
	wchar_t* s(new wchar_t[o.size / 2]);
	in.read((char*)s, sizeof(char) * o.size);
	UTF16LE2GBK(s, o.size / 2, o.s);
	delete[] s;
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, Vector4& o)
{
	in.read((char*)&o, sizeof(Vector4));
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, Vector3& o)
{
	in.read((char*)&o, sizeof(Vector3));
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, Vector2& o)
{
	in.read((char*)&o, sizeof(Vector2));
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, BDEF1& o)
{
	switch (SkeletonIndexType)
	{
	case 0:
	{
		uint8_t x;
		in.read((char*)&x, sizeof(uint8_t));
		o.index = x;
		break;
	}
	case 1:
	{
		uint16_t x;
		in.read((char*)&x, sizeof(uint16_t));
		o.index = x;
		break;
	}
	case 2:
	{
		uint32_t x;
		in.read((char*)&x, sizeof(uint32_t));
		o.index = x;
		break;
	}
	default:
		break;
	}
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, BDEF2& o)
{
	switch (SkeletonIndexType)
	{
	case 0:
	{
		uint8_t x;
		in.read((char*)&x, sizeof(uint8_t));
		o.index0 = x;
		in.read((char*)&x, sizeof(uint8_t));
		o.index1 = x;
		break;
	}
	case 1:
	{
		uint16_t x;
		in.read((char*)&x, sizeof(uint16_t));
		o.index0 = x;
		in.read((char*)&x, sizeof(uint16_t));
		o.index1 = x;
		break;
	}
	case 2:
	{
		uint32_t x;
		in.read((char*)&x, sizeof(uint32_t));
		o.index0 = x;
		in.read((char*)&x, sizeof(uint32_t));
		o.index1 = x;
		break;
	}
	default:
		break;
	}
	in.read((char*)&o.weight, sizeof(float));
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, BDEF4& o)
{
	switch (SkeletonIndexType)
	{
	case 0:
	{
		uint8_t x[4];
		in.read((char*)&x, sizeof(uint8_t) * 4);
		o.index[0] = x[0];
		o.index[1] = x[1];
		o.index[2] = x[2];
		o.index[3] = x[3];
		break;
	}
	case 1:
	{
		uint16_t x[4];
		in.read((char*)&x, sizeof(uint16_t) * 4);
		o.index[0] = x[0];
		o.index[1] = x[1];
		o.index[2] = x[2];
		o.index[3] = x[3];
		break;
	}
	case 2:
	{
		uint32_t x[4];
		in.read((char*)&x, sizeof(uint32_t) * 4);
		o.index[0] = x[0];
		o.index[1] = x[1];
		o.index[2] = x[2];
		o.index[3] = x[3];
		break;
	}
	default:
		break;
	}
	in.read((char*)&o.weight, sizeof(float) * 4);
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, Vertex& o)
{
	in >> o.position >> o.normal >> o.tex;

	o.extraVec.resize(ExtraVectorNum);
	for (auto& iter : o.extraVec)
	{
		in >> iter;
	}

	in.read((char*)&o.weightType, sizeof(char));
	switch ((unsigned)o.weightType)
	{
	case 0:
	{
		BDEF1 x;
		in >> x;
		o.skeletonWeight = x;
		break;
	}
	case 1:
	{
		BDEF2 x;
		in >> x;
		o.skeletonWeight = x;
		break;
	}
	case 2:
	{
		BDEF4 x;
		in >> x;
		o.skeletonWeight = x;
		break;
	}
	default:
		throw - 1;
		break;
	}

	in.read((char*)&o.edgeMagnification, sizeof(float));

	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, Surface& o)
{
	switch (VertexIndexType)
	{
	case 0:
	{
		array<uint8_t, 3> x;
		in.read((char*)x.data(), sizeof(uint8_t) * 3);
		o.indices = std::move(x);
		break;
	}
	case 1:
	{
		array<uint16_t, 3> x;
		in.read((char*)x.data(), sizeof(uint16_t) * 3);
		o.indices = std::move(x);
		break;
	}
	case 2:
	{
		array<uint32_t, 3> x;
		in.read((char*)x.data(), sizeof(uint32_t) * 3);
		o.indices = std::move(x);
		break;
	}
	default:
		break;
	}
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, Texture& o)
{
	in >> o.file;
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, Material& o)
{
	in >> o.localName >> o.globalName >> o.diffuse >> o.specular;
	in.read((char*)&o.specularStrength, sizeof(float));
	in >> o.ambient;
	in.read(&o.drawFlag, sizeof(char));
	in >> o.edgeColor;
	in.read((char*)&o.edgeRatio, sizeof(float));
	switch (TextureIndexType)
	{
	case 0:
	{
		uint8_t x;
		in.read((char*)&x, sizeof(uint8_t));
		o.textureIndex = x;
		in.read((char*)&x, sizeof(uint8_t));
		o.textureExIndex = x;
		break;
	}
	case 1:
	{
		uint16_t x;
		in.read((char*)&x, sizeof(uint16_t));
		o.textureIndex = x;
		in.read((char*)&x, sizeof(uint16_t));
		o.textureExIndex = x;
		break;
	}
	case 2:
	{
		uint32_t x;
		in.read((char*)&x, sizeof(uint32_t));
		o.textureIndex = x;
		in.read((char*)&x, sizeof(uint32_t));
		o.textureExIndex = x;
		break;
	}
	default:
		break;
	}
	in.read(&o.mixMode, sizeof(char));
	in.read(&o.textureReference, sizeof(char));
	in.read(&o.textureEx, sizeof(char));
	in >> o.metadata;
	in.read((char*)&o.surfaceNum, sizeof(int32_t));
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, ParentSkeleton& o)
{
	switch (SkeletonIndexType)
	{
	case 0:
	{
		uint8_t x;
		in.read((char*)&x, sizeof(uint8_t));
		o.index = x;
		break;
	}
	case 1:
	{
		uint16_t x;
		in.read((char*)&x, sizeof(uint16_t));
		o.index = x;
		break;
	}
	case 2:
	{
		uint32_t x;
		in.read((char*)&x, sizeof(uint32_t));
		o.index = x;
		break;
	}
	default:
		break;
	}
	in.read((char*)&o.weight, sizeof(float));
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, SkeletonFixed& o)
{
	in >> o.axis;
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, SkeletonLocal& o)
{
	in >> o.xAxis >> o.zAxis;
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, Exoskeleton& o)
{
	switch (SkeletonIndexType)
	{
	case 0:
	{
		uint8_t x;
		in.read((char*)&x, sizeof(uint8_t));
		o.index = x;
		break;
	}
	case 1:
	{
		uint16_t x;
		in.read((char*)&x, sizeof(uint16_t));
		o.index = x;
		break;
	}
	case 2:
	{
		uint32_t x;
		in.read((char*)&x, sizeof(uint32_t));
		o.index = x;
		break;
	}
	default:
		break;
	}
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, AngleLimit& o)
{
	in >> o.min >> o.max;
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, IKLink& o)
{
	switch (SkeletonIndexType)
	{
	case 0:
	{
		uint8_t x;
		in.read((char*)&x, sizeof(uint8_t));
		o.index = x;
		break;
	}
	case 1:
	{
		uint16_t x;
		in.read((char*)&x, sizeof(uint16_t));
		o.index = x;
		break;
	}
	case 2:
	{
		uint32_t x;
		in.read((char*)&x, sizeof(uint32_t));
		o.index = x;
		break;
	}
	default:
		break;
	}
	in.read(&o.angleLimitFlag, sizeof(char));
	if (o.angleLimitFlag)
	{
		in >> o.angleLimit;
	}
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, IK& o)
{
	switch (SkeletonIndexType)
	{
	case 0:
	{
		uint8_t x;
		in.read((char*)&x, sizeof(uint8_t));
		o.index = x;
		break;
	}
	case 1:
	{
		uint16_t x;
		in.read((char*)&x, sizeof(uint16_t));
		o.index = x;
		break;
	}
	case 2:
	{
		uint32_t x;
		in.read((char*)&x, sizeof(uint32_t));
		o.index = x;
		break;
	}
	default:
		break;
	}
	in.read((char*)&o.loopTimes, sizeof(int));
	in.read((char*)&o.angleLimit, sizeof(float));
	in.read((char*)&o.linkNum, sizeof(int));
	if (o.linkNum)
	{
		for (auto i = 0u; i < o.linkNum; ++i)
		{
			IKLink x;
			in >> x;
			o.link.push_back(x);
		}

	}
	return in;
}

ifstream& d3dModel::operator>>(ifstream& in, Skeleton& o)
{
	in >> o.localName >> o.globalName >> o.position;
	switch (SkeletonIndexType)
	{
	case 0:
	{
		uint8_t x;
		in.read((char*)&x, sizeof(uint8_t));
		o.parentIndex = x;
		break;
	}
	case 1:
	{
		uint16_t x;
		in.read((char*)&x, sizeof(uint16_t));
		o.parentIndex = x;
		break;
	}
	case 2:
	{
		uint32_t x;
		in.read((char*)&x, sizeof(uint32_t));
		o.parentIndex = x;
		break;
	}
	default:
		break;
	}
	in.read((char*)&o.calculateLevel, sizeof(int));
	in.read((char*)&o.flag, sizeof(uint16_t));
	if (o.flag & 0x1)
	{
		switch (SkeletonIndexType)
		{
		case 0:
		{
			uint8_t x;
			in.read((char*)&x, sizeof(uint8_t));
			o.finalPosition = x;
			break;
		}
		case 1:
		{
			uint16_t x;
			in.read((char*)&x, sizeof(uint16_t));
			o.finalPosition = x;
			break;
		}
		case 2:
		{
			uint32_t x;
			in.read((char*)&x, sizeof(uint32_t));
			o.finalPosition = x;
			break;
		}
		default:
			break;
		}
	}
	else
	{
		Vector3 x;
		in >> x;
		o.finalPosition = x;
	}
	if (o.flag & 0x300)
	{
		in >> o.parentSkeleton;
	}
	if (o.flag & 0x400)
	{
		in >> o.skeletonFixed;
	}
	if (o.flag & 0x800)
	{
		in >> o.skeletonLocal;
	}
	if (o.flag & 0x200)
	{
		in >> o.exoSkeleton;
	}
	if (o.flag & 0x20)
	{
		in >> o.ik;
	}
	return in;
}

std::ifstream& d3dModel::operator>>(std::ifstream& fin, BoneKeyFrame& result)
{
	static const auto frameResourceSize(sizeof(uint32_t) + sizeof(float) * 7 + sizeof(uint8_t) * 64);
	char s[16];
	fin.read(s, sizeof(char) * 15);
	SHIFTJIS2GBK(s, result.name);
	fin.read((char*)&result.FrameResource, frameResourceSize);
	return fin;
}

DirectX::XMMATRIX d3dModel::GetTransformMatrix(BoneAnimation& start, BoneAnimation& end, float percent, std::pair<DirectX::XMVECTOR, DirectX::XMVECTOR>& datum)
{
	return XMLoadFloat4x4(&MathHelper::Identity4x4());
}
