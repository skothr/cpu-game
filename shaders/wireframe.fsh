#version 330

#define LINE_W 0.1

uniform vec3 stretch;

smooth in vec3 color;
smooth in vec2 texCoords;
smooth in vec2 texScale;
out vec4 fragColor;

void main()
{
  if(texCoords.x <= LINE_W/texScale.x || texCoords.y <= LINE_W/texScale.y ||
     1.0 - texCoords.x <= LINE_W/texScale.x || 1.0 - texCoords.y <= LINE_W/texScale.y )
    {
      fragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
  else if(mod(texCoords.x*texScale.x, 1.0) <= 0.05 || mod(texCoords.y*texScale.y, 1.0) <= 0.05 )
    {
      fragColor = vec4(1.0, 1.0, 1.0, 0.4);

    }
  else
    {
      fragColor = vec4(0.0, 0.0, 0.0, 0.0);
    }
}
