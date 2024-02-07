#pragma once

#include "Material.h"

class PolyRenderer
{
	struct Vertex
	{
		vec3 pos;
		vec2 uv;
		u32 col;
	};
	struct Cmd
	{
		enum CmdType { Cmd_None, Cmd_SetMaterial, Cmd_RenderPrimitive };
		u32 cmdType = Cmd_None;
		u32 cmdData = 0;		// material IDX or Primitive Type
		u32 vertStart = 0;
		u32 vertCount = 0;
		u32 indexStart = 0;
		u32 indexCount = 0;
	};
	vector<Vertex> m_verts;
	vector<u32> m_indices;
	vector<MaterialRef> m_materials[2];
	vector<Cmd> m_cmds[2];
	struct NeoGeometryBuffer* m_geomBuffer[4]{};

	PrimType m_primitiveType = PrimType_TriangleList;
	u32 m_vertStart = 0;
	u32 m_indexStart = 0;
	int m_currentFrame = 0;
	int m_currentGeomBuffer = 0;

public:
	// these functions used drawing the draw frame
	void BeginFrame();
	void UseMaterial(Material* mat);
	void StartPrimitive(PrimType primType);
	void AddVert(vec3 pos, vec2 uv, u32 col);
	void EndPrimitive();
	void EndFrame();

	// this function used when ready to actually render the cmd list on the RenderThread
	void Draw();
};

