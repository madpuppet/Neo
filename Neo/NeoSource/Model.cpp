#include "Neo.h"
#include "Model.h"
#include "Material.h"
#include "StringUtils.h"
#include "SHAD.h"
#include "ResourceLoadedManager.h"
#include <tiny_obj_loader.h>

#include <iostream>
#include <sstream>
#include <streambuf>

#define MODEL_VERSION 1

DECLARE_MODULE(ModelFactory, NeoModulePri_ModelFactory);

Model::Model(const string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_Model, name, nullptr, [this](AssetData* data) { OnAssetDeliver(data); });
}

Model::~Model()
{
}

void Model::OnAssetDeliver(AssetData* data)
{
	Assert(data->type == AssetType_Model, "Bad Asset Type");
	m_assetData = dynamic_cast<ModelAssetData*>(data);

	// create dependant resources
	vector<Resource*> dependantResources;
	m_assetData->material.Create(m_assetData->materialName);
	dependantResources.push_back(*m_assetData->material);

	// we need to wait for our dependant resources, like Shaders and Textures,  to load first before creating our platform data (which are pipeline states)
	// note that if they are already loaded, this will just trigger off the callback immediately
	ResourceLoadedManager::Instance().AddDependancyList(this, dependantResources, [this]() { m_platformData = ModelPlatformData_Create(m_assetData); OnLoadComplete(); });
}

void Model::Reload()
{
}

ModelFactory::ModelFactory()
{
	auto ati = new AssetTypeInfo();
	ati->m_assetCreateFromData = [](MemBlock memBlock) -> AssetData* { auto assetData = new ModelAssetData; assetData->MemoryToAsset(memBlock); return assetData; };
	ati->m_assetCreateFromSource = [](const vector<MemBlock>& srcBlocks, AssetCreateParams* params) -> AssetData* { return ModelAssetData::Create(srcBlocks, params);  };
	ati->m_assetExt = ".neomdl";
	ati->m_sourceExt.push_back({ { ".obj" }, true });		// on of these src image files
	AssetManager::Instance().RegisterAssetType(AssetType_Model, ati);
}

Model* ModelFactory::Create(const string& name)
{
	u64 hash = StringHash64(name);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		Model* resource = new Model(name);
		m_resources.insert(std::pair<u64, Model*>(hash, resource));

		return resource;
	}
	it->second->IncRef();
	return it->second;
}

void ModelFactory::Destroy(Model* resource)
{
	if (resource && resource->DecRef() == 0)
	{
		u64 hash = StringHash64(resource->GetName());
		m_resources.erase(hash);
		delete resource;
	}
}

class MemoryStream : public std::streambuf {
public:
	MemoryStream(u8* data, std::size_t size) {
		setg((char*)data, (char*)data, (char*)(data+size));
	}
};

AssetData* ModelAssetData::Create(vector<MemBlock> srcFiles, AssetCreateParams* params)
{
	auto asset = new ModelAssetData;
	Assert(srcFiles.size() == 1, STR("Expected 1 src file for model"));

	// Create a MemoryStream object using the memory block and its size
	MemoryStream memoryStream(srcFiles[0].Mem(), srcFiles[0].Size());

	// Create an istream object using the MemoryStream
	std::istream inputStream(&memoryStream);

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warn, err;

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &inputStream)) {
		throw std::runtime_error(warn + err);
	}

	hashtable<Vertex, u32> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};

			vertex.uv = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.col = { 1.0f, 1.0f, 1.0f };

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(asset->verts.size());
				asset->verts.push_back(vertex);
			}

			asset->indices.push_back(uniqueVertices[vertex]);
		}
	}

	asset->materialName = "my_mat";

	return asset;
}

MemBlock ModelAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(AssetType_Model);
	stream.WriteU16(MODEL_VERSION);
	stream.WriteString(name);

	u32 vertSize = (u32)(verts.size() * sizeof(Vertex));
	stream.WriteU32(vertSize);
	stream.WriteMemory((u8*)verts.data(), vertSize);
	u32 indiceSize = (u32)(indices.size() * sizeof(u32));
	stream.WriteU32(indiceSize);
	stream.WriteMemory((u8*)indices.data(), indiceSize);
	stream.WriteString(materialName);

	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool ModelAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);
	type = (AssetType)stream.ReadU16();
	version = stream.ReadU16();
	name = stream.ReadString();

	if (type != AssetType_Model)
	{
		Log(STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_Model));
		return false;
	}
	if (version != MODEL_VERSION)
	{
		Log(STR("Rebuilding {} - old version {} - expected {}", name, version, MODEL_VERSION));
		return false;
	}

	auto vertBlock = stream.ReadMemory();
	auto indiceBlock = stream.ReadMemory();
	materialName = stream.ReadString();

	verts.assign((Vertex*)vertBlock.Mem(), (Vertex*)(vertBlock.Mem() + vertBlock.Size()));
	vertBlock.ClearMemPtrs();

	indices.assign((u32*)indiceBlock.Mem(), (u32*)(indiceBlock.Mem() + vertBlock.Size()));
	indiceBlock.ClearMemPtrs();

	return true;
}