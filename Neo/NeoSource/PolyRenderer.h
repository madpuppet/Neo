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
		u32 indiceStart = 0;
		u32 indiceCount = 0;
	};
	vector<Vertex> m_verts;
	vector<u32> m_indices;
	vector<MaterialRef> m_materials;
	vector<Cmd> m_cmds;

	PrimType m_primitiveType = PrimType_TriangleList;
	int m_currentFrame = 0;
	u32 m_vertStart = 0;
	u32 m_indiceStart = 0;

public:
	void BeginFrame();
	void UseMaterial(Material* mat);
	void StartPrimitve(PrimType primType);
	void AddVert(vec3 pos, vec2 uv, u32 col);
};

