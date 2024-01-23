#include "Camera.h"

Camera::Camera() : 
    m_Position(glm::vec3(0.)), m_Forward(glm::vec3(0.,0.,-1.)),
    m_Fov(45.), m_Aspect(1.), m_Near(0.1), m_Far(100.)
{
    m_Left = glm::normalize(glm::cross(m_WorldUp, m_Forward));
    m_Up = glm::cross(m_Forward, m_Left);

    UpdateViewMatrix();
    m_Projection = glm::perspective(m_Fov, m_Aspect, m_Near, m_Far);
}

Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up, double fov, double aspect, double near, double far) :
    m_Position(position), m_Fov(fov), m_Aspect(aspect), m_Near(near), m_Far(far)
{
    m_Forward = glm::normalize(target - position);
    m_Left = glm::normalize(glm::cross(m_WorldUp, m_Forward));
    m_Up = glm::cross(m_Forward, m_Left);

    UpdateViewMatrix();
    m_Projection = glm::perspective(m_Fov, m_Aspect, m_Near, m_Far);
}

void Camera::SetAspect(double aspect)
{
    m_Aspect = aspect;
}

const glm::mat4& Camera::GetView()
{
    return m_View;
}

const glm::mat4& Camera::GetProjection()
{
    return m_Projection;
}

void Camera::UpdateViewMatrix()
{
    m_View = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
}

EulerCamera::EulerCamera() : Camera(), m_MouseSensibility(1.), m_Speed(1.)
{
}

EulerCamera::EulerCamera(glm::vec3 position, glm::vec3 target, glm::vec3 up, double fov, double aspect, double near, double far) :
    Camera(position, target, up, fov, aspect, near, far), m_MouseSensibility(1.), m_Speed(1.)
{
}

void EulerCamera::SetMouseSensibility(float mouseSensibility)
{
    m_MouseSensibility = mouseSensibility;
}

void EulerCamera::SetSpeed(float speed)
{
    m_Speed = speed;
}

void EulerCamera::Move(Direction direction, float deltaTime)
{
    switch (direction)
    {
        case FORWARD:
        {
            m_Position += m_Forward * m_Speed * deltaTime;
            break;
        }
        case LEFT:
        {
            m_Position += m_Left * m_Speed * deltaTime;
            break;
        }
        case BACKWARD:
        {
            m_Position -= m_Forward * m_Speed * deltaTime;
            break;
        }
        case RIGHT:
        {
            m_Position -= m_Left * m_Speed * deltaTime;
            break;
        }
        case UP:
        {
            m_Position += m_WorldUp * m_Speed * deltaTime;
            break;
        }
        case DOWN:
        {
            m_Position -= m_WorldUp * m_Speed * deltaTime;
            break;
        }
    }

    UpdateViewMatrix();
}

void EulerCamera::ProcessMouseMouve(double x, double y, bool leftClick)
{
    if (leftClick)
    {
        m_Yaw += (x - m_LastMouseX) * m_MouseSensibility * 90.;
        m_Pitch += (y - m_LastMouseY) * m_MouseSensibility * 90.;

        if (m_Yaw >= 360)
            m_Yaw -= 360;
        if (m_Yaw <= -360)
            m_Yaw += 360;
        m_Pitch = glm::clamp<double>(m_Pitch, -90. + 1e-3, 90. - 1e-3);

        double radPitch = glm::radians(m_Pitch);
        double radYaw = glm::radians(m_Yaw);

        double cPitch = glm::cos(radPitch);
        double sPitch = glm::sin(radPitch);
        double cYawcPitch = glm::cos(radYaw) * cPitch;
        double sYawcPitch = glm::sin(radYaw) * cPitch;

        m_Forward = glm::normalize(glm::vec3(sYawcPitch, sPitch, -cYawcPitch));
        m_Left = glm::normalize(glm::cross(m_WorldUp, m_Forward));
        m_Up = glm::cross(m_Forward, m_Left);

        UpdateViewMatrix();
    }

    m_LastMouseX = x;
    m_LastMouseY = y;
}
