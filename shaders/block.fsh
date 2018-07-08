#version 330

uniform sampler2D uTex;

smooth in vec3 normal;
smooth in vec2 texCoord;
out vec4 fragColor;

void main()
{
  //fragColor = vec4(texCoord, 0.0, 1.0);
  fragColor = vec4(texture(uTex, texCoord).rgb, 1.0);
}


/*
void lerp(in float x1, in float x2, in float a, out float xOut)
{
  xOut = (1.0 - a) * x1 + a * x2;
}
float lerp(in float x1, in float x2, in float a)
{
  return (1.0 - a) * x1 + a * x2;
}
vec3 lerp(in vec3 v1, in vec3 v2, in float a)
{
  return v1 * (1.0 - a) + v2 * a;
}
void lerp(in vec3 v1, in vec3 v2, in float a, out vec3 vOut)
{
  lerp(v1.x, v2.x, a, vOut.x);
  lerp(v1.y, v2.y, a, vOut.y);
  lerp(v1.z, v2.z, a, vOut.z);
}
*/
