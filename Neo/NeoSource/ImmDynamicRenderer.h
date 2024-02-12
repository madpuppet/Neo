#pragma once

#include "Material.h"

// immediate dynamic renderer can be used by routines on the RenderThread to immediately draw dynamic primitives (triangles/lines)

#define IMMDYNREN_FRAMES 2
#define IMMDYNREN_MAXTRIANGLES 65536
#define IMMDYNREN_MAXVERTS (IMMDYNREN_MAXTRIANGLES*3)
#define IMMDYNREN_MAXINDICES (IMMDYNREN_MAXTRIANGLES*3)

class ImmDynamicRenderer : public Module<ImmDynamicRenderer>
{
	struct Vertex
	{
		vec3 pos;
		vec2 uv;
		u32 col;
	};

	// geometry buffers have vertex buffers
	array<NeoGeometryBuffer*, IMMDYNREN_FRAMES> m_geomBuffers;

	// leave them permanently mapped
	array<void*, IMMDYNREN_FRAMES> vertexBufferPtr;
	array<void*, IMMDYNREN_FRAMES> indexBufferPtr;

	// current primitive information
	PrimType m_primType = PrimType_Unknown;
	u32 m_vertStart = 0;
	u32 m_indexStart = 0;
	u32 m_currentFrame = 0;

	// current vert
	u32 m_nextVert = 0;
	u32 m_nextIndex = 0;

public:
	ImmDynamicRenderer();
	~ImmDynamicRenderer();

	void BeginFrame();
	void BeginRender();
	void UseMaterial(Material* mat);
	void StartPrimitive(PrimType primType);
	void AddVert(const vec3& pos, const vec2& uv, u32 col);
	void EndPrimitive();
	void EndRender();
	void EndFrame();
};
