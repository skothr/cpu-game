#version 330

#define LINE_W 0.05

smooth in vec3 color;
smooth in vec2 texCoords;
smooth in vec3 normal;
out vec4 fragColor;

void main()
{
  if(abs(texCoords.x) > (1.0-LINE_W) || abs(texCoords.y) > (1.0-LINE_W))
    {
      fragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
  else
    {
      fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
