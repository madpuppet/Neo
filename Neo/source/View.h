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

	void SetOrthographic(const OrthographicInfo& info);
	void SetPerspective(const PerspectiveInfo& info);
	void SetLookAt(const vec3& eye, const vec3& target, const vec3& up);
	void SetCameraMatrix(const mat4x4& camMatrix);

	void Apply(float aspectRatio);

protected:
	PerspectiveInfo m_perspective;
	OrthographicInfo m_orthographic;

	mat4x4 m_cameraMatrix = { mat4x4(1) };
};
