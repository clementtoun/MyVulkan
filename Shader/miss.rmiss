#version 460
#extension GL_EXT_ray_tracing : enable

layout(location = 0) rayPayloadInEXT bool hitValue;

void main()
{
    hitValue = false;
}