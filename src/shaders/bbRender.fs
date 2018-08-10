#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// texture samplers
uniform sampler2D texture1;
uniform bool isTest;

void main()
{
    if(!isTest)
	{
	   FragColor = texture(texture1, TexCoord);

	   // ignore the background
	   if (FragColor.a == 0)
       {  
          discard;  
       }
	}
	else
	{
		FragColor=vec4(0.8, 0.5, 0.2, 0.3);
	}
}