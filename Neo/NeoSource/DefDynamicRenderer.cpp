#include "Neo.h"
#include "DefDynamicRenderer.h"
#include "RenderThread.h"

DECLARE_MODULE(DefDynamicRenderer, NeoModuleInitPri_DefDynamicRenderer, NeoModulePri_Early, NeoModulePri_Early);

DefDynamicRenderer::DefDynamicRenderer()
{
	// this will execute before the first module update, so our buffers should be ready
	RenderThread::Instance().AddPreDrawTask(
		[this]()
		{
			for (int i = 0; i < DYNREN_FRAMES; i++)
			{
				m_geomBuffers[i] = GIL::Instance().CreateGeometryBuffer(nullptr, DYNREN_MAXVERTS * sizeof(Vertex), nullptr, DYNREN_MAXINDICES * sizeof(u32));
				GIL::Instance().MapGeometryBufferMemory(m_geomBuffers[i], &(vertexBufferPtr[i]), &(indexBufferPtr[i]));
			}
		}
	);
}

DefDynamicRenderer::~DefDynamicRenderer()
{
	// destroy the buffers?  or just leak coz this is end of program..
}

void DefDynamicRenderer::BeginFrame()
{
	m_nextVert = 0;
	m_nextIndex = 0;

	m_renderBlocks[m_updateFrame].clear();
	m_cmds[m_updateFrame].clear();
	m_materials[m_updateFrame].clear();
}

void DefDynamicRenderer::BeginRender(u32 drawOrder)
{
	// start a new render block - only one thread can add to render blocks at a time
	m_renderLock.Lock();
	m_cmdStart = (u32)m_cmds[m_updateFrame].size();
	m_drawOrder = drawOrder;
	m_primType = PrimType_Unknown;
}

void DefDynamicRenderer::UseMaterial(Material* mat)
{
	Cmd cmd;
	cmd.cmdType = CmdType_SetMaterial;
	cmd.cmdData = (u32)m_materials[m_updateFrame].size();
	m_cmds[m_updateFrame].emplace_back(cmd);
	m_materials[m_updateFrame].emplace_back(MaterialRef(mat));
}

void DefDynamicRenderer::StartPrimitive(PrimType primType)
{
	if (m_primType != primType)
	{
		Cmd cmd;
		cmd.cmdType = CmdType_SetPrimType;
		cmd.cmdData = (u32)primType;
		m_cmds[m_updateFrame].emplace_back(cmd);
		m_primType = primType;
	}
	m_vertStart = m_nextVert;
	m_indexStart = m_nextIndex;
}

void DefDynamicRenderer::AddVert(const vec3& pos, const vec2& uv, u32 col)
{
	if (m_nextVert < DYNREN_MAXVERTS && m_nextIndex < DYNREN_MAXINDICES)
	{
		Vertex* vertMem = (Vertex*)vertexBufferPtr[m_updateFrame] + m_nextVert;
		vertMem->pos = pos;
		vertMem->uv = uv;
		vertMem->col = col;

		if (m_primType == PrimType_LineList || m_primType == PrimType_TriangleList)
		{
			u32* indexMem = (u32*)indexBufferPtr[m_updateFrame] + m_nextIndex;
			*indexMem = m_nextVert;
			m_nextIndex++;
		}
		m_nextVert++;
	}
}

void DefDynamicRenderer::EndPrimitive()
{
	u32 vertCount = m_nextVert - m_vertStart;
	u32 indexCount = m_nextIndex - m_indexStart;

	if (vertCount > 0)
	{
		Cmd cmd;
		cmd.cmdType = CmdType_RenderPrimitive;
		cmd.vertStart = m_vertStart;
		cmd.vertCount = vertCount;
		cmd.indexStart = m_indexStart;
		cmd.indexCount = indexCount;
		m_cmds[m_updateFrame].emplace_back(cmd);
	}
}

void DefDynamicRenderer::EndRender()
{
	RenderBlock block;
	block.drawOrder = m_drawOrder;
	block.cmdStart = m_cmdStart;
	block.cmdCount = (u32)m_cmds[m_updateFrame].size() - m_cmdStart;
	m_renderBlocks[m_updateFrame].emplace_back(block);
	m_renderLock.Release();
}

void DefDynamicRenderer::EndFrame()
{
	u32 useFrame = m_updateFrame;
	m_updateFrame = (m_updateFrame + 1) % DYNREN_FRAMES;
	int vertDataSize = m_nextVert * sizeof(Vertex);
	int indexDataSize = m_nextIndex * sizeof(u32);

	// create platform rendering data - will be ready for the Draw frame
	RenderThread::Instance().AddPreDrawTask
	(
		[this, useFrame, vertDataSize, indexDataSize]()
		{
			m_drawFrame = useFrame;
			GIL::Instance().FlushGeometryBufferMemory(m_geomBuffers[m_drawFrame], vertDataSize, indexDataSize);
			std::sort(m_renderBlocks[m_drawFrame].begin(), m_renderBlocks[m_drawFrame].end(),
				[](const RenderBlock& a, const RenderBlock& b) { return a.drawOrder < b.drawOrder; });
			m_nextRenderBlock = 0;
		}
	);
}

void DefDynamicRenderer::Render(u32 endDrawOrder)
{
	auto& gil = GIL::Instance();
	auto& rb = m_renderBlocks[m_drawFrame];

	mat4x4 model(1);
	gil.BindGeometryBuffer(m_geomBuffers[m_drawFrame]);
	gil.SetModelMatrix(model);

	bool lines = false;
	while (m_nextRenderBlock < rb.size() && rb[m_nextRenderBlock].drawOrder < endDrawOrder)
	{
		auto& block = rb[m_nextRenderBlock++];
		for (u32 c = 0; c < block.cmdCount; c++)
		{
			auto& cmd = m_cmds[m_drawFrame][c + block.cmdStart];
			switch (cmd.cmdType)
			{
				case CmdType_SetMaterial:
					gil.BindMaterial(m_materials[m_drawFrame][cmd.cmdData], lines);
					break;
				case CmdType_SetPrimType:
					gil.SetRenderPrimitiveType((PrimType)cmd.cmdData);
					lines = (cmd.cmdData == PrimType_LineList || cmd.cmdData == PrimType_LineStrip);
					break;
				case CmdType_RenderPrimitive:
					gil.RenderPrimitive(cmd.vertStart, cmd.vertCount, cmd.indexStart, cmd.indexCount);
					break;
			}
		}

	}
}



