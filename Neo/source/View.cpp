#include "Neo.h"
#include "View.h"
#include "RenderThread.h"
#include "ShaderManager.h"
#include "RenderPass.h"

View::View()
{
	m_beginUpdateHandle = NeoAddBeginUpdateTask
	(
		[this]()
		{
			// copy over last frames data
			if (!m_firstUpdate)
				m_viewData[NeoUpdateFrameIdx] = m_viewData[1 - NeoUpdateFrameIdx];
			m_firstUpdate = false;
		}, 0
	);
}

View::~View()
{
	NeoRemoveBeginUpdateTask(m_beginUpdateHandle);
}

void View::SetOrthographic(const OrthographicInfo& info)
{
	m_viewData[NeoUpdateFrameIdx].orthographic = info;
}

void View::SetPerspective(const PerspectiveInfo& info)
{
	m_viewData[NeoUpdateFrameIdx].perspective = info;
}

void View::SetLookAt(const vec3& eye, const vec3& target, const vec3& up)
{
	m_viewData[NeoUpdateFrameIdx].cameraMatrix = LookAt(eye, target, up);
}

void View::SetCameraMatrix(const mat4x4& camMatrix)
{
	m_viewData[NeoUpdateFrameIdx].cameraMatrix = camMatrix;
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

void View::InitUBOView(UBO_View &viewData, float aspectRatio)
{
	auto& vd = m_viewData[NeoDrawFrameIdx];

	viewData.proj = glm::perspective(vd.perspective.fov, aspectRatio, vd.perspective.nearPlane, vd.perspective.farPlane);
	viewData.proj[1][1] *= -1.0f;
	viewData.ortho = OrthoProj(vd.orthographic.orthoRect, vd.orthographic.nearPlane, vd.orthographic.farPlane);
	viewData.view = glm::inverse(vd.cameraMatrix);
}

