#version 330

uniform mat4 pvm;
uniform vec3 camPos;
uniform float fogStart;
uniform float fogEnd;
uniform vec3 dirScale;
uniform sampler2DArray uTex;

#define LIGHT_DIR vec3(0.6, 0.8, 1.0)
#define MIN_LIGHT 0.3

smooth in vec3 normal;
smooth in vec3 texCoord;
smooth in float lighting;
smooth in vec3 dist;
out vec4 fragColor;

void main()
{
  float fog = sqrt(dist.x*dist.x*dirScale.x +
                   dist.y*dist.y*dirScale.y +
                   dist.z*dist.z*dirScale.z );
  fog = (fogEnd - fog)/(fogEnd - fogStart);
  
  float mult = mix(MIN_LIGHT, 0.9, lighting);
  vec4 texColor = texture(uTex, texCoord);
  fragColor = mix(vec4(0.229, 0.657, 0.921, 1.0), vec4(texColor.rgb*mult, texColor.a),
                clamp(fog, 0.0, 1.0) );
  //fragColor = vec4(clamp(fog, 0.0, 1.0), 0, 0, 1);
}
