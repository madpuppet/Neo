#include "Neo.h"
#include "StaticMesh.h"
#include "Material.h"
#include "StringUtils.h"
#include "SHAD.h"
#include "ResourceLoadedManager.h"
#include <tiny_obj_loader.h>

#include <iostream>
#include <sstream>
#include <streambuf>

#define STATICMESH_VERSION 1

DECLARE_MODULE(StaticMeshFactory, NeoModuleInitPri_StaticMeshFactory, NeoModulePri_None, NeoModulePri_None);

StaticMesh::StaticMesh(const string& name) : Resource(name)
{
	AssetManager::Instance().DeliverAssetDataAsync(AssetType_StaticMesh, name, nullptr, [this](AssetData* data) { OnAssetDeliver(data); });
}

StaticMesh::~StaticMesh()
{
}

void StaticMesh::OnAssetDeliver(AssetData* data)
{
	Assert(data->type == AssetType_StaticMesh, "Bad Asset Type");
	m_assetData = dynamic_cast<StaticMeshAssetData*>(data);

	// create dependant resources
	vector<Resource*> dependantResources;
	m_assetData->material.Create(m_assetData->materialName);
	dependantResources.push_back(*m_assetData->material);

	// we need to wait for our dependant resources, like Shaders and Textures,  to load first before creating our platform data (which are pipeline states)
	// note that if they are already loaded, this will just trigger off the callback immediately
	ResourceLoadedManager::Instance().AddDependancyList(this, dependantResources, [this]() { m_platformData = StaticMeshPlatformData_Create(m_assetData); OnLoadComplete(); });
}

void StaticMesh::Reload()
{
}

StaticMeshFactory::StaticMeshFactory()
{
	auto ati = new AssetTypeInfo();
	ati->name = "StaticMesh";
	ati->assetExt = ".neomdl";
	ati->assetCreator = []() -> AssetData* { return new StaticMeshAssetData; };
	ati->sourceExt.push_back({ { ".obj" }, true });		// on of these src image files
	AssetManager::Instance().RegisterAssetType(AssetType_StaticMesh, ati);
}

StaticMesh* StaticMeshFactory::Create(const string& name)
{
	u64 hash = StringHash64(name);
	auto it = m_resources.find(hash);
	if (it == m_resources.end())
	{
		StaticMesh* resource = new StaticMesh(name);
		m_resources.insert(std::pair<u64, StaticMesh*>(hash, resource));

		return resource;
	}
	it->second->IncRef();
	return it->second;
}

void StaticMeshFactory::Destroy(StaticMesh* resource)
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


class NeoMaterialReader : public tinyobj::MaterialReader
{
public:
	NeoMaterialReader() {}
	virtual ~NeoMaterialReader() {}

	virtual bool operator()(const string& matId,
		std::vector<tinyobj::material_t>* materials,
		std::map<string, int>* matMap, string* warn,
		string* err)
	{
		tinyobj::material_t mat;
		mat.name = StringGetFilenameNoExt(matId);
		int pos = (int)materials->size();
		(*matMap)[mat.name] = pos;
		(*materials).push_back(mat);
		return true;
	}
};



bool StaticMeshAssetData::SrcFilesToAsset(vector<MemBlock> &srcFiles, AssetCreateParams* params)
{
	Assert(srcFiles.size() == 1, STR("Expected 1 src file for StaticMesh"));

	// Create a MemoryStream object using the memory block and its size
	MemoryStream memoryStream(srcFiles[0].Mem(), srcFiles[0].Size());

	// Create an istream object using the MemoryStream
	std::istream inputStream(&memoryStream);

	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	string warn, err;

	NeoMaterialReader matReader;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, &inputStream, &matReader)) 
	{
		Error(STR("Tiny Object unable to parse source file for StaticMesh: {}", name));
		return false;
	}

	hashtable<Vertex_p3f_t2f_c4b, u32> uniqueVertices{};

	for (const auto& shape : shapes) {
		for (const auto& index : shape.mesh.indices) {
			Vertex_p3f_t2f_c4b vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 2],
				-attrib.vertices[3 * index.vertex_index + 1]
			};

			vertex.texCoord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};

			vertex.color = vec4ToR8G8B8A8({ 1.0f, 1.0f, 1.0f, 1.0f });

			if (uniqueVertices.count(vertex) == 0) {
				uniqueVertices[vertex] = static_cast<uint32_t>(verts.size());
				verts.push_back(vertex);
			}

			indices.push_back(uniqueVertices[vertex]);
		}
	}

	Assert(materials.size() == 1, "Only support single material objs atm");
	materialName = materials[0].name;
	return true;
}

MemBlock StaticMeshAssetData::AssetToMemory()
{
	Serializer_BinaryWriteGrow stream;
	stream.WriteU16(AssetType_StaticMesh);
	stream.WriteU16(STATICMESH_VERSION);
	stream.WriteString(name);

	u32 vertSize = (u32)(verts.size() * sizeof(Vertex_p3f_t2f_c4b));
	stream.WriteU32(vertSize);
	stream.WriteMemory((u8*)verts.data(), vertSize);
	u32 indiceSize = (u32)(indices.size() * sizeof(u32));
	stream.WriteU32(indiceSize);
	stream.WriteMemory((u8*)indices.data(), indiceSize);
	stream.WriteString(materialName);

	return MemBlock::CloneMem(stream.DataStart(), stream.DataSize());
}

bool StaticMeshAssetData::MemoryToAsset(const MemBlock& block)
{
	Serializer_BinaryRead stream(block);
	type = (AssetType)stream.ReadU16();
	version = stream.ReadU16();
	name = stream.ReadString();

	if (type != AssetType_StaticMesh)
	{
		LOG(Mesh, STR("Rebuilding {} - bad type {} - expected {}", name, (int)type, (int)AssetType_StaticMesh));
		return false;
	}
	if (version != STATICMESH_VERSION)
	{
		LOG(Mesh, STR("Rebuilding {} - old version {} - expected {}", name, version, STATICMESH_VERSION));
		return false;
	}

	auto vertBlock = stream.ReadMemory();
	auto indiceBlock = stream.ReadMemory();
	materialName = stream.ReadString();

	verts.assign((Vertex_p3f_t2f_c4b*)vertBlock.Mem(), (Vertex_p3f_t2f_c4b*)(vertBlock.Mem() + vertBlock.Size()));
	vertBlock.ClearMemPtrs();

	indices.assign((u32*)indiceBlock.Mem(), (u32*)(indiceBlock.Mem() + indiceBlock.Size()));
	indiceBlock.ClearMemPtrs();

	return true;
}

