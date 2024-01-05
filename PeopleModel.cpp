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
		x->diffuseSrvHeapIndex = static_cast<UnionIndex>(iter.textureIndex);
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

		mSkeleton.push_back(iter);
		mSkeletonWithIndex[iter.localName.s] = mSkeleton.size() - 1;

		if (iter.flag & 0x20)
		{
			mIKSkeleton.push_back(mSkeleton.size() - 1);
		}
		if (iter.flag & 0x300)
		{
			mInheritSkeleton.push_back(mSkeleton.size() - 1);
		}

		if (static_cast<UnionIndex>(iter.parentIndex) < 1000)
		{
			mSkeleton[static_cast<UnionIndex>(iter.parentIndex)].childIndex.push_back(mSkeleton.size() - 1);
		}
	}
	mSkeletonTransform.resize(skeletonNum);
	mSkeletonOriTransform.resize(skeletonNum);
	mSkeletonMotion.resize(skeletonNum);

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
	file.read((char*)&boneKeyFrameNumber, sizeof(uint32_t));
	if (boneKeyFrameNumber)
	{
		for (auto i = 0u; i < boneKeyFrameNumber; ++i)
		{
			BoneKeyFrame tempBoneKeyFrame;
			file >> tempBoneKeyFrame;
			if (mSkeletonWithIndex.find(tempBoneKeyFrame.name) == mSkeletonWithIndex.end())
				continue;
			mSkeletonMotion[mSkeletonWithIndex[tempBoneKeyFrame.name]].push_back(BoneAnimation(tempBoneKeyFrame.FrameResource.Translation, tempBoneKeyFrame.FrameResource.Rotation, tempBoneKeyFrame.FrameResource.FrameTime));
		}
	}
	for (auto& iter : mSkeletonMotion)
	{
		if (iter.empty())
			continue;
		sort(iter.begin(), iter.end(), [](const BoneAnimation& o1, const BoneAnimation& o2) { return o1.frameTime < o2.frameTime; });
	}

	file.close();
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
	if (frameTime < 10)
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
		mSkeletonTransform[0] = mSkeletonOriTransform[0] = MathHelper::Identity4x4();
	}
	else
	{
		auto& motion(mSkeletonMotion[0]);
		if (motion[0].frameTime >= motionTime)
		{
			auto temp(GetTransformMatrix(motion[0], motion[0], 0.0));
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[0],
				XMMatrixTranslation(mSkeleton[0].position.x, mSkeleton[0].position.y, mSkeleton[0].position.z) * temp);
			XMStoreFloat4x4(&mSkeletonOriTransform[0], temp);
		}
		else if (motion[motion.size() - 1].frameTime <= motionTime)
		{
			auto temp(GetTransformMatrix(motion[motion.size() - 1], motion[motion.size() - 1], 0.0));
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[0],
				XMMatrixTranslation(mSkeleton[0].position.x, mSkeleton[0].position.y, mSkeleton[0].position.z) * temp);
			XMStoreFloat4x4(&mSkeletonOriTransform[0], temp);
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
			auto temp(GetTransformMatrix(motion[startIndex], motion[endIndex], percent));
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[0], 
				XMMatrixTranslation(mSkeleton[0].position.x, mSkeleton[0].position.y, mSkeleton[0].position.z) * temp);
			XMStoreFloat4x4(&mSkeletonOriTransform[0], temp);
		}
	}
	for (auto i = 1u; i < mSkeletonTransform.size(); ++i)
	{
		auto& motion(mSkeletonMotion[i]);
		int parentIndex(static_cast<UnionIndex>(mSkeleton[i].parentIndex));

		XMMATRIX parentTrans, parentOffset;
		if (parentIndex == 65535)
		{
			parentTrans = XMLoadFloat4x4(&MathHelper::Identity4x4());
			parentOffset = XMMatrixTranslation(mSkeleton[i].position.x, mSkeleton[i].position.y, mSkeleton[i].position.z);
		}
		else
		{
			parentTrans = XMLoadFloat4x4(&mSkeletonTransform[parentIndex]);
			parentOffset = XMMatrixTranslation(
				mSkeleton[i].position.x - mSkeleton[parentIndex].position.x,
				mSkeleton[i].position.y - mSkeleton[parentIndex].position.y,
				mSkeleton[i].position.z - mSkeleton[parentIndex].position.z
			);
		}

		if (motion.empty())
		{
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[i],
				parentOffset * parentTrans);
			mSkeletonOriTransform[i] = MathHelper::Identity4x4();
			continue;
		}
		if (motion[0].frameTime >= motionTime)
		{
			auto temp(GetTransformMatrix(motion[0], motion[0], 0.0));
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[i],
				temp * parentOffset * parentTrans);
			XMStoreFloat4x4(&mSkeletonOriTransform[i], temp);
		}
		else if (motion[motion.size() - 1].frameTime <= motionTime)
		{
			auto temp(GetTransformMatrix(motion[motion.size() - 1], motion[motion.size() - 1], 0.0));
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[i],
				temp * parentOffset * parentTrans);
			XMStoreFloat4x4(&mSkeletonOriTransform[i], temp);
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
			auto temp(GetTransformMatrix(motion[startIndex], motion[endIndex], percent));
			DirectX::XMStoreFloat4x4(&mSkeletonTransform[i],
				temp * parentOffset * parentTrans);
			XMStoreFloat4x4(&mSkeletonOriTransform[i], temp);
		}
	}

	for (auto iter : mIKSkeleton)
	{
		auto& ik(mSkeleton[iter]);

		//int ikParentIndex(static_cast<UnionIndex>(mSkeleton[iter].parentIndex));
		Vector3 ppp(mSkeleton[static_cast<UnionIndex>(ik.ik.link[ik.ik.link.size() - 1].index)].position);
		XMFLOAT3 ikParentPos;
		XMStoreFloat3(&ikParentPos, XMVector3TransformCoord(XMVectorZero(), XMLoadFloat4x4(&mSkeletonTransform[static_cast<UnionIndex>(ik.ik.link[ik.ik.link.size() - 1].index)])));
		int finalSkeletonIndex(static_cast<UnionIndex>(ik.ik.index));

		vector<int> ikLinkIndex;
		vector<AngleLimit> ikLinkLimit;
		for (auto& i : ik.ik.link)
		{
			int tempIndex(static_cast<UnionIndex>(i.index));
			if (tempIndex == -1 || tempIndex > 1000)
				throw - 1;
			ikLinkIndex.push_back(tempIndex);
			ikLinkLimit.push_back(i.angleLimit);
		}

		vector<XMVECTOR> tempIKLinkLocalPos;
		for (auto& i : ikLinkIndex)
		{
			tempIKLinkLocalPos.push_back(
				XMVector3TransformCoord(XMVectorZero(), XMLoadFloat4x4(&mSkeletonTransform[i])/** XMMatrixTranslation(-ikParentPos.x, -ikParentPos.y, -ikParentPos.z)*/)
			);
		}
		vector<XMVECTOR> iiiiii(tempIKLinkLocalPos);
		auto tempFinalSkeletonLocalPos(XMVector3TransformCoord(
			XMVectorZero(),
			XMLoadFloat4x4(&mSkeletonTransform[finalSkeletonIndex])/**
			XMMatrixTranslation(-ikParentPos.x, -ikParentPos.y, -ikParentPos.z)*/
		)),
			tempTargetLocalPos(XMVector3TransformCoord(
				XMVectorZero(),
				XMLoadFloat4x4(&mSkeletonTransform[iter])/**
				XMMatrixTranslation(-ikParentPos.x, -ikParentPos.y, -ikParentPos.z)*/
			));

		auto tttt(tempFinalSkeletonLocalPos);

		vector<XMFLOAT3> finalPos;

		for (auto i = 0; i < ik.ik.loopTimes; ++i)
		{
			if (XMVectorGetX(XMVector3LengthSq(tempFinalSkeletonLocalPos - tempTargetLocalPos)) < 1e-6)
				break;
			for (auto j = 0u; j < tempIKLinkLocalPos.size(); ++j)
			{
				auto now(tempFinalSkeletonLocalPos - tempIKLinkLocalPos[j]),
					target(tempTargetLocalPos - tempIKLinkLocalPos[j]);
				auto axis(XMVector3Cross(now, target));
				if (XMVector3Equal(axis, XMVectorZero()))
					continue;
				auto angle(XMVectorGetX(XMVector3AngleBetweenVectors(now, target)));
				auto trans(XMMatrixRotationAxis(axis, angle));
				/*if (ik.ik.link[j].angleLimitFlag)
				{
					XMFLOAT4 q;
					XMStoreFloat4(&q, XMQuaternionRotationMatrix(trans));
					float Yaw, Pitch, Roll;

					double sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
					double cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
					Roll = std::atan2(sinr_cosp, cosr_cosp);

					double sinp = 2 * (q.w * q.y - q.z * q.x);
					if (std::abs(sinp) >= 1)
						Pitch = std::copysign(XM_PI / 2, sinp);
					else
						Pitch = std::asin(sinp);

					double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
					double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
					Yaw = std::atan2(siny_cosp, cosy_cosp);

					auto maxAngle(ik.ik.link[j].angleLimit.max),
						minAngle(ik.ik.link[j].angleLimit.min);

					auto clampRoll(clamp(Roll, minAngle.z, maxAngle.z)),
						clampPitch(clamp(Pitch, minAngle.x, maxAngle.x)),
						clampYaw(clamp(Yaw, minAngle.y, maxAngle.y));
					if (clampRoll != Roll || clampPitch != Pitch || clampYaw != Yaw)
						trans = XMMatrixRotationRollPitchYaw(clampPitch, clampYaw, clampRoll);
				}*/
				auto offset(XMMatrixTranslationFromVector(tempIKLinkLocalPos[j]));
				tempFinalSkeletonLocalPos =
					XMVector3TransformCoord(tempFinalSkeletonLocalPos, XMMatrixInverse(nullptr, offset) * trans * offset);
				XMFLOAT3 temptemp;
				XMStoreFloat3(&temptemp, tempFinalSkeletonLocalPos);
				finalPos.push_back(temptemp);
				for (auto p = 0u; p < j; ++p)
				{
					offset = XMMatrixTranslationFromVector(tempIKLinkLocalPos[p] - tempIKLinkLocalPos[j]);
					tempIKLinkLocalPos[p] = XMVector3TransformCoord(tempIKLinkLocalPos[p], XMMatrixInverse(nullptr, offset) * trans * offset);
				}
			}
		}

		vector<XMMATRIX> iitt;
		for (auto i = 0u; i < iiiiii.size(); ++i)
		{
			if (!i)
			{
				auto now(tempFinalSkeletonLocalPos - tempIKLinkLocalPos[i]),
					target(tttt - iiiiii[i]);
				auto axis(XMVector3Cross(now, target));
				if (XMVector3Equal(axis, XMVectorZero()))
				{
					iitt.push_back(XMMatrixIdentity());
					continue;
				}
				auto angle(XMVectorGetX(XMVector3AngleBetweenVectors(now, target)));
				if (angle < 1e-3)
				{
					iitt.push_back(XMMatrixIdentity());
					continue;
				}
				auto trans(XMMatrixRotationAxis(axis, angle));
				iitt.push_back(trans);
			}
			else
			{
				auto now(tempIKLinkLocalPos[i - 1] - tempIKLinkLocalPos[i]),
					target(iiiiii[i - 1] - iiiiii[i]);
				auto axis(XMVector3Cross(now, target));
				if (XMVector3Equal(axis, XMVectorZero()))
				{
					iitt.push_back(XMMatrixIdentity());
					continue;
				}
				auto angle(XMVectorGetX(XMVector3AngleBetweenVectors(now, target)));
				if (angle < 1e-3)
				{
					iitt.push_back(XMMatrixIdentity());
					continue;
				}
				auto trans(XMMatrixRotationAxis(axis, angle));
				iitt.push_back(trans);
			}
		}

		for (int i = ikLinkIndex.size() - 1; i >= 0; --i)
		{
			int thisIndex(ikLinkIndex[i]), parentIndex(static_cast<UnionIndex>(mSkeleton[thisIndex].parentIndex));

			XMMATRIX parentTrans, parentOffset;
			if (parentIndex == 65535)
			{
				parentTrans = XMLoadFloat4x4(&MathHelper::Identity4x4());
				parentOffset = XMMatrixTranslation(mSkeleton[thisIndex].position.x, mSkeleton[thisIndex].position.y, mSkeleton[thisIndex].position.z);
			}
			else
			{
				parentTrans = XMLoadFloat4x4(&mSkeletonTransform[parentIndex]);
				parentOffset = XMMatrixTranslation(
					mSkeleton[thisIndex].position.x - mSkeleton[parentIndex].position.x,
					mSkeleton[thisIndex].position.y - mSkeleton[parentIndex].position.y,
					mSkeleton[thisIndex].position.z - mSkeleton[parentIndex].position.z
				);
			}
			XMStoreFloat4x4(
				&mSkeletonTransform[thisIndex],
				XMLoadFloat4x4(&mSkeletonOriTransform[thisIndex])* iitt[i] * parentOffset* parentTrans
			);
			XMStoreFloat4x4(
				&mSkeletonOriTransform[thisIndex],
				XMLoadFloat4x4(&mSkeletonOriTransform[thisIndex])* iitt[i]
			);
		}

		auto childIndex(ikLinkIndex[0]);
		while (mSkeleton[childIndex].childIndex.size())
		{
			auto parentIndex(childIndex);
			childIndex = mSkeleton[childIndex].childIndex[0];
			XMMATRIX parentTrans(XMLoadFloat4x4(&mSkeletonTransform[parentIndex])),
				parentOffset(XMMatrixTranslation(
					mSkeleton[childIndex].position.x - mSkeleton[parentIndex].position.x,
					mSkeleton[childIndex].position.y - mSkeleton[parentIndex].position.y,
					mSkeleton[childIndex].position.z - mSkeleton[parentIndex].position.z
				));
			XMStoreFloat4x4(
				&mSkeletonTransform[childIndex],
				XMLoadFloat4x4(&mSkeletonOriTransform[childIndex])* parentOffset* parentTrans
			);
		}
	}

	for (auto iter : mInheritSkeleton)
	{
		int inheritIndex(static_cast<UnionIndex>(mSkeleton[iter].parentSkeleton.index)),
			inheritParentIndex(static_cast<UnionIndex>(mSkeleton[inheritIndex].parentIndex)),
			parentIndex(static_cast<UnionIndex>(mSkeleton[iter].parentIndex));
		XMFLOAT4X4 targetTrans(mSkeletonOriTransform[inheritIndex]);

		//XMStoreFloat4x4(
		//	&targetTrans,
		//	XMLoadFloat4x4(&mSkeletonTransform[inheritIndex]) *
		//	XMMatrixInverse(nullptr, XMLoadFloat4x4(&mSkeletonTransform[inheritParentIndex])) *
		//	XMMatrixInverse(nullptr, XMMatrixTranslation(
		//		mSkeleton[inheritIndex].position.x - mSkeleton[inheritParentIndex].position.x,
		//		mSkeleton[inheritIndex].position.y - mSkeleton[inheritParentIndex].position.y,
		//		mSkeleton[inheritIndex].position.z - mSkeleton[inheritParentIndex].position.z
		//	))
		//);

		if (!(mSkeleton[iter].flag & 0x100))
		{
			targetTrans._11 = targetTrans._22 = targetTrans._33 = 1.0;
			targetTrans._12 = targetTrans._13 = targetTrans._21 = targetTrans._23 = targetTrans._31 = targetTrans._32 = 0.0;
		}
		else if (!(mSkeleton[iter].flag & 0x200))
		{
			targetTrans._41 = targetTrans._42 = targetTrans._43 = 0.0;
		}

		XMStoreFloat4x4(
			&mSkeletonTransform[iter],
			XMLoadFloat4x4(&targetTrans) *
			XMMatrixTranslation(
				mSkeleton[iter].position.x - mSkeleton[parentIndex].position.x,
				mSkeleton[iter].position.y - mSkeleton[parentIndex].position.y,
				mSkeleton[iter].position.z - mSkeleton[parentIndex].position.z
			)*
			XMLoadFloat4x4(&mSkeletonTransform[parentIndex])
		);
		mSkeletonOriTransform[iter] = targetTrans;

		auto childIndex(iter);
		while (mSkeleton[childIndex].childIndex.size())
		{
			auto parentIndex(childIndex);
			childIndex = mSkeleton[childIndex].childIndex[0];
			XMMATRIX parentTrans(XMLoadFloat4x4(&mSkeletonTransform[parentIndex])),
				parentOffset(XMMatrixTranslation(
					mSkeleton[childIndex].position.x - mSkeleton[parentIndex].position.x,
					mSkeleton[childIndex].position.y - mSkeleton[parentIndex].position.y,
					mSkeleton[childIndex].position.z - mSkeleton[parentIndex].position.z
				));
			XMStoreFloat4x4(
				&mSkeletonTransform[childIndex],
				XMLoadFloat4x4(&mSkeletonOriTransform[childIndex])* parentOffset* parentTrans
			);
		}
	}

	for (auto i = 0u; i < mMeshSize; ++i)
	{
		d3dUtil::Vector2 temp(mMesh[i]);
		DirectX::XMFLOAT3 pos(0, 0, 0), normal(0, 0, 0);
		auto loadedPos(XMLoadFloat3(&pos)), loadedNormal(XMLoadFloat3(&normal));
		for (auto j = 0u; j < 4; ++j)
		{
			if (temp.boneWeight[j] == 0.0)
				break;
			XMVECTOR p;
			p = XMVector3TransformCoord(XMLoadFloat3(&temp.pos),
				XMMatrixTranslation(-mSkeleton[temp.boneIndices[j]].position.x, -mSkeleton[temp.boneIndices[j]].position.y, -mSkeleton[temp.boneIndices[j]].position.z)
			);
			loadedPos += temp.boneWeight[j] * XMVector3TransformCoord(p, XMLoadFloat4x4(&mSkeletonTransform[temp.boneIndices[j]]));
			loadedNormal += temp.boneWeight[j] * XMVector3TransformNormal(XMLoadFloat3(&temp.normal), XMLoadFloat4x4(&mSkeletonTransform[temp.boneIndices[j]]));
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
	else
	{
		o.angleLimit.max = { -1,-1,-1 };
		o.angleLimit.min = { -1,-1,-1 };
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
	fin.read((char*)&result.FrameResource, sizeof(result.FrameResource));/*
	result.FrameResource.Rotation[0] = -result.FrameResource.Rotation[0];
	auto temp(result.FrameResource.Rotation[1]);
	result.FrameResource.Rotation[1] = -result.FrameResource.Rotation[2];
	result.FrameResource.Rotation[2] = -temp;*/
	return fin;
}

DirectX::XMMATRIX d3dModel::GetTransformMatrix(BoneAnimation& start, BoneAnimation& end, float percent)
{
	XMVECTOR t0(XMLoadFloat3(&start.translation)),
		t1(XMLoadFloat3(&end.translation)),
		r0(XMLoadFloat4(&start.rotation)),
		r1(XMLoadFloat4(&end.rotation));

	auto t(XMVectorLerp(t0, t1, percent)),
		r(XMQuaternionSlerp(r0, r1, percent));

	auto tM(XMMatrixTranslationFromVector(t)),
		rM(XMMatrixRotationQuaternion(r));

	return rM * tM;
}

d3dModel::UnionIndex::operator int() const
{
	switch (this->x.index())
	{
	case 0:
		return get<uint8_t>(this->x);
	case 1:
		return get<uint16_t>(this->x);
	case 2:
		return get<uint32_t>(this->x);
	default:
		return -1;
	}
}
