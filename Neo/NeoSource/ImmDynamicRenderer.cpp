#include "Neo.h"
#include "ImmDynamicRenderer.h"
#include "RenderThread.h"

DECLARE_MODULE(ImmDynamicRenderer, NeoModuleInitPri_ImmDynamicRenderer, NeoModulePri_Early, NeoModulePri_Early);

ImmDynamicRenderer::ImmDynamicRenderer()
{
	RenderThread::Instance().AddBeginFrameTask([this]() {BeginFrame(); });
	RenderThread::Instance().AddEndFrameTask([this]() {EndFrame(); });

	// this will execute before the first module update, so our buffers should be ready
	RenderThread::Instance().AddPreDrawTask(
		[this]()
		{
			for (int i = 0; i < 2; i++)
			{
				m_geomBuffers[i] = GIL::Instance().CreateGeometryBuffer(nullptr, IMMDYNREN_MAXVERTS * sizeof(Vertex_p3f_t2f_c4b), nullptr, IMMDYNREN_MAXINDICES * sizeof(u32));
				GIL::Instance().MapGeometryBufferMemory(m_geomBuffers[i], &(vertexBufferPtr[i]), &(indexBufferPtr[i]));
			}
		}
	);
}

ImmDynamicRenderer::~ImmDynamicRenderer()
{
}

void ImmDynamicRenderer::BeginFrame()
{
	m_nextVert = 0;
	m_nextIndex = 0;
	m_primType = PrimType_Unknown;
}

void ImmDynamicRenderer::BeginRender()
{
	auto& gil = GIL::Instance();
	mat4x4 model(1);
	gil.BindGeometryBuffer(m_geomBuffers[m_currentFrame]);
}

void ImmDynamicRenderer::UseMaterial(Material* mat)
{
	bool drawLines = (m_primType == PrimType_LineList || m_primType == PrimType_LineStrip);
	GIL::Instance().BindMaterial(mat, drawLines);
}

void ImmDynamicRenderer::StartPrimitive(PrimType primType)
{
	if (m_primType != primType)
	{
		GIL::Instance().SetRenderPrimitiveType(primType);
		m_primType = primType;
	}
	m_vertStart = m_nextVert;
	m_indexStart = m_nextIndex;
}

void ImmDynamicRenderer::AddVert(const vec3& pos, const vec2& texCoord, u32 color)
{
	if (m_nextVert < IMMDYNREN_MAXVERTS && m_nextIndex < IMMDYNREN_MAXINDICES)
	{
		auto vertMem = (Vertex_p3f_t2f_c4b*)vertexBufferPtr[m_currentFrame] + m_nextVert;
		vertMem->pos = pos;
		vertMem->texCoord = texCoord;
		vertMem->color = color;

		if (m_primType == PrimType_LineList || m_primType == PrimType_TriangleList)
		{
			u32* indexMem = (u32*)indexBufferPtr[m_currentFrame] + m_nextIndex;
			*indexMem = m_nextVert;
			m_nextIndex++;
		}
		m_nextVert++;
	}
}

void ImmDynamicRenderer::EndPrimitive()
{
	u32 vertCount = m_nextVert - m_vertStart;
	u32 indexCount = m_nextIndex - m_indexStart;
	if (vertCount > 0 || indexCount > 0)
	{
		GIL::Instance().RenderPrimitive(m_vertStart, vertCount, m_indexStart, indexCount);
	}
}

void ImmDynamicRenderer::EndRender()
{
}

void ImmDynamicRenderer::EndFrame()
{
	int vertDataSize = m_nextVert * sizeof(Vertex_p3f_t2f_c4b);
	int indexDataSize = m_nextIndex * sizeof(u32);
	GIL::Instance().FlushGeometryBufferMemory(m_geomBuffers[m_currentFrame], vertDataSize, indexDataSize);
	m_currentFrame = (m_currentFrame + 1) % IMMDYNREN_FRAMES;
}
