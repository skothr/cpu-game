#version 330

uniform mat4 pvm;
uniform vec3 camPos;
uniform float fogStart;
uniform float fogEnd;
uniform vec3 dirScale;

smooth in vec3 color;
smooth in float alpha;
smooth in float fog;
out vec4 fragColor;

void main()
{
  //fragColor = mix(vec4(0.229, 0.657, 0.921, 1.0), vec4(color, 1.0), clamp(fog, 0.0, 1.0));
  //fragColor = mix(vec4(0.229, 0.657, 0.921, 1.0), vec4(color, 1.0), clamp(fog, 0.0, 1.0) );
  fragColor = vec4(color, alpha);
}
