#ifndef TEXT_H
#define TEXT_H

#include <ft2build.h>
#include FT_FREETYPE_H
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/shader.h"
#include "core/glfw.h"
#include <map>
#include <iostream>


/// Holds all state information relevant to a character as loaded using FreeType
struct Character
{
	GLuint textureID;    // ID handle of the glyph texture
	glm::ivec2 size;     // Size of glyph
	glm::ivec2 bearing;  // Offset from baseline to left/top of glyph
	GLuint advance;      // Horizontal offset to advance to next glyph
};

class TextASCII
{
public:
	glm::vec3 position;
	glm::vec4 rotate;
	glm::vec3 scale;
	glm::vec3 color;
	std::string text;
	float size;
	Glfw& glfw;

	TextASCII(std::string _fontPath, Glfw& _glfw)
		:position(0.0f, 0.0f, 0.0f),
		rotate(glm::vec4(0.0f, 1.0f, 0.0f, glm::radians(0.0f))),
		scale(1.0f, 1.0f, 1.0f),
		color(glm::vec3(0.5, 0.8f, 0.2f)),
		text("TEXT"),
		size(0.01f),
		fontPath(_fontPath),
		glfw(_glfw)
	{
		setup();
	}

	~TextASCII()
	{
		glDeleteBuffers(1, &VAO);
	}

	void render(Shader &shader)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		float x = 0.0;
		float y = 0.0f;
		glm::mat4 model;
		model = glm::translate(model, position);
		model = glm::rotate(model, rotate.w, glm::vec3(rotate.x, rotate.y, rotate.z));
		model = glm::scale(model, scale);

		shader.use();
		shader.setMat4("projection", glfw.mainCamProjection);
		shader.setMat4("view", glfw.mainCamView);
		shader.setMat4("model", model);
		shader.setVec3("textColor", color);

		glActiveTexture(GL_TEXTURE0);
		glBindVertexArray(VAO);
		// retrive the code in the font text 
		std::string::const_iterator c;
		for (c = text.begin(); c != text.end(); c++)
		{
			Character ch = Characters[*c];

			float xpos = x + ch.bearing.x * size;
			float ypos = y - (ch.size.y - ch.bearing.y) * size;

			float w = ch.size.x * size;
			float h = ch.size.y * size;
			// update the VBO according to the corresponding code
			float vertices[6][4] = {
			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos,     ypos,       0.0, 1.0 },
			{ xpos + w, ypos,       1.0, 1.0 },

			{ xpos,     ypos + h,   0.0, 0.0 },
			{ xpos + w, ypos,       1.0, 1.0 },
			{ xpos + w, ypos + h,   1.0, 0.0 }
			};
			glBindTexture(GL_TEXTURE_2D, ch.textureID);
			glBindBuffer(GL_ARRAY_BUFFER, VBO);
			glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
			glBindBuffer(GL_ARRAY_BUFFER, 0);
			glDrawArrays(GL_TRIANGLES, 0, 6);
			// update the position to the origin of the character(note unit is 1/64 pixels)
			x += (ch.advance >> 6) * size; // bit offset 6 units to acquire single unit of pixel(2^6 = 64)
		}
		glBindVertexArray(0);
		glBindTexture(GL_TEXTURE_2D, 0);

		glDisable(GL_BLEND);
	}

private:
	std::string fontPath;
	std::map<GLchar, Character> Characters;
	GLuint VAO, VBO;

	void setup()
	{
		// FreeType
		FT_Library ft;

		// All functions return a value different than 0 whenever an error occurred
		if (FT_Init_FreeType(&ft))
			std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;

		// Load font as face
		FT_Face face;
		if (FT_New_Face(ft, fontPath.c_str(), 0, &face))
			std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;

		// Set size to load glyphs as
		FT_Set_Pixel_Sizes(face, 0, 48);

		// Disable byte-alignment restriction
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		// Load first 128 characters of ASCII set
		for (GLubyte c = 0; c < 128; c++)
		{
			// Load character glyph 
			if (FT_Load_Char(face, c, FT_LOAD_RENDER))
			{
				std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
				continue;
			}

			// Generate texture
			GLuint texture;
			glGenTextures(1, &texture);
			glBindTexture(GL_TEXTURE_2D, texture);
			glTexImage2D(
				GL_TEXTURE_2D,
				0,
				GL_RED,
				face->glyph->bitmap.width,
				face->glyph->bitmap.rows,
				0,
				GL_RED,
				GL_UNSIGNED_BYTE,
				face->glyph->bitmap.buffer
			);

			// Set texture options
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

			// Now store character for later use
			Character character = {
				texture,
				glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				face->glyph->advance.x
			};
			Characters.insert(std::pair<GLchar, Character>(c, character));
		}
		glBindTexture(GL_TEXTURE_2D, 0);

		// Destroy FreeType once we're finished
		FT_Done_Face(face);
		FT_Done_FreeType(ft);

		// Configure VAO/VBO for texture quads
		glGenVertexArrays(1, &VAO);
		glGenBuffers(1, &VBO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);
	}
};

#endif // !TEXT_H
