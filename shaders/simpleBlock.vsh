#version 330

uniform mat4 pvm;
uniform vec3 camPos;
uniform float fogStart;
uniform float fogEnd;
uniform vec3 dirScale;
uniform sampler2DArray uTex;

layout(location = 0) in vec3 posAttr;
layout(location = 1) in vec3 normalAttr;
layout(location = 2) in vec3 texCoordAttr; // third element is block type
layout(location = 3) in float lightAttr; // third element is block type

smooth out vec3 normal;
smooth out vec3 texCoord;
smooth out float lighting;
smooth out vec3 dist;

void main()
{
  vec4 vPos = pvm * vec4(posAttr, 1);
  gl_Position = vPos;
  normal = normalAttr;
  
  texCoord = vec3(texCoordAttr);
  lighting = lightAttr;
  dist = camPos - posAttr;
}
