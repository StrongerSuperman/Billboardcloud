#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;

out VS_OUT {
    vec3 FragPos;
    vec3 Normal;
    vec2 TexCoords;
} vs_out;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float xScale;
uniform float yScale;
uniform float xOffset;
uniform float yOffset;

void main()
{
    vs_out.FragPos = aPos;
    vs_out.Normal = aNormal;
    vs_out.TexCoords = aTexCoords;

	vec4 pos = projection * view * model * vec4(aPos, 1);

	gl_Position = vec4(pos.x / pos.w * xScale + xOffset, 
	                   pos.y  / pos.w * yScale + yOffset, 
					   pos.z / pos.w, 
					   1.0);
}