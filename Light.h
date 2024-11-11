#pragma once

#include "VkGLM.h"

struct alignas(16) UniformDirectionalLight
{
    alignas(16) glm::vec3 Color;
    alignas(16) glm::vec3 Direction;
};

struct alignas(16) UniformPointLight
{
    alignas(16) glm::vec3 Color;
    alignas(16) glm::vec3 Position;
};

class Light
{

public:
    void SetColor(const glm::vec3& color);
    void SetIntensity(const float intensity);
    
    glm::vec3& GetColor();
    float GetIntensity();

protected:
    Light() = default;
    Light(glm::vec3 color, float intensity);
    virtual ~Light();

    glm::vec3 m_Color = glm::vec3(1.f);
    float m_Intensity = 1.f;
};

class DirectionalLight : public Light
{
public:
    DirectionalLight() = default;
    DirectionalLight(glm::vec3 direction);
    DirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity);

    UniformDirectionalLight GetUniformDirectionalLight();
    
    ~DirectionalLight() override = default;

private:
    glm::vec3 m_Direction = glm::vec3(0.f, -1.f, 0.f);
};

class PointLight : public Light
{
public:
    PointLight() = default;
    PointLight(glm::vec3 position);
    PointLight(glm::vec3 position, glm::vec3 color, float intensity);

    UniformPointLight GetUniformPointLight();

    ~PointLight() override = default;

private:
    glm::vec3 m_Position = glm::vec3(0.f);
};
