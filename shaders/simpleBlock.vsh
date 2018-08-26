#version 330


#define FOG_START 128
#define FOG_END 200

uniform mat4 pvm;
uniform vec3 camPos;
uniform float fogStart;
uniform float fogEnd;
uniform sampler2DArray uTex;
//uniform int uBlockType;

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
  float dist = sqrt((camPos.x - posAttr.x)*(camPos.x - posAttr.x) +
                    (camPos.y - posAttr.y)*(camPos.y - posAttr.y) +
                    (camPos.z - posAttr.z)*(camPos.z - posAttr.z) );
    fog = (fogEnd - dist)/(fogEnd - fogStart);
  //    fog = (FOG_END - dist)/(FOG_END - FOG_START);
}
