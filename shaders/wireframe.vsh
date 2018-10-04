#version 330

uniform mat4 pvm;
uniform vec3 stretch;

layout(location = 0) in vec3 posAttr;
layout(location = 1) in vec3 normalAttr;
layout(location = 2) in vec2 texCoordAttr;

smooth out vec2 texCoords;
smooth out vec2 texScale;
void main()
{
  gl_Position = pvm * vec4(posAttr,1);
  texCoords = texCoordAttr;
  vec2 tex;
  
  if(abs(normalAttr.z) > 0.5)
    {
      tex.x = (tex.x > 0.5 ? stretch.x : 1.0);
      tex.y = (tex.y > 0.5 ? stretch.y : 1.0);
    }
  else if(abs(normalAttr.x) > 0.5)
    {
      tex.x = stretch.z;
      tex.y = stretch.y;
    }
  else if(abs(normalAttr.y) > 0.5)
    {
      tex.x = stretch.x;
      tex.y = stretch.z;
    }
  
  texScale = tex;
}
