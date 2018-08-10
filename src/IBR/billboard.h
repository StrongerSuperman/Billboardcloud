#ifndef BILLBOARD_H
#define BILLBOARD_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/shader.h"
#include "core/texture.h"
#include "core/glfw.h"
#include "core/config.h"
#include "object/quad.h"
#include "object/object.h"
#include "rectangle.h"
#include <iostream>

/// the billboard's type is as follow:
/// 1.screen aligned
/// 2.world oriented
/// 3.view plane oriented
/// 4.view point oriented
/// 5.billboard clouds

/// note:
/// the pose of the Quad that the billboard use is :
/// it's perpendicular to the z-axis which is defined in the camera-view coodinate
/// and its center is in the world origin point in the world coodinate
class Billboard
{
public:
	glm::vec3 position;
	glm::vec4 rotate;
	glm::vec3 scale;
	GLuint texture;
	GLuint frameBuffer;
	Rect rectangle;
	bool isTest;

	Billboard(Glfw& _glfw)
		:glfw(_glfw),
		isTest(false),
		position(glm::vec3(0.0f, 0.0f, 0.0f)),
		rotate(glm::vec4(0.0f, 1.0f, 0.0f, glm::radians(0.0f))),
		scale(glm::vec3(1.0f, 1.0f, 1.0f))
	{
	}

	Billboard(Rect _rectangle, Glfw& _glfw)
		:glfw(_glfw),
		rectangle(_rectangle),
		isTest(false),
		position(glm::vec3(0.0f, 0.0f, 0.0f)),
		rotate(glm::vec4(0.0f, 1.0f, 0.0f, glm::radians(0.0f))),
		scale(glm::vec3(1.0f, 1.0f, 1.0f))
	{
	}

	~Billboard()
	{
		glDeleteFramebuffers(1, &frameBuffer);
	}

	void render(Shader& shader, std::string mode)
	{
		if (mode == "view point oriented")
		{
			glm::mat4 model;
			model = glm::translate(model, position);
			model = glm::rotate(model, rotate.w, glm::vec3(rotate.x, rotate.y, rotate.z));
			model = glm::scale(model, scale);

			shader.use();
			shader.setMat4("projection", glfw.mainCamProjection);
			shader.setMat4("view", glfw.mainCamView);
			shader.setMat4("model", model);
			shader.setVec3("billboardCenter", rectangle.center);
			shader.setVec2("billboardSize", glm::vec2(rectangle.axisXLength / 2, rectangle.axisYLength / 2));
			shader.setBool("isTest", isTest);
			shader.setInt("mode", 0);

			quad.render(shader, texture);
		}

		else if (mode == "bbc")
		{
			glm::mat4 model;
			model = glm::translate(model, position);
			model = glm::rotate(model, rotate.w, glm::vec3(rotate.x, rotate.y, rotate.z));
			model = glm::scale(model, scale);

			shader.use();
			shader.setMat4("projection", glfw.mainCamProjection);
			shader.setMat4("view", glfw.mainCamView);
			shader.setMat4("model", model);
			shader.setVec3("billboardCenter", rectangle.center);
			shader.setVec2("billboardSize", glm::vec2(rectangle.axisXLength / 2, rectangle.axisYLength / 2));
			shader.setVec3("axisX", rectangle.axisX);
			shader.setVec3("axisY", rectangle.axisY);
			shader.setBool("isTest", isTest);
			shader.setInt("mode", 1);

			quad.render(shader, texture);
		}
		else if (mode == "imposter")
		{
			quad.render(shader, texture);
		}
	}

	void renderTotexture(Object& object, Shader& shader, glm::vec3& _position, glm::vec4& _rotate, glm::vec3& _scale)
	{
		setTextureFromFrameBuffer(IMG_WIDTH, IMG_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
		glViewport(0, 0, IMG_WIDTH, IMG_HEIGHT);

		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)IMG_WIDTH / (float)IMG_HEIGHT, 0.1f, 1000.0f);
		Camera camera(glm::vec3(0.0f, 0.0f, 4.7f));
		glm::mat4 view = camera.getViewMatrix();
		glm::mat4 model;
		model = glm::translate(model, _position);
		model = glm::rotate(model, _rotate.w, glm::vec3(_rotate.x, _rotate.y, _rotate.z));
		model = glm::scale(model, _scale);
		glm::vec3 lightPos = glm::vec3(10, 15, 10);
		shader.use();
		shader.setMat4("projection", projection);
		shader.setMat4("view", view);
		shader.setMat4("model", model);
		shader.setVec3("lightPos", lightPos);
		shader.setVec3("viewPos", camera.position);
		shader.setBool("blinn", false);
		shader.setBool("alphaTest", false);

		object.renderToTexture(shader);
	}

	void setTextureFromFrameBuffer(int textureWidth, int textureHeight)
	{
		auto tmp = loadTextureFromFrameBuffer(textureWidth, textureHeight);
		frameBuffer = tmp.first;
		texture = tmp.second;
	}

private:
	Quad quad;
	Glfw& glfw;
};

#endif