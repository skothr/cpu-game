#version 330

uniform sampler2DArray uTex;

#define LIGHT_DIR vec3(0.6, 0.8, 1.0)
#define MIN_LIGHT 0.3

smooth in vec3 normal;
smooth in vec3 texCoord;
smooth in float lighting;
out vec4 fragColor;

void main()
{
  float mult = mix(MIN_LIGHT, 0.9, lighting);
  fragColor = vec4(texture(uTex, texCoord).rgb*mult, 1.0);
}
