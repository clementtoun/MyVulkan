#include "Light.h"

void Light::SetColor(const glm::vec3& color)
{
    m_Color = color;
}

void Light::SetIntensity(const float intensity)
{
    m_Intensity = intensity;
}

glm::vec3& Light::GetColor()
{
    return m_Color;
}

float Light::GetIntensity()
{
    return m_Intensity;
}

Light::~Light()
{
}

Light::Light(glm::vec3 color, float intensity)
{
    m_Color = color;
    m_Intensity = intensity;
}

DirectionalLight::DirectionalLight(glm::vec3 direction)
{
    m_Direction = direction;
}

DirectionalLight::DirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity)
{
    m_Direction = direction;
    m_Color = color;
    m_Intensity = intensity;
}

UniformDirectionalLight DirectionalLight::GetUniformDirectionalLight()
{
    return {m_Color * m_Intensity, m_Direction};
}

PointLight::PointLight(glm::vec3 position)
{
    m_Position = position;
}

PointLight::PointLight(glm::vec3 position, glm::vec3 color, float intensity)
{
    m_Position = position;
    m_Color = color;
    m_Intensity = intensity;
}

void PointLight::SetPosition(glm::vec3 position)
{
    m_Position = position;
}

UniformPointLight PointLight::GetUniformPointLight()
{
    return {m_Color * m_Intensity, m_Position};
}
