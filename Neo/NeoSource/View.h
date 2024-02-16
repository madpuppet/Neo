#pragma once

// NEO cordinate system
//
// Y (+ up)
// .     Z (+ into screen)
// .   .
// . . 
// . . . . . X (+ right)
//
// clockwise polygon culling for front face.
//
// NOTE: Vulkan clip space is:
//   left->right = -1 .. 1
//   top->bottom = -1 .. 1
//   near->far   =  0 .. 1

#include "MathUtils.h"

class View
{
public:
	View();
	~View();

	struct PerspectiveInfo
	{
		float fov = PI/2.0f;
		float nearPlane = 0.5f;
		float farPlane = 20.0f;
	};
	struct OrthographicInfo
	{
		rect orthoRect = { 0, 0, 1, 1 };
		float nearPlane = 0.0f;
		float farPlane = 1.0f;
	};

	void SetViewport(const rect& viewPort);
	void SetOrthographic(const OrthographicInfo& info);
	void SetPerspective(const PerspectiveInfo& info);
	void SetLookAt(const vec3& eye, const vec3& target, const vec3& up);
	void SetCameraMatrix(const mat4x4& camMatrix);
	void SetDepthRange(float minDepth, float maxDepth);

	// set scissor rect - top-left == 0,0,   bottom-right == 1,1
	void SetScissorRect(const rect& scissorRect);

	void Apply();

protected:
	PerspectiveInfo m_perspective;
	OrthographicInfo m_orthographic;

	UBOInfo* m_viewUBO;
	UniformBufferPlatformData* m_platformData;

	mat4x4 m_cameraMatrix = { mat4x4(1) };
	rect m_viewport;
	rect m_scissor;
	float m_minDepth = 0.0f;
	float m_maxDepth = 1.0f;
};
