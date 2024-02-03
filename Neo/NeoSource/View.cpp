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
	vec3 z = glm::normalize(target - eye);
	vec3 x = glm::cross(up, z);
	vec3 y = glm::cross(z, x);
	x = glm::normalize(x);
	y = glm::normalize(y);
	m_cameraMatrix[0] = vec4(x, 0.0f);
	m_cameraMatrix[1] = vec4(y, 0.0f);
	m_cameraMatrix[2] = vec4(z, 0.0f);
	m_cameraMatrix[3] = vec4(eye, 1.0f);
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
		projMat = glm::perspective(m_perspective.fov, m_viewport.size.x / m_viewport.size.y, m_perspective.nearPlane, m_perspective.farPlane);
		projMat[1][1] *= -1.0f;
	}
	else
	{
		projMat = OrthoProj(m_orthographic.orthoRect, m_orthographic.nearPlane, m_orthographic.farPlane);
	}

	mat4x4 viewMat = mat4x4(1);

	gil.SetViewMatrices(viewMat, projMat);
	gil.SetViewport(m_viewport, m_minDepth, m_maxDepth);
	gil.SetScissor(m_scissor);
}
