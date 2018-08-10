#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform float xCoordScale;
uniform float yCoordScale;
uniform float xCoordOffset;
uniform float yCoordOffset;

out vec2 TexCoord;

void main()
{
	gl_Position = projection * view * model * vec4(aPos, 1.0);

	TexCoord.x = aTexCoord.x * xCoordScale + xCoordOffset;
	TexCoord.y = aTexCoord.y * yCoordScale + yCoordOffset;
}