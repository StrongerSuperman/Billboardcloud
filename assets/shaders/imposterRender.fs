#version 330 core
out vec4 FragColor;

in vec2 TexCoord;

// texture samplers
uniform sampler2D texture1;

void main()
{
	FragColor = texture(texture1, TexCoord);

	// ignore the background
	if (FragColor.r == 0 && FragColor.g == 0 && FragColor.b == 0)
    {  
       discard;  
    }
}