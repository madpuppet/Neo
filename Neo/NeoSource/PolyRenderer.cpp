#include "Neo.h"
#include "PolyRenderer.h"

void PolyRenderer::BeginFrame()
{
}

void PolyRenderer::UseMaterial(Material* mat)
{
	Cmd cmd;
	cmd.cmdType = Cmd::Cmd_SetMaterial;
	cmd.cmdData = (u32)m_materials.size();
	m_cmds.emplace_back(cmd);
	m_materials.emplace_back(MaterialRef(mat));
}

void PolyRenderer::StartPrimitve(PrimType primType)
{
	m_primitiveType = primType;
	m_vertStart = (u32)m_verts.size();
	m_indiceStart = (u32)m_indices.size();
}

void PolyRenderer::AddVert(vec3 pos, vec2 uv, u32 col)
{
	m_verts.emplace_back( pos,uv,col );
}


