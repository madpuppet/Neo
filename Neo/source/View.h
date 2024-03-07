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

class View
{
public:
	struct PerspectiveInfo
	{
		float fov = PI / 2.0f;
		float nearPlane = 0.5f;
		float farPlane = 20.0f;
	};
	struct OrthographicInfo
	{
		rect orthoRect = { 0, 0, 1, 1 };
		float nearPlane = 0.0f;
		float farPlane = 1.0f;
	};
	struct ViewData
	{
		PerspectiveInfo perspective;
		OrthographicInfo orthographic;
		mat4x4 cameraMatrix = { mat4x4(1) };
	};

	View();
	~View();
	void SetOrthographic(const OrthographicInfo& info);
	void SetPerspective(const PerspectiveInfo& info);
	void SetLookAt(const vec3& eye, const vec3& target, const vec3& up);
	void SetCameraMatrix(const mat4x4& camMatrix);
	void InitUBOView(UBO_View& viewData, float aspectRatio);

protected:
	// double buffered view data
	ViewData m_viewData[2];
	int m_beginUpdateHandle = 0;
	bool m_firstUpdate = true;
};
