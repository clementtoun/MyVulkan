#pragma once

#include "VkGLM.h"
#include <glm/trigonometric.hpp>
#include <glm/gtc/matrix_transform.hpp>
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

	virtual void Move(Direction direction, float deltaTime) = 0;
	virtual void ProcessMouseMouve(double x, double y, bool leftClick) = 0;

	virtual void SetMouseSensibility(float mouseSensibility) = 0;
	virtual void SetSpeed(float speed) = 0;
	void SetAspect(double aspect);

	const glm::mat4& GetView();
	const glm::mat4& GetProjection();
	const glm::vec3& GetPosition();

protected:
	void UpdateViewMatrix();

	glm::vec3 m_Position;
	glm::vec3 m_Forward;
	glm::vec3 m_Left;
	glm::vec3 m_Up;
	const glm::vec3 m_WorldUp = glm::vec3(0., 1., 0.);

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

	void SetMouseSensibility(float mouseSensibility) override;
	void SetSpeed(float speed) override;

	void Move(Direction direction, float deltaTime) override;
	void ProcessMouseMouve(double x, double y, bool leftClick) override;

private:
	float m_MouseSensibility;
	float m_Speed;

	float m_LastMouseX = 0;
	float m_LastMouseY = 0;
	float m_Yaw = 0;
	float m_Pitch = 0;
};

