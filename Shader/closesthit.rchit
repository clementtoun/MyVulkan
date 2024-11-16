#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable

layout(location = 0) rayPayloadInEXT bool hitValue;
hitAttributeEXT vec2 attribs;

void main()
{
  //const vec3 barycentricCoords = vec3(1.0f - attribs.x - attribs.y, attribs.x, attribs.y);
  //hitValue = vec4(barycentricCoords, 1.);
  
  hitValue = true;
}