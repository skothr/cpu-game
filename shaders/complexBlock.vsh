#version 330

#define ATLAS_SIZE 512
#define ATLAS_BLOCK_SIZE 64
#define ATLAS_W (ATLAS_SIZE / ATLAS_BLOCK_SIZE)

uniform mat4 pvm;
uniform int uBlockType;

layout(location = 0) in vec3 posAttr;
layout(location = 1) in vec3 normalAttr;
layout(location = 2) in vec2 texCoordAttr;

smooth out vec3 normal;
smooth out vec3 texCoord;

void main()
{
  gl_Position = pvm * vec4(posAttr,1);
  normal = normalAttr;
  texCoord = vec3(texCoordAttr, uBlockType);
  /*
  float row = floor(uBlockType / ATLAS_W);
  float col = (uBlockType % ATLAS_W);
  texCoord = ((texCoordAttr * (ATLAS_BLOCK_SIZE - 3) + 1) / ATLAS_SIZE +
	      vec2((col*ATLAS_BLOCK_SIZE) / ATLAS_SIZE,
		   (row*ATLAS_BLOCK_SIZE) / ATLAS_SIZE) );
  */
}
