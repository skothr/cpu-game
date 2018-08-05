#version 330

uniform sampler2DArray uTex;

#define LIGHT_DIR vec3(0.6, 0.8, 1.0)

smooth in vec3 normal;
smooth in vec3 texCoord;
out vec4 fragColor;

void main()
{
  //float intensity = max(dot(normalize(normal), normalize(LIGHT_DIR)), 0.0);
  fragColor = vec4(texture(uTex, texCoord).rgb, 1.0);
}
