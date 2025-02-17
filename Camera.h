#pragma once

#include "VkGLM.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/trigonometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <iostream>

enum Direction
{
	FORWARD,
	LEFT,
	BACKWARD,
	RIGHT,
	UP,
	DOWN
};

class Camera
{
public:
	Camera();
	Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up, double fov, double aspect, double near, double far);

	virtual ~Camera();

	virtual void Move(Direction direction, float deltaTime) = 0;
	virtual void UpdatePosition(float deltaTime) = 0;
	virtual void ProcessMouseMouve(double x, double y) = 0;
	virtual void ProcessScroll(double xoffset, double yoffset) = 0;
	virtual void ProcessMouseButton(int button, int action, int mods);

	void SetMouseSensibility(float mouseSensibility);
	void SetSpeed(float speed);
	void SetAspect(double aspect);

	const glm::mat4& GetView();
	const glm::mat4& GetProjection();
	const glm::vec3& GetPosition();

	float* GetSpeed();

protected:
	void UpdateViewMatrix();
	void UpdateProjectionMatrix();

	glm::vec3 m_Position;
	glm::vec3 m_Forward;
	glm::vec3 m_Left;
	glm::vec3 m_Up;
	const glm::vec3 m_WorldUp = glm::vec3(0., 1., 0.);

	int m_leftClic = false;
	bool m_newLeftClic = true;
	float m_LastMouseX = 0;
	float m_LastMouseY = 0;

	float m_MouseSensibility;
	float m_Speed;

	double m_Fov;
	double m_Aspect;
	double m_Near;
	double m_Far;

	glm::mat4 m_View;
	glm::mat4 m_Projection;
};

class EulerCamera : public Camera
{
public:
	EulerCamera();
	EulerCamera(glm::vec3 position, glm::vec3 target, glm::vec3 up, double fov, double aspect, double near, double far);

	void Move(Direction direction, float deltaTime) override;
	void ProcessMouseMouve(double x, double y) override;
	void ProcessScroll(double xoffset, double yoffset) override {};

private:

	float m_Yaw = 0;
	float m_Pitch = 0;
};

class TrackBallCamera : public Camera
{
public:
	TrackBallCamera();
	TrackBallCamera(glm::vec3 position, glm::vec3 target, glm::vec3 up, double fov, double aspect, double near, double far);

	void Move(Direction direction, float deltaTime) override {};
	void UpdatePosition(float deltaTime) override {};
	void ProcessMouseMouve(double x, double y) override;
	void ProcessScroll(double xoffset, double yoffset) override;

private:

	float m_Yaw = 0;
	float m_Pitch = 0;
};

class QuaternionCamera : public Camera
{
public:
	QuaternionCamera();
	QuaternionCamera(glm::vec3 position, glm::vec3 target, glm::vec3 up, double fov, double aspect, double near, double far);

	void Move(Direction direction, float deltaTime) override;
	void UpdatePosition(float deltaTime) override;
	void ProcessMouseMouve(double x, double y) override;
	void ProcessScroll(double xoffset, double yoffset) override {};

private:

	float m_Yaw = 0;
	float m_Pitch = 0;

	glm::vec3 m_Deplacement = glm::vec3(0.f);
	float m_Falloff = 0.f;
	bool m_IsMoving = false;
};



