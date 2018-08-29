#version 330

#define LINE_W 0.05

smooth in vec2 texCoords;
smooth in float fog;
out vec4 fragColor;

void main()
{
  vec4 color;
  if(abs(texCoords.x) > (1.0-LINE_W) || abs(texCoords.y) > (1.0-LINE_W))
    { color = vec4(1.0, 0.0, 0.0, 1.0); }
  else
    { color = vec4(0.0, 0.0, 0.0, 0.0); }
  
  fragColor = vec4(1.0,0.0,0.0,1.0); //mix(vec4(0.229, 0.657, 0.921, 1.0), color, clamp(fog, 0.0, 1.0));
  
  /* if(abs(texCoords.x) > (1.0-LINE_W) || abs(texCoords.y) > (1.0-LINE_W)) */
  /*   { fragColor = mix(vec4(1.0, 0.0, 0.0, 1.0), vec4(0.229, 0.657, 0.921, 1.0), clamp(fog, 0.0, 1.0)); } */
  /* else */
  /*   { fragColor = vec4(0.0, 0.0, 0.0, 0.0); } */
  

  //fragColor = vec4(1.0, 0.0, 0.0, 1.0);
}
