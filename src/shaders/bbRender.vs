#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform int mode;
uniform vec3 billboardCenter;
uniform vec2 billboardSize;
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec3 axisX;
uniform vec3 axisY;

out vec2 TexCoord;

void main()
{
    // view point oriented
    if (mode == 0)
	{
	   vec3 CameraRight_worldspace = vec3(view[0][0], view[1][0], view[2][0]);
	   vec3 CameraUp_worldspace =  vec3(view[0][1], view[1][1], view[2][1]);

	   vec3 vertexPosition_worldspace = billboardCenter +
	                                    CameraRight_worldspace * aPos.x * billboardSize.x+ 
	                                    CameraUp_worldspace * aPos.y * billboardSize.y;

	   gl_Position = projection * view * model * vec4(vertexPosition_worldspace, 1.0);

	   TexCoord = aTexCoord;
	}

	// bbc
	else if (mode == 1)
	{
	   vec3 aPos = billboardCenter + axisX * aPos.x * billboardSize.x+ axisY * aPos.y * billboardSize.y;

	   gl_Position = projection * view * model * vec4(aPos, 1.0);

	   TexCoord = aTexCoord;
	}
}