#version 330

uniform mat4 pvm;
//uniform sampler2DArray uTex;

smooth in vec3 normal;
//smooth in vec3 texCoord;
//smooth in float lighting;
out vec4 fragColor;

void main()
{
  //float mult = mix(MIN_LIGHT, 0.9, lighting);
  //vec4 texColor = texture(uTex, texCoord);
  fragColor = vec4(normal, 0.3);
  //fragColor = vec4(1,0,0,1);
}
