#include "Neo.h"
#include "View.h"
#include "RenderThread.h"
#include "ShaderManager.h"

View::View()
{
}

View::~View()
{

}

void View::SetOrthographic(const OrthographicInfo& info)
{
	m_orthographic = info;
}

void View::SetPerspective(const PerspectiveInfo& info)
{
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

void View::SetCameraMatrix(const mat4x4& camMatrix)
{
	m_cameraMatrix = camMatrix;
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
	UBO_View viewData;
	ivec2 screenSize = gil.GetFrameBufferSize();
	viewData.proj = glm::perspective(m_perspective.fov, (m_viewport.w * screenSize.x) / (m_viewport.h * screenSize.y), m_perspective.nearPlane, m_perspective.farPlane);
	viewData.proj[1][1] *= -1.0f;
	viewData.ortho = OrthoProj(m_orthographic.orthoRect, m_orthographic.nearPlane, m_orthographic.farPlane);
	viewData.view = glm::inverse(m_cameraMatrix);
	
	auto viewPD = ShaderManager::Instance().FindUBO("UBO_View")->dynamicInstance->platformData;
	gil.UpdateDynamicUBO(viewPD, &viewData, sizeof(viewData));

	gil.SetViewport(m_viewport, m_minDepth, m_maxDepth);
	gil.SetScissor(m_scissor);
}
