#version 330

uniform mat4 pvm;
uniform vec3 camPos;
uniform float fogStart;
uniform float fogEnd;
uniform vec3 dirScale;

layout(location = 0) in vec3 posAttr;
layout(location = 1) in vec3 normalAttr;
layout(location = 2) in vec2 texCoordAttr;

smooth out vec3 color;
smooth out float fog;

void main()
{
  gl_Position = pvm * vec4(posAttr, 1);
  
  color = normalAttr;
  float dist = sqrt((camPos.x - posAttr.x)*(camPos.x - posAttr.x)*dirScale.x +
                    (camPos.y - posAttr.y)*(camPos.y - posAttr.y)*dirScale.y +
                    (camPos.z - posAttr.z)*(camPos.z - posAttr.z)*dirScale.z );
  fog = (fogEnd - dist)/(fogEnd - fogStart);
}
