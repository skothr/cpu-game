#version 330

uniform float fogStart;
uniform float fogEnd;
uniform vec3 dirScale;
uniform sampler2D rayTex;

layout(location = 0) in vec2 posAttr;
layout(location = 1) in vec2 texCoordAttr; // third element is block type

smooth out vec2 texCoords;
             
void main()
{
  gl_Position = vec4(posAttr, 0.999999, 1.0);
  texCoords = texCoordAttr;
}
/*
uniform vec3 camPos;
uniform float fovY;
uniform float aspect;
uniform mat4 ip;
uniform mat4 iv;

uniform float fogStart;
uniform float fogEnd;
uniform vec3 dirScale;
uniform sampler2DArray uTex;

layout(location = 0) in vec2 posAttr;
layout(location = 1) in vec3 texCoordAttr; // third element is block type

smooth out vec3 rayDir;

void main()
{
  gl_Position = vec4(posAttr, 0.999999, 1.0);
  vec4 sDir = ip * vec4(posAttr.x, posAttr.y, -1.0, 1.0);
  sDir.z = -1.0;
  sDir.w = 0.0;
  rayDir = normalize(vec3(iv * sDir));
}
*/
