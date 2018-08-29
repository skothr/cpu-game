#version 330

uniform mat4 pvm;

layout(location = 0) in vec3 posAttr;
layout(location = 1) in vec3 normalAttr;
layout(location = 2) in vec2 texCoordAttr;

smooth out vec2 texCoords;
void main()
{
  gl_Position = pvm * vec4(posAttr,1);
  texCoords = texCoordAttr;
}
