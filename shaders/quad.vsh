#version 330

layout(location = 0) in vec2 posAttr;
smooth out vec2 texCoords;

void main()
{
  gl_Position = vec4(posAttr, 0.0, 1.0);
  texCoords = (posAttr + 1.0) / 2.0;
}
