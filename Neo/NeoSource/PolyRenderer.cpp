#include "Neo.h"
#include "PolyRenderer.h"
#include "RenderThread.h"

void PolyRenderer::BeginFrame()
{
	m_verts.clear();
	m_indices.clear();
	m_cmds[m_currentFrame].clear();
	m_materials[m_currentFrame].clear();
}

void PolyRenderer::UseMaterial(Material* mat)
{
	Cmd cmd;
	cmd.cmdType = Cmd::Cmd_SetMaterial;
	cmd.cmdData = (u32)m_materials[m_currentFrame].size();
	m_cmds[m_currentFrame].emplace_back(cmd);
	m_materials[m_currentFrame].emplace_back(MaterialRef(mat));
}

void PolyRenderer::StartPrimitive(PrimType primType)
{
	m_primitiveType = primType;
	m_vertStart = (u32)m_verts.size();
	m_indexStart = (u32)m_indices.size();
}


void PolyRenderer::AddVert(vec3 pos, vec2 uv, u32 col)
{
	m_indices.emplace_back( (u32)m_verts.size());
	m_verts.emplace_back( pos,uv,col );
}

void PolyRenderer::EndPrimitive()
{
	Cmd cmd;
	cmd.cmdType = Cmd::Cmd_RenderPrimitive;
	cmd.cmdData = m_primitiveType;
	cmd.vertStart = m_vertStart;
	cmd.vertCount = (u32)m_verts.size() - m_vertStart;
	cmd.indexStart = m_indexStart;
	cmd.indexCount = (u32)m_indices.size() - m_indexStart;
	m_cmds[m_currentFrame].emplace_back(cmd);
}

void PolyRenderer::EndFrame()
{
	// create platform rendering data - will be ready for the Draw frame
	RenderThread::Instance().AddPreDrawTask
	(
		[this]()
		{
			m_currentGeomBuffer = (m_currentGeomBuffer + 1) % 2;
			GIL::Instance().DestroyGeometryBuffer(m_geomBuffer[m_currentGeomBuffer]);
			m_geomBuffer[m_currentGeomBuffer] = GIL::Instance().CreateGeometryBuffer(m_verts.data(), (u32)m_verts.size() * sizeof(Vertex), m_indices.data(), (u32)m_indices.size() * sizeof(u32));
			m_currentFrame = 1 - m_currentFrame;
		}
	);
}

void PolyRenderer::Draw()
{
	int frame = 1 - m_currentFrame;
	if (!m_cmds[frame].empty())
	{
		auto& gil = GIL::Instance();
		gil.BindGeometryBuffer(m_geomBuffer[m_currentGeomBuffer]);
		for (auto& cmd : m_cmds[frame])
		{
			switch (cmd.cmdType)
			{
				case Cmd::Cmd_SetMaterial:
					gil.BindMaterial(m_materials[frame][cmd.cmdData]);
					break;
				case Cmd::Cmd_RenderPrimitive:
					gil.RenderPrimitive((PrimType)cmd.cmdData, cmd.vertStart, cmd.vertCount, cmd.indexStart, cmd.indexCount);
					break;
			}
		}
	}
}

