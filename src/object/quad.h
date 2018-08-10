#ifndef QUAD_H
#define QUAD_H

#include <glad/glad.h>
#include "core/shader.h"

class Quad
{
public:
	Quad()
	{
		setup();
	}

	~Quad()
	{
		glDeleteBuffers(1, &VAO);
	}

	void render(const Shader& shader, const GLuint& texture)
	{
		glActiveTexture(GL_TEXTURE0);

		glUniform1i(glGetUniformLocation(shader.ID, "texture1"), 0);
		glBindTexture(GL_TEXTURE_2D, texture);

		// draw mesh
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
		glBindVertexArray(0);

		// always good practice to set everything back to defaults once configured.
		glActiveTexture(GL_TEXTURE0);
	}

private:

	/*  Render data  */
	GLuint VAO, VBO, EBO;

	void setup()
	{
		// set up vertex data (and buffer(s)) and configure vertex attributes
		float vertices[] = {
			// positions        // texture coords
			1.0f, 1.0f, 0.0f,    1.0f, 1.0f,  // top right
			1.0f, -1.0f, 0.0f,   1.0f, 0.0f,  // bottom right
			-1.0f, -1.0f, 0.0f,  0.0f, 0.0f,  // bottom left
			-1.0f, 1.0f, 0.0f,   0.0f, 1.0f   // top left 
		};

		unsigned int indices[] = {
			0, 1, 3, // first triangle
			1, 2, 3  // second triangle
		};
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);

		glBindVertexArray(VAO);

		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

		// position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
		glEnableVertexAttribArray(0);
		// texture coord attribute
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
	}
};

#endif