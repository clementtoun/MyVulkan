#include "Camera.h"

Camera::Camera() : 
    m_Position(glm::vec3(0.)), m_Forward(glm::vec3(0.,0.,-1.)),
    m_Fov(45.), m_Aspect(1.), m_Near(0.1), m_Far(100.), m_MouseSensibility(1.), m_Speed(1.)
{
    m_Left = glm::normalize(glm::cross(m_WorldUp, m_Forward));
    m_Up = glm::cross(m_Forward, m_Left);

    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

Camera::Camera(glm::vec3 position, glm::vec3 target, glm::vec3 up, double fov, double aspect, double near, double far) :
    m_Position(position), m_Fov(fov), m_Aspect(aspect), m_Near(near), m_Far(far), m_MouseSensibility(1.), m_Speed(1.)
{
    m_Forward = glm::normalize(target - position);
    m_Left = glm::normalize(glm::cross(m_WorldUp, m_Forward));
    m_Up = glm::cross(m_Forward, m_Left);

    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

void Camera::SetMouseSensibility(float mouseSensibility)
{
    m_MouseSensibility = mouseSensibility;
}

void Camera::SetSpeed(float speed)
{
    m_Speed = speed;
}

void Camera::SetAspect(double aspect)
{
    m_Aspect = aspect;
    UpdateProjectionMatrix();
}

const glm::mat4& Camera::GetView()
{
    return m_View;
}

const glm::mat4& Camera::GetProjection()
{
    return m_Projection;
}

const glm::vec3& Camera::GetPosition()
{
    return m_Position;
}

void Camera::UpdateViewMatrix()
{
    m_View = glm::lookAt(m_Position, m_Position + m_Forward, m_Up);
}

void Camera::UpdateProjectionMatrix()
{
    m_Projection = glm::perspective(glm::radians(m_Fov), m_Aspect, m_Near, m_Far);
}

EulerCamera::EulerCamera() : Camera()
{
}

EulerCamera::EulerCamera(glm::vec3 position, glm::vec3 target, glm::vec3 up, double fov, double aspect, double near, double far) :
    Camera(position, target, up, fov, aspect, near, far)
{
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
        m_Pitch += (m_LastMouseY - y) * m_MouseSensibility * 90.;
        
        if (m_Yaw >= 360)
            m_Yaw -= 360;
        if (m_Yaw <= -360)
            m_Yaw += 360;
        m_Pitch = glm::clamp<double>(m_Pitch, -89.9999, 89.9999);

        float radPitch = glm::radians(m_Pitch);
        float radYaw = glm::radians(m_Yaw);

        float cPitch = glm::cos(radPitch);
        float sPitch = glm::sin(radPitch);
        float cYawcPitch = glm::cos(radYaw) * cPitch;
        float sYawcPitch = glm::sin(radYaw) * cPitch;

        m_Forward = glm::normalize(glm::vec3(sYawcPitch, sPitch, -cYawcPitch));
        m_Left = glm::normalize(glm::cross(m_WorldUp, m_Forward));
        m_Up = glm::cross(m_Forward, m_Left);

        UpdateViewMatrix();
    }

    m_LastMouseX = x;
    m_LastMouseY = y;
}

TrackBallCamera::TrackBallCamera() : Camera()
{
}

TrackBallCamera::TrackBallCamera(glm::vec3 position, glm::vec3 target, glm::vec3 up, double fov, double aspect, double near, double far) :
    Camera(position, target, up, fov, aspect, near, far)
{
}

void TrackBallCamera::ProcessMouseMouve(double x, double y, bool leftClick)
{
    if (leftClick)
    {
        m_Yaw += (x - m_LastMouseX) * m_MouseSensibility * 90.;
        m_Pitch += (m_LastMouseY - y) * m_MouseSensibility * 90.;

        if (m_Yaw >= 360)
            m_Yaw -= 360;
        if (m_Yaw <= -360)
            m_Yaw += 360;
        m_Pitch = glm::clamp<double>(m_Pitch, -89.9999, 89.9999);

        float distance = glm::length(m_Position);

        float radPitch = glm::radians(m_Pitch);
        float radYaw = glm::radians(m_Yaw);

        float cPitch = glm::cos(radPitch);
        float sPitch = glm::sin(radPitch);
        float cYawcPitch = glm::cos(radYaw) * cPitch;
        float sYawcPitch = glm::sin(radYaw) * cPitch;

        m_Position = distance * glm::vec3(-sYawcPitch, -sPitch, cYawcPitch);

        m_Forward = glm::normalize(-m_Position);
        m_Left = glm::normalize(glm::cross(m_WorldUp, m_Forward));
        m_Up = glm::cross(m_Forward, m_Left);

        UpdateViewMatrix();
    }

    m_LastMouseX = x;
    m_LastMouseY = y;
}

void TrackBallCamera::ProcessScroll(double xoffset, double yoffset)
{
    m_Position += m_Forward * m_Speed * float(yoffset);


    if (glm::dot(m_Position, m_Position) < 1.f)
        m_Position = -m_Forward;

    UpdateViewMatrix();
}

QuaternionCamera::QuaternionCamera() : Camera()
{
}

QuaternionCamera::QuaternionCamera(glm::vec3 position, glm::vec3 target, glm::vec3 up, double fov, double aspect, double near, double far) :
    Camera(position, target, up, fov, aspect, near, far)
{
}

void QuaternionCamera::Move(Direction direction, float deltaTime)
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

void QuaternionCamera::ProcessMouseMouve(double x, double y, bool leftClick)
{
    if (leftClick)
    {
        m_Yaw += (x - m_LastMouseX) * m_MouseSensibility * 90.;
        m_Pitch += (y - m_LastMouseY) * m_MouseSensibility * 90.;

        if (m_Yaw >= 360)
            m_Yaw -= 360;
        if (m_Yaw <= -360)
            m_Yaw += 360;
        if (m_Pitch >= 360)
            m_Pitch -= 360;
        if (m_Pitch <= -360)
            m_Pitch += 360;


        glm::quat q = glm::angleAxis(glm::radians(m_Pitch), glm::vec3(1, 0, 0));
        q *= glm::angleAxis(glm::radians(m_Yaw), glm::vec3(0, 1, 0));

        glm::mat3 rotMatrix = glm::mat3_cast(q);

        m_Left = -glm::row(rotMatrix, 0);
        m_Up = glm::row(rotMatrix, 1);
        m_Forward = -glm::row(rotMatrix, 2);

        UpdateViewMatrix();
    }

    m_LastMouseX = x;
    m_LastMouseY = y;
}
