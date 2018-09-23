#version 430

uniform vec2 screenSize;
uniform vec3 camPos;
uniform vec3 v00;
uniform vec3 v10;
uniform vec3 v01;
uniform vec3 v11;

uniform sampler2DArray blockTex;

#define LIGHT_DIR vec3(0.2673,0.5345,0.8018)
#define BLOCK_RAD 32
#define SX 32
#define SY 32
#define SZ 32

struct Chunk
{
  int blocks[SX*SY*SZ];
};

layout(local_size_x = 1, local_size_y = 1) in;
layout(rgba32f, binding = 1) uniform image2D imgOut;
layout(std430, binding = 2) buffer cubes
{
  //Chunk chunks[];
  int blocks[];
};

struct HitPos
{
  int type;
  vec3 normal;
  vec3 texcoord;
  vec3 intersect;
};

int index(ivec3 p)
{ return p.x + SX * (p.z + SZ * p.y); }

bool checkIntersect(vec3 p, vec3 d)
{
  ivec3 st = ivec3(sign(d));
  ivec3 limit = clamp(st * BLOCK_RAD, 0, BLOCK_RAD);
  vec3 tMax = (floor(p) - p + clamp(sign(d), 0, 1)) / d;
  vec3 delta = st / d;
  
    ivec3 pi = ivec3(p);
  
    while(st.x*pi.x < limit.x && st.y*pi.y < limit.y && st.z*pi.z < limit.z)
      {
        if(blocks[index(pi)] >= 0)
          { return true; }
      
        if(tMax.x < tMax.y)
          {
            if(tMax.x < tMax.z)
              {
                pi.x += st.x;
                tMax.x += delta.x;
              }
            else
              {
                pi.z += st.z;
                tMax.z += delta.z;
              }
          }
        else if(tMax.y < tMax.z)
          {
            pi.y += st.y;
            tMax.y += delta.y;
          }
        else
          {
            pi.z += st.z;
            tMax.z += delta.z;
          }
      }
    return false;
}


HitPos intersect(vec3 p, vec3 d)
{
  ivec3 st = ivec3(sign(d));
  ivec3 limit = clamp(st * BLOCK_RAD, 0, BLOCK_RAD);
  vec3 tMax = (floor(p) - p + clamp(sign(d), 0, 1)) / d;
  vec3 delta = st / d;
  
    ivec3 pi = ivec3(p);
  
    HitPos hp;
    hp.type = -1;
    hp.normal = vec3(0);
    hp.texcoord = vec3(0);
    while(st.x*pi.x < limit.x && st.y*pi.y < limit.y && st.z*pi.z < limit.z)
      {
        hp.type = blocks[index(pi)];
        if(hp.type >= 0)
          {
            tMax -= delta;
            hp.intersect = p + d * max(tMax.x, max(tMax.y, tMax.z));
            hp.texcoord = hp.intersect - floor(hp.intersect);
            if(hp.normal.x != 0)
              { hp.texcoord.x = hp.texcoord.z; }
            else if(hp.normal.y != 0)
              { hp.texcoord.y = hp.texcoord.z; }
            hp.texcoord.z = hp.type;
            break;
          }
        else
          {
            if(tMax.x < tMax.y)
              {
                if(tMax.x < tMax.z)
                  {
                    pi.x += st.x;
                    tMax.x += delta.x;
                    hp.normal = vec3(-st.x, 0, 0);
                  }
                else
                  {
                    pi.z += st.z;
                    tMax.z += delta.z;
                    hp.normal = vec3(0, 0, -st.z);
                  }
              }
            else if(tMax.y < tMax.z)
              {
                pi.y += st.y;
                tMax.y += delta.y;
                hp.normal = vec3(0, -st.y, 0);
              }
            else
              {
                pi.z += st.z;
                tMax.z += delta.z;
                hp.normal = vec3(0, 0, -st.z);
              }
          }
      }
    return hp;
}


void main()
{
  // base pixel colour for image
  vec4 pixel = vec4(0.0, 0.0, 0.0, 0.0);
  
  // get index in global work group i.e x,y position
  ivec2 pixCoords = ivec2(gl_GlobalInvocationID.xy);
  vec2 screenCoords = vec2(pixCoords) / screenSize;
  vec3 rayDir = mix(mix(v00, v10, screenCoords.x),
                    mix(v01, v11, screenCoords.x), screenCoords.y);

  HitPos hp = intersect(camPos, rayDir);

  if(hp.type >= 0)
    {
      pixel = texture(blockTex, hp.texcoord);
      if(checkIntersect(hp.intersect + 0.00001*hp.normal, LIGHT_DIR))
        { pixel.xyz *= 0.5; }
    }

  // output to a specific pixel in the image
  imageStore(imgOut, pixCoords, pixel);
}
