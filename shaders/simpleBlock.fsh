#version 330

uniform sampler2DArray uTex;

#define LIGHT_DIR vec3(0.6, 0.8, 1.0)
#define MIN_LIGHT 0.3

smooth in vec3 normal;
smooth in vec3 texCoord;
smooth in float lighting;
smooth in float fog;
out vec4 fragColor;

void main()
{
  float mult = mix(MIN_LIGHT, 0.9, lighting);
  vec4 texColor = texture(uTex, texCoord);
  fragColor = mix(vec4(0.229, 0.657, 0.921, 1.0), vec4(texColor.rgb*mult, texColor.a),
                clamp(fog, 0.0, 1.0) );
  //fragColor = vec4(clamp(fog, 0.0, 1.0), 0, 0, 1);
}
