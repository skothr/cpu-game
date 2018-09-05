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
smooth out float fog;

void main()
{
  vec4 vPos = pvm * vec4(posAttr, 1);
  gl_Position = vPos;
  normal = normalAttr;
  
  texCoord = vec3(texCoordAttr);
  lighting = lightAttr;
  float dist = sqrt((camPos.x - posAttr.x)*(camPos.x - posAttr.x)*dirScale.x +
                    (camPos.y - posAttr.y)*(camPos.y - posAttr.y)*dirScale.y +
                    (camPos.z - posAttr.z)*(camPos.z - posAttr.z)*dirScale.z );
  fog = (fogEnd - dist)/(fogEnd - fogStart);
}
