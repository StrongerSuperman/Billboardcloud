#ifndef TEXTURE_H
#define TEXTURE_H

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STBI_MSC_SECURE_CRT
#define STBRP_STATIC
#define STB_RECT_PACK_IMPLEMENTATION

#include <glad/glad.h>
#include <stb/stb_image.h>
#include <stb/stb_image_write.h>
#include <stb/stb_rect_pack.h>
#include "config.h"
#include <vector>
#include <iostream>

//----------------------------------------------------------------------------------------------------------
// writing 2D texture into image file
//----------------------------------------------------------------------------------------------------------

/// comp=1, format = GL_RED
/// comp=2, format = GL_RG
/// comp=3, format = GL_RGB
/// comp=4, format = GL_RGBA
static bool writeToPng(const char *filename, const void *data, int w, int h, int comp = 4)
{
	if (stbi_write_png(filename, w, h, comp, data, w*comp) == 1)
		return true;
	else
		return false;
}

//----------------------------------------------------------------------------------------------------------
// packing 2D textures into txeture atlas
//----------------------------------------------------------------------------------------------------------

static bool writeToTextureAtlasPng(const char *filename,
	const std::vector<unsigned int>& Width,
	const std::vector<unsigned int>& height,
	const std::vector<BYTE*>& data,
	int packWidth,
	int packHeight)
{
	// the method stbrp_pack_rects use "Skyline Bottom-Left algorithm"
	stbrp_context context;
	std::vector<stbrp_rect> rects(Width.size());
	for (int i = 0; i < Width.size(); i++)
	{
		rects[i].id = i;
		rects[i].w = Width[i];
		rects[i].h = height[i];
		rects[i].x = 0;
		rects[i].y = 0;
		rects[i].was_packed = 0;
	}
	const int nodeCount = packWidth * 2;
	std::vector<stbrp_node> nodes(nodeCount);
	stbrp_init_target(&context, packWidth, packHeight, &nodes[0], nodeCount);
	stbrp_setup_allow_out_of_mem(&context, 1);
	stbrp_pack_rects(&context, &rects[0], rects.size());

	// insert the data into the corresponding position in the texture atlas
	// the following code need to be optimized due to its bad running efficiency
	BYTE* pack = (BYTE *)malloc(packWidth*packHeight * 4);
	for (int i = 0; i < data.size(); i++)
	{
		if (rects[i].was_packed)
		{
			for (int j = 0; j < rects[i].h; j++)
			{
				for (int k = 0; k < rects[i].w; k++)
				{
					pack[((rects[i].y + j)*packWidth + rects[i].x + k) * 4] = data[i][(j*rects[i].w + k)*4];
					pack[((rects[i].y + j)*packWidth + rects[i].x + k) * 4 + 1] = data[i][(j*rects[i].w + k)*4+1];
					pack[((rects[i].y + j)*packWidth + rects[i].x + k) * 4 + 2] = data[i][(j*rects[i].w + k)*4+2];
					pack[((rects[i].y + j)*packWidth + rects[i].x + k) * 4 + 3] = data[i][(j*rects[i].w + k)*4+3];
				}
			}
		}
	}
	if (writeToPng(filename, pack, packWidth, packHeight))
	{
		free(pack);
		return true;
	}
	else
	{
		free(pack);
		return false;
	}
}

//----------------------------------------------------------------------------------------------------------
// loading a 2D texture from file
//----------------------------------------------------------------------------------------------------------

static unsigned int loadTexture(const char *path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

static unsigned int loadTexture(const char *path, const std::string &directory)
{
	std::string filename = std::string(path);
	filename = directory + '/' + filename;

	unsigned int textureID;
	glGenTextures(1, &textureID);

	int width, height, nrComponents;
	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &nrComponents, 0);
	if (data)
	{
		GLenum format;
		if (nrComponents == 1)
			format = GL_RED;
		else if (nrComponents == 3)
			format = GL_RGB;
		else if (nrComponents == 4)
			format = GL_RGBA;

		glBindTexture(GL_TEXTURE_2D, textureID);
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		stbi_image_free(data);
	}
	else
	{
		std::cout << "Texture failed to load at path: " << path << std::endl;
		stbi_image_free(data);
	}

	return textureID;
}

//----------------------------------------------------------------------------------------------------------
// set texture from fameBuffer(render to texture)
//----------------------------------------------------------------------------------------------------------

static std::pair<GLuint, GLuint> loadTextureFromFrameBuffer(int textureWidth, int textureHeight)
{
	GLuint frameBuffer;
	// The rendered framebuffer
	// note: must bind Framebuffer before create the texture we will render to
	glGenFramebuffers(1, &frameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);

	GLuint texture;
	glGenTextures(1, &texture);
	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, texture);
	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureWidth, textureHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0);   // set as "GL_RGBA" to store alpha channel
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glBindTexture(GL_TEXTURE_2D, 0);   // bind back !!!
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

	GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
	glDrawBuffers(1, DrawBuffers);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		std::cout << "ERROR::FRAMEBUFFER:: Framebuffer is not complete!" << std::endl;

	// Unbind Framebuffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return std::make_pair(frameBuffer, texture);
}

// -----------------------------------------------------------
// loads a cubemap texture from 6 individual texture faces
// order:
// +X (right)
// -X (left)
// +Y (top)
// -Y (bottom)
// +Z (front) 
// -Z (back)
// -----------------------------------------------------------

static unsigned int loadCubemap(const std::vector<std::string> faces)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrChannels;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}

#endif