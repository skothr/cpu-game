#version 330

uniform float fogStart;
uniform float fogEnd;
uniform vec3 dirScale;
uniform sampler2D rayTex;

smooth in vec2 texCoords;
out vec4 fragColor;

void main()
{
  fragColor = texture(rayTex, texCoords);
}

/*
uniform vec3 camPos;

#define LIGHT_DIR vec3(0.6, 0.8, 1.0)
#define MIN_LIGHT 0.3

smooth in vec3 rayDir;
out vec4 fragColor;

float intersectPlane(vec3 n, vec3 p, vec3 pos, vec3 dir, float t) 
{ 
  // assuming vectors are all normalized
  float denom = dot(n, dir);
  vec3 pl = p - pos; 
  t = dot(pl, n) / denom;
  return t;
}
bool intersectSphere(vec3 c, float r, vec3 pos, vec3 dir, float t) 
{
  vec3 diff = pos - c;
  float d1 = (2*(dot(dir, diff)));
  t = (-2*(dot(dir, (pos - c))) + sqrt(d1*d1 - 4*dot(dir, dir)*(dot(diff, diff) - r*r))) / (2*dot(dir,dir));

  return t >= 0;
}

void main()
{
  fragColor = vec4(0,0,0, 0.5);
  vec3 pos = vec3(0,0,0);
  vec3 n = vec3(0,0,1);
  float t = intersectPlane(n, pos, camPos, rayDir, 0.0);
  if(t >= 0)
    {
      vec3 iPos = camPos + rayDir*t;
      vec3 fPos = floor(iPos);
      vec3 texcoord = iPos - pos;
      if(n.x > 0.0)
        {
          texcoord.x = texcoord.y;
          texcoord.y = texcoord.z;
          texcoord.z = 0.2;
        }
      else if(n.y > 0)
        {
          texcoord.y = texcoord.z;
          texcoord.z = 0.2;
        }
      else if(n.z > 0)
        {
          texcoord.z = 0.2;
        }
  
      if(texcoord.x >= 0 && texcoord.x <= 1 &&
         texcoord.y >= 0 && texcoord.y <= 1 &&
         texcoord.z >= 0 && texcoord.z <= 1)
        {
          fragColor = texture(uTex, texcoord);
        }
    }
}
*/
