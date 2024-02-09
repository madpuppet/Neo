#pragma once

#include "Material.h"

#define DYNREN_FRAMES 3
#define DYNREN_MAXTRIANGLES 65536
#define DYNREN_MAXVERTS DYNREN_MAXTRIANGLES*3
#define DYNREN_MAXINDICES DYNREN_MAXTRIANGLES*3

class DynamicRenderer : public Module<DynamicRenderer>
{
	struct Vertex
	{
		vec3 pos;
		vec2 uv;
		u32 col;
	};
	enum CmdType { CmdType_None, CmdType_SetMaterial, CmdType_SetPrimType, CmdType_RenderPrimitive };
	struct Cmd
	{
		u32 cmdType = CmdType_None;
		u32 cmdData = 0;		// material IDX or Primitive Type
		u32 vertStart = 0;
		u32 vertCount = 0;
		u32 indexStart = 0;
		u32 indexCount = 0;
	};
	struct RenderBlock
	{
		u32 drawOrder;
		u32 cmdStart;
		u32 cmdCount;
	};
	vector<RenderBlock> m_renderBlocks[DYNREN_FRAMES];
	vector<Cmd> m_cmds[DYNREN_FRAMES];
	vector<MaterialRef> m_materials[DYNREN_FRAMES];

	// geometry buffers have vertex buffers
	NeoGeometryBuffer* m_geomBuffers[DYNREN_FRAMES]{};

	// leave them permanently mapped
	void* vertexBufferPtr[DYNREN_FRAMES]{};
	void* indexBufferPtr[DYNREN_FRAMES]{};

	Mutex m_renderLock;

	// current frame (for double buffered buffers / materials / cmds / renderBlocks)
	int m_updateFrame = 0;
	int m_drawFrame = 1;

	// current render block info
	u32 m_drawOrder = 0;
	u32 m_cmdStart = 0;

	// current primitive information
	PrimType m_primType = PrimType_Unknown;
	u32 m_vertStart = 0;
	u32 m_indexStart = 0;

	// current vert
	u32 m_nextVert = 0;
	u32 m_nextIndex = 0;

	// draw order being rendered
	u32 m_nextRenderBlock = 0;

public:
	DynamicRenderer();
	~DynamicRenderer();

	void BeginFrame();
	
	void BeginRender(u32 drawOrder);
	void UseMaterial(Material* mat);
	void StartPrimitive(PrimType primType);
	void AddVert(const vec3& pos, const vec2& uv, u32 col);
	void EndPrimitive();
	void EndRender();

	void EndFrame();

	void Render(u32 endDrawOrder);
};
