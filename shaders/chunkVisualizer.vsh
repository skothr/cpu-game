#version 330

uniform mat4 pvm;
//uniform sampler2DArray uTex;

layout(location = 0) in vec3 posAttr;
layout(location = 1) in vec3 normalAttr;
layout(location = 2) in vec3 texCoordAttr;
layout(location = 3) in float lightAttr;

smooth out vec3 normal;
//smooth out vec3 texCoord;
//smooth out float lighting;

void main()
{
  gl_Position = pvm * vec4(posAttr, 1);
  normal = normalAttr;
  //texCoord = vec3(texCoordAttr);
  //lighting = lightAttr;
}
