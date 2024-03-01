#include "Neo.h"
#include "View.h"
#include "RenderThread.h"
#include "ShaderManager.h"
#include "RenderPass.h"

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

void View::SetCameraMatrix(const mat4x4& camMatrix)
{
	m_cameraMatrix = camMatrix;
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

void View::Apply(float aspectRatio)
{
	UBO_View viewData;

	viewData.proj = glm::perspective(m_perspective.fov, aspectRatio, m_perspective.nearPlane, m_perspective.farPlane);
	viewData.proj[1][1] *= -1.0f;
	viewData.ortho = OrthoProj(m_orthographic.orthoRect, m_orthographic.nearPlane, m_orthographic.farPlane);
	viewData.view = glm::inverse(m_cameraMatrix);
	
	auto viewUBOInstance = ShaderManager::Instance().FindUBO("UBO_View")->dynamicInstance;
	GIL::Instance().UpdateUBOInstance(viewUBOInstance, &viewData, sizeof(viewData), true);
}
