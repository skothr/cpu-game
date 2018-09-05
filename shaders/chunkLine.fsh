#version 330

#define LINE_W 0.05

smooth in vec3 normal;
smooth in float fog;
out vec4 fragColor;

void main()
{
  //fragColor = mix(vec4(0.229, 0.657, 0.921, 1.0), vec4(normal, 1.0), clamp(fog, 0.0, 1.0));
  fragColor = vec4(normal, 1.0);
}
