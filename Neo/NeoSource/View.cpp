#include "Neo.h"
#include "View.h"

void View::SetOrthographic(const OrthographicInfo& info)
{
	m_isPerspective = false;
	m_orthographic = info;
}

void View::SetPerspective(const PerspectiveInfo& info)
{
	m_isPerspective = true;
	m_perspective = info;
}

void View::SetLookAt(const vec3& eye, const vec3& target, const vec3& up)
{
	m_cameraMatrix = LookAt(eye, target, up);
}

void View::SetViewport(const rect& viewport)
{
	m_viewport = viewport;
}

void View::SetDepthRange(float minDepth, float maxDepth)
{
	m_minDepth = minDepth;
	m_maxDepth = maxDepth;
}

void View::SetScissorRect(const rect& scissorRect)
{
	m_scissor = scissorRect;
}

struct vector3
{
	union
	{
		float x;
		float r;
	};
	union
	{
		float y;
		float g;
	};
	union
	{
		float z;
		float b;
	};
};

void View::Apply()
{
	auto& gil = GIL::Instance();
	mat4x4 projMat;
	if (m_isPerspective)
	{
		ivec2 screenSize = gil.GetFrameBufferSize();
		projMat = glm::perspective(m_perspective.fov, (m_viewport.size.x * screenSize.x) / (m_viewport.size.y * screenSize.y), m_perspective.nearPlane, m_perspective.farPlane);
		projMat[1][1] *= -1.0f;
	}
	else
	{
		projMat = OrthoProj(m_orthographic.orthoRect, m_orthographic.nearPlane, m_orthographic.farPlane);
	}

	mat4x4 viewMat = glm::inverse(m_cameraMatrix);
	gil.SetViewMatrices(viewMat, projMat);
	gil.SetViewport(m_viewport, m_minDepth, m_maxDepth);
	gil.SetScissor(m_scissor);
}
