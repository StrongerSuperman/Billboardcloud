#ifndef OBJECT_H
#define OBJECT_H

#include "core/shader.h"
#include "core/mesh.h"
#include "core/model.h"
#include "core/glfw.h"
#include <iostream>

class Object
{
public:
	Mesh * const mesh;
	Model* const model;
	glm::vec3 position;
	glm::vec4 rotate;
	glm::mat4 rotateMat;
	glm::vec3 scale;
	Glfw& glfw;

	Object(Model* _model, Glfw& _glfw)
		:position(glm::vec3(0.0f, 0.0f, 0.0f)),
		rotate(0.0f, 1.0f, 0.0f, glm::radians(0.0f)),
		rotateMat(glm::mat4(1.0f)),
		scale(1.0f, 1.0f, 1.0f),
		model(_model),
		glfw(_glfw),
		mesh(nullptr),
		renderMode("model")
	{
	}

	Object(Mesh* _mesh, Glfw& _glfw)
		:mesh(_mesh),
		glfw(_glfw),
		model(nullptr),
		renderMode("mesh")
	{
	}

	void render(Shader& shader)
	{
		if (renderMode == "model")
		{
			glm::mat4 _model;
			_model = glm::translate(_model, position);
			_model = _model * rotateMat;
			_model = glm::rotate(_model, rotate.w, glm::vec3(rotate.x, rotate.y, rotate.z));
			_model = glm::scale(_model, scale);

			shader.use();
			shader.setMat4("projection", glfw.mainCamProjection);
			shader.setMat4("view", glfw.mainCamView);
			shader.setMat4("model", _model);
			shader.setVec3("lightPos", glfw.lightPos);
			shader.setVec3("viewPos", glfw.mainCamPos);
			shader.setBool("blinn", glfw.blinn);

			model->render(shader);
		}

		if (renderMode == "mesh")
		{
			glm::mat4 _model;
			_model = glm::translate(_model, position);
			_model = _model * rotateMat;
			_model = glm::rotate(_model, rotate.w, glm::vec3(rotate.x, rotate.y, rotate.z));
			_model = glm::scale(_model, scale);

			shader.use();
			shader.setMat4("projection", glfw.mainCamProjection);
			shader.setMat4("view", glfw.mainCamView);
			shader.setMat4("model", _model);
			shader.setVec3("lightPos", glfw.lightPos);
			shader.setVec3("viewPos", glfw.mainCamPos);
			shader.setBool("blinn", glfw.blinn);

			mesh->render(shader);
		}
	}

	void renderToTexture(const Shader& shader)
	{
		if (renderMode == "model")
		{
			model->render(shader);
		}
		if (renderMode == "mesh")
		{
			mesh->render(shader);
		}
	}

private:
	const std::string renderMode;
};
#endif // !OBJECT_H
