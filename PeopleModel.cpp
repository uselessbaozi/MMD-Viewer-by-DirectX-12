#include "pch.h"
#include "PeopleModel.h"
using namespace fbxsdk;
using namespace DirectX;

d3dModel::PeopleModel::PeopleModel()
{
}

d3dModel::PeopleModel::~PeopleModel()
{
}

void d3dModel::PeopleModel::Initialize(const char* fileName)
{
	mFileName = fileName;

	FbxManager* fbxManager(FbxManager::Create());
	FbxIOSettings* fbxIOSetting(FbxIOSettings::Create(fbxManager, IOSROOT));
	fbxManager->SetIOSettings(fbxIOSetting);

	FbxImporter* fbxImporter(FbxImporter::Create(fbxManager, ""));
	if (!fbxImporter->Initialize(mFileName.c_str(), -1, fbxManager->GetIOSettings()))
	{
		throw - 1;
	}

	FbxScene* fbxScene = FbxScene::Create(fbxManager, "1");

	fbxImporter->Import(fbxScene);

	fbxImporter->Destroy();

	auto fbxRootNode(fbxScene->GetRootNode());
	if (fbxRootNode)
	{
		for (auto i = 0u; i < fbxRootNode->GetChildCount(); ++i)
		{
			ProcessNode(fbxRootNode->GetChild(i));
		}
	}

	fbxManager->Destroy();
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
	int i(0);
	for (auto& iter : mMaterial)
	{
		iter->name = mMaterialByIndex[i];
		iter->matCBIndex += matSize;
		iter->diffuseSrvHeapIndex = tex[mTextureName[i]]->offset;
		mat[iter->name] = std::move(iter);
		++i;
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

void d3dModel::PeopleModel::ProcessNode(fbxsdk::FbxNode* node)
{
	if (!node)
		return;

	if (node->GetNodeAttribute())
	{
		switch (node->GetNodeAttribute()->GetAttributeType())
		{
		case FbxNodeAttribute::eMesh:
			ProcessMesh(node);
			break;
		case FbxNodeAttribute::eSkeleton:
			ProcessSkeleton(node);
			break;
		default:
			break;
		}
	}

	for (auto i = 0u; i < node->GetChildCount(); ++i)
	{
		ProcessNode(node->GetChild(i));
	}
}

void d3dModel::PeopleModel::ProcessMesh(fbxsdk::FbxNode* node)
{
	auto fbxMesh(node->GetMesh());
	if (!fbxMesh)
		return;

	auto vertexCount(fbxMesh->GetControlPointsCount());
	std::vector<d3dUtil::Vector2> tempVertices(vertexCount);
	std::vector<bool> tempVerticesVisited(vertexCount, false);
	auto indexCount(fbxMesh->GetPolygonCount());
	std::map<int, std::vector<uint32_t>> indexWithMaterial;
	auto meshMaterial(fbxMesh->GetElementMaterial());
	uint32_t tempPolygonIndices[3]{};
	for (auto i = 0u; i < indexCount; ++i)
	{
		auto polygonType(fbxMesh->GetPolygonSize(i));
		if (polygonType != 3)
			throw - 1;
		tempPolygonIndices[0] = (fbxMesh->GetPolygonVertex(i, 0));
		tempPolygonIndices[1] = (fbxMesh->GetPolygonVertex(i, 1));
		tempPolygonIndices[2] = (fbxMesh->GetPolygonVertex(i, 2));

		auto controlPoint(fbxMesh->GetControlPoints());
		auto pointNormal(fbxMesh->GetElementNormal());
		auto pointTex(fbxMesh->GetElementUV());
		auto polygonMaterial(fbxMesh->GetElementMaterial());

		for (auto j = 0u; j < 3; ++j)
		{
			if (tempVerticesVisited[tempPolygonIndices[j]])
				continue;
			tempVerticesVisited[tempPolygonIndices[j]] = true;
			tempVertices[tempPolygonIndices[j]].pos = fbxV4xm3(controlPoint[tempPolygonIndices[j]]);

			FbxVector4 normal/*(pointNormal->GetDirectArray().GetAt(tempPolygonIndices[j]))*/;
			auto normalIndex(fbxMesh->GetPolygonVertexNormal(i, j, normal));
			tempVertices[tempPolygonIndices[j]].normal = fbxV4xm3(normal);

			auto texIndex(fbxMesh->GetTextureUVIndex(i, j));
			FbxVector2 tex(pointTex->GetDirectArray().GetAt(texIndex));
			tempVertices[tempPolygonIndices[j]].texC = fbxV2xm2(tex);
		}

		auto materialIndex(meshMaterial->GetIndexArray().GetAt(i));
		if (indexWithMaterial.find(materialIndex) == indexWithMaterial.end())
		{
			indexWithMaterial[materialIndex] = std::vector<uint32_t>();
		}
		indexWithMaterial[materialIndex].push_back(tempPolygonIndices[0]);
		indexWithMaterial[materialIndex].push_back(tempPolygonIndices[1]);
		indexWithMaterial[materialIndex].push_back(tempPolygonIndices[2]);
	}

	mMeshSize = tempVertices.size();
	mMesh.insert(mMesh.end(), tempVertices.begin(), tempVertices.end());
	mIndexSize = 0;
	for (auto i = 0u; i < indexWithMaterial.size(); ++i)
	{
		mIndexOffset.push_back(mIndexSize);
		mIndexSize += indexWithMaterial[i].size();
		mIndex.insert(mIndex.end(), indexWithMaterial[i].begin(), indexWithMaterial[i].end());
	}

	auto materialCount(node->GetMaterialCount());
	static auto phongId(FbxSurfacePhong::ClassId);
	static auto lambertId(FbxSurfaceLambert::ClassId);
	std::vector<void*> data;
	for (auto i = 0u; i < materialCount; ++i)
	{
		auto tempMaterial(std::make_unique<d3dUtil::Material>());
		if (node->GetMaterial(i)->GetClassId().Is(phongId))
		{
			auto iter((fbxsdk::FbxSurfacePhong*)(node->GetMaterial(i)));
			tempMaterial->matCBIndex = i;
			tempMaterial->diffuseSrvHeapIndex = 0;
			tempMaterial->diffuseAlbedo = fbxD3xm4(iter->Diffuse);
			tempMaterial->diffuseAlbedo.w = iter->DiffuseFactor;
			//tempMaterial->diffuseAlbedo = { 1.0,1.0,1.0,1.0 };
			//tempMaterial->fresnelR0 = { 0.0,0.0,0.0 };
			tempMaterial->roughness = iter->BumpFactor;
		}
		else if (node->GetMaterial(i)->GetClassId().Is(lambertId))
		{
			auto iter((fbxsdk::FbxSurfaceLambert*)(node->GetMaterial(i)));
			tempMaterial->matCBIndex = i;
			tempMaterial->diffuseSrvHeapIndex = 0;
			//tempMaterial->diffuseAlbedo = fbxD3xm4(iter->Diffuse);
			//tempMaterial->diffuseAlbedo.w = iter->DiffuseFactor;
			tempMaterial->diffuseAlbedo = { 1.0,1.0,1.0,1.0 };
			tempMaterial->fresnelR0 = { 0.0,0.0,0.0 };
			tempMaterial->roughness = iter->BumpFactor;
		}
		else
			throw - 1;

		mMaterial.push_back(std::move(tempMaterial));
		char* materialName;
		FbxUTF8ToAnsi(node->GetMaterial(i)->GetName(), materialName);
		mMaterialByIndex.push_back(materialName);
		delete[] materialName;

		char* textureName;
		auto property(node->GetMaterial(i)->FindProperty(node->GetMaterial(i)->sDiffuse));
		if (property.GetSrcObjectCount() != 1)
			throw - 1;
		auto tex((FbxFileTexture*)property.GetSrcObject(0));
		FbxUTF8ToAnsi(tex->GetRelativeFileName(), textureName);
		std::string stringName(textureName);
		auto index(stringName.find('.'));
		mTextureName.push_back(stringName.substr(0, index));
		delete[] textureName;
		continue;
	}

	auto controlPointCount(fbxMesh->GetControlPointsCount());
	auto deformerCount(fbxMesh->GetDeformerCount());

	for (auto i = 0u; i < deformerCount; ++i)
	{
		auto fbxDeformer(fbxMesh->GetDeformer(i));
		if (!fbxDeformer || fbxDeformer->GetDeformerType() != FbxDeformer::eSkin)
			continue;
		auto fbxSkin((FbxSkin*)fbxDeformer);

		auto clusterCount(fbxSkin->GetClusterCount());
		for (auto j = 0u; j < clusterCount; j++)
		{
			auto fbxCluster(fbxSkin->GetCluster(j));
			if (!fbxCluster)
				continue;

			auto linkNode(fbxCluster->GetLink());
			char* linkName;
			FbxUTF8ToAnsi(linkNode->GetName(), linkName);
			std::string skeletonName(linkName);
			delete[] linkName;

			fbxsdk::FbxAMatrix transformMatrix, transformLinkMatrix;
			fbxCluster->GetTransformMatrix(transformMatrix);
			fbxCluster->GetTransformLinkMatrix(transformLinkMatrix);
			DirectX::XMStoreFloat4x4(
				&mSkeletonTransformLink[mSkeletonWithIndex[skeletonName]],
				DirectX::XMLoadFloat4x4(&fbxMAxmM(transformLinkMatrix))
			);

			auto controlPointIndicesCount(fbxCluster->GetControlPointIndicesCount());
			auto controlPointIndices(fbxCluster->GetControlPointIndices());
			auto controlPointWeight(fbxCluster->GetControlPointWeights());
			for (auto p = 0u; p < controlPointIndicesCount; ++p)
			{
				auto& meshPoint(mMesh[controlPointIndices[p]]);
				for (auto q = 0u; q < 4; q++)
				{
					if (meshPoint.boneWeight[q] == 0.0)
					{
						meshPoint.boneWeight[q] = (float)controlPointWeight[p];
						auto iii(mSkeletonWithIndex[skeletonName]);
						meshPoint.boneIndices[q] = mSkeletonWithIndex[skeletonName];
						break;
					}
				}
			}
		}
	}
}

void d3dModel::PeopleModel::ProcessSkeleton(fbxsdk::FbxNode* node)
{
	static bool notFirstTime(false);
	if (!node)
		return;
	if (!((FbxSkeleton*)node)->IsSkeletonRoot() || notFirstTime)
		return;

	for (auto i = 0u; i < node->GetChildCount(); ++i)
	{
		ProcessSkeletonHelper(node->GetChild(i), -1);
	}

	auto size(mSkeleton.size());
	mSkeletonTransform.resize(size, MathHelper::Identity4x4());
	mSkeletonTransformLink.resize(size, MathHelper::Identity4x4());
	mSkeletonMotion.resize(size);
	notFirstTime = !notFirstTime;
}

void d3dModel::PeopleModel::ProcessSkeletonHelper(fbxsdk::FbxNode* node, int parentIndex)
{
	int thisIndex(-1);
	if (node->GetNodeAttribute() && node->GetNodeAttribute()->GetAttributeType() && node->GetNodeAttribute()->GetAttributeType() == FbxNodeAttribute::eSkeleton)
	{
		char* s;
		FbxUTF8ToAnsi(node->GetName(), s);
		mSkeleton.push_back(std::make_pair(s, parentIndex));
		thisIndex = mSkeleton.size() - 1;
		mSkeletonWithIndex[s] = thisIndex;
		delete[] s;

		DirectX::XMVECTOR startPoint, endPoint;
		//auto boneGlobalTrans(node->EvaluateLocalTransform(0));
		if (parentIndex != -1)
		{
			auto local(node->LclTranslation.Get());
			startPoint = XMLoadFloat3(&fbxD3xm3(local));
			auto rotation(node->LclRotation.Get());
			XMFLOAT3 xmRotation(rotation[1], rotation[0], rotation[2]);
			endPoint = XMQuaternionRotationRollPitchYawFromVector(XMLoadFloat3(&xmRotation));
			startPoint += mSkeletonPoint[parentIndex].first;
			endPoint = XMQuaternionMultiply(endPoint, mSkeletonPoint[parentIndex].second);
		}
		else
		{
			XMFLOAT4 temp(0.0, 0.0, 0.0, 1.0);
			startPoint = endPoint = XMLoadFloat4(&temp);
		}
		mSkeletonPoint.push_back(std::make_pair(startPoint, endPoint));
	}
	for (auto i = 0u; i < node->GetChildCount(); ++i)
	{
		ProcessSkeletonHelper(node->GetChild(i), thisIndex);
	}
}

DirectX::XMFLOAT3 d3dModel::fbxV4xm3(const fbxsdk::FbxVector4& o)
{
	return DirectX::XMFLOAT3(o[0], o[1], o[2]);
}

DirectX::XMFLOAT3 d3dModel::fbxV4xm3forPos(const fbxsdk::FbxVector4& o)
{
	return DirectX::XMFLOAT3(-o[0], o[2], o[1]);
}

DirectX::XMFLOAT2 d3dModel::fbxV2xm2(const fbxsdk::FbxVector2& o)
{
	return DirectX::XMFLOAT2(o[0], -o[1]);
}

DirectX::XMFLOAT4 d3dModel::fbxV4xm4forPos(const fbxsdk::FbxVector4& o)
{
	return DirectX::XMFLOAT4(-o[0], o[2], o[1], 1.0);
}

fbxsdk::FbxVector4 d3dModel::xm4fbxV4(const DirectX::XMFLOAT4& o)
{
	return fbxsdk::FbxVector4(-o.x, o.z, o.y, o.w);
}

DirectX::XMFLOAT4 d3dModel::fbxD3xm4(const fbxsdk::FbxDouble3& o)
{
	return DirectX::XMFLOAT4(o[0], o[1], o[2], 1.0f);
}

DirectX::XMFLOAT3 d3dModel::fbxD3xm3(const fbxsdk::FbxDouble3& o)
{
	return DirectX::XMFLOAT3(o[0], o[1], o[2]);
}

DirectX::XMFLOAT4 d3dModel::fbxD4xm4(const fbxsdk::FbxDouble4& o)
{
	return DirectX::XMFLOAT4(o[0], o[1], o[2], o[3]);
}

DirectX::XMFLOAT4X4 d3dModel::fbxMAxmM(const fbxsdk::FbxAMatrix& o)
{
	/*auto T(DirectX::XMLoadFloat4(&fbxD4xm4(o.GetT())));
	auto R(DirectX::XMLoadFloat4(&fbxD4xm4(o.GetR())));
	auto S(DirectX::XMLoadFloat4(&fbxD4xm4(o.GetS())));
	S = DirectX::operator*(S, 0.01);

	DirectX::XMFLOAT4X4 result;
	DirectX::XMStoreFloat4x4(
		&result,
		DirectX::XMMatrixTranslationFromVector(T) *
		DirectX::XMMatrixScalingFromVector(S) *
		DirectX::XMMatrixReflect(R)
	);
	return result;*/

	return DirectX::XMFLOAT4X4(
		o[0][0], o[0][1], o[0][2], o[0][3],
		o[1][0], o[1][1], o[1][2], o[1][3],
		o[2][0], o[2][1], o[2][2], o[2][3],
		o[3][0], o[3][1], o[3][2], o[3][3]
		);
}

std::ifstream& d3dModel::operator>>(std::ifstream& fin, BoneKeyFrame& result)
{
	static const auto frameResourceSize(sizeof(uint32_t) + sizeof(float) * 7 + sizeof(uint8_t) * 64);
	char s[16];
	fin.read(s, sizeof(char) * 15);
	result.name = s;
	fin.read((char*)&result.FrameResource, frameResourceSize);
	return fin;
}

DirectX::XMMATRIX d3dModel::GetTransformMatrix(BoneAnimation& start, BoneAnimation& end, float percent, std::pair<DirectX::XMVECTOR, DirectX::XMVECTOR> & datum)
{
	using namespace DirectX;
	//return XMLoadFloat4x4(&MathHelper::Identity4x4());

	XMVECTOR t0(XMLoadFloat3(&start.translation)),
		t1(XMLoadFloat3(&end.translation)),
		r0(XMLoadFloat4(&start.rotation)),
		r1(XMLoadFloat4(&end.rotation));

	auto t(XMVectorLerp(t0, t1, percent)),
		r(XMQuaternionSlerp(r0, r1, percent));

	auto tM(XMMatrixTranslationFromVector(t)),
		rM(XMMatrixRotationQuaternion(r));

	//auto p0t(XMLoadFloat4x4(&XMFLOAT4X4(
	//	-1.0, 0.0, 0.0, 0.0,
	//	0.0, 0.0, 1.0, 0.0,
	//	0.0, -1.0, 0.0, 0.0,
	//	datum.x, datum.z, -datum.y, 1.0
	//))),
	//	p1t(XMLoadFloat4x4(&XMFLOAT4X4(
	//		-1.0, 0.0, 0.0, 0.0,
	//		0.0, 0.0, -1.0, 0.0,
	//		0.0, 1.0, 0.0, 0.0,
	//		datum.x, datum.y, datum.z, 1.0
	//	)));

	auto p1r(XMMatrixRotationQuaternion(datum.second) * XMMatrixTranslationFromVector(datum.first));
	auto p0r(XMMatrixInverse(nullptr, p1r));

	return p0r * rM * tM * p1r;
}
