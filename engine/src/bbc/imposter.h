#ifndef IMPOSTER_H
#define IMPOSTER_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "math/spherepointsampling.h"
#include "core/camera.h"
#include "core/shader.h"
#include "core/glfw.h"
#include "core/config.h"
#include "billboard.h"
#include <vector>

#ifndef pi
#define pi 3.1415926f
#endif // !pi

class Imposter
{
public:
	glm::vec3 position;
	glm::vec4 rotate;
	glm::vec3 scale;
	std::vector<glm::mat4> rotMats;
	Billboard bb;
	Glfw& glfw;

	Imposter(Glfw& _glfw)
		:glfw(_glfw),
		camColumnNum(8),
		camRowNum(8),
		bb(_glfw),
		position(glm::vec3(0.0f, 0.0f, 0.0f)),
		rotate(glm::vec4(0.0f, 1.0f, 0.0f, glm::radians(1.0f))),
		scale(glm::vec3(1.0f, 1.0f, 1.0f))
	{
		calcuViewsRotMats();
	}

	void render(Shader& shader)
	{
		glm::mat4 model;
		model = glm::translate(model, position);
		model = glm::rotate(model, rotate.w, glm::vec3(rotate.x, rotate.y, rotate.z));
		model = glm::scale(model, scale);
		// display the billboardCloud according to the nearest billboard
		int index = calcuNearestViewIndex(glfw.mainCamPos - position);
		model = model * rotMats[index];

		// every camera frameTexture is stored in the order as follow:
		// the indexFromOne is accordance with the camera stored order
		//  (0,1)！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！(1,1)
		//       |  1 |  2 |    |    |    |    |    |
		//       |    |    |    |    |    |    |    |
		//       ！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
		//       |    |    |    |    |    |    |    |
		//       |    |    |    |    |    |    |    |
		//       ！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
		//       |    |    |    |    |    |    |    |
		//       |    |    |    |    |    |    |    |
		//       ！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！
		//       |    |    |    |    |    |    |    |
		//       |    |    |    |    |    |    |    |
		//  (0,0)！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！！(1,0)
		int indexFromOne = index + 1;
		int rowIndex = indexFromOne % camColumnNum == 0 ? indexFromOne / camColumnNum : indexFromOne / camColumnNum + 1;
		int columnIndex = indexFromOne - (rowIndex - 1)*camColumnNum;
		float xCoordOffset = (columnIndex - 1)*1.0f / camColumnNum;
		float yCoordOffset = (camRowNum - rowIndex)*1.0f / camRowNum;

		shader.use();
		shader.setMat4("projection", glfw.mainCamProjection);
		shader.setMat4("view", glfw.mainCamView);
		shader.setMat4("model", model);
		shader.setFloat("xCoordScale", 1.0f / camColumnNum);
		shader.setFloat("yCoordScale", 1.0f / camRowNum);
		shader.setFloat("xCoordOffset", xCoordOffset);
		shader.setFloat("yCoordOffset", yCoordOffset);

		bb.render(shader, "imposter");
	}

	void renderTotexture(Object& object, Shader& shader, glm::vec3& _position, glm::vec4& _rotate, glm::vec3& _scale)
	{
		bb.setTextureFromFrameBuffer(IMG_WIDTH, IMG_HEIGHT);
		glBindFramebuffer(GL_FRAMEBUFFER, bb.frameBuffer);
		glViewport(0, 0, IMG_WIDTH, IMG_HEIGHT);

		glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)IMG_WIDTH / (float)IMG_HEIGHT, 0.1f, 1000.0f);
		glm::mat4 model;
		model = glm::translate(model, _position);
		model = glm::rotate(model, _rotate.w, glm::vec3(_rotate.x, _rotate.y, _rotate.z));
		model = glm::scale(model, _scale);

		shader.use();
		shader.setMat4("projection", projection);
		shader.setMat4("model", model);
		shader.setVec3("lightPos", glm::vec3(10, 15, 10));
		shader.setBool("blinn", false);
		shader.setBool("alphaTest", false);
		for (int i = 0; i < positions.size(); i++)
		{
			int indexFromOne = i + 1;
			int rowIndex = indexFromOne % camColumnNum == 0 ? indexFromOne / camColumnNum : indexFromOne / camColumnNum + 1;
			int columnIndex = indexFromOne - (rowIndex - 1)*camColumnNum;
			float leftTopCenterX = -1 + 2.0 / camColumnNum / 2;
			float leftTopCenterY = 1 - 2.0 / camRowNum / 2;
			float xOffset = leftTopCenterX + (columnIndex - 1)*2.0 / camColumnNum;
			float yOffset = leftTopCenterY - (rowIndex - 1)*2.0 / camColumnNum;

			shader.setMat4("view", viewMats[i]);
			shader.setFloat("xScale", 1.0 / camColumnNum);
			shader.setFloat("yScale", 1.0 / camRowNum);
			shader.setFloat("xOffset", xOffset);
			shader.setFloat("yOffset", yOffset);
			shader.setVec3("viewPos", positions[i]);

			object.renderToTexture(shader);
		}
	}

private:
	int camColumnNum;
	int camRowNum;
	std::vector<glm::vec3> positions;
	std::vector<glm::mat4> viewMats;

	// render the billboard according to :
	// whose angle between the vector(camPos to billboard's Quad Center) 
	// and the vector(perpendicular to the billboard Quad) is the minimum
	int calcuNearestViewIndex(const glm::vec3 viewDirection)
	{
		float minAngleOfCosValue = -1;   // min angle have the max cos value
		int minAngleIndex = 0;
		for (int i = 0; i < positions.size(); i++)
		{
			float tmp = glm::dot(glm::normalize(viewDirection), glm::normalize(positions[i]));
			if (minAngleOfCosValue < tmp)
			{
				minAngleOfCosValue = tmp;
				minAngleIndex = i;
			}
		}
		return minAngleIndex;
	}

	void calcuViewsRotMats()
	{
		glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
		float radius = 4.7f;
		positions = gen_lat_lon_sphere_point(center, radius, camColumnNum, camRowNum);

		for (auto& point : positions)
		{
			viewMats.emplace_back(glm::lookAt(point, center, glm::vec3(0, 1, 0)));

			// note: the rotMat is calculated according to the billboard's default Quad pose(perpendicular to the z-aixs)
			// calculate the two sphere space coord: theta、phi
			glm::mat4 rotMat;
			float theta = 0.0f;
			float phi = 0.0f;
			// separate the point to the corresponding quadrant
			// the range of theta is [0,2*PI], the range of phi is [-PI/2,PI/2]
			if (point.z < 0)
			{
				if (point.x > 0)
				{
					theta = glm::atan(-point.z / point.x) + pi / 2;
				}
				else
				{
					theta = glm::atan(-point.x / -point.z) + pi;
				}
			}
			else
			{
				theta = glm::atan(point.x / point.z);
			}
			phi = glm::asin(point.y / radius);

			rotMat = glm::rotate(rotMat, theta, glm::vec3(0, 1, 0));
			rotMat = glm::rotate(rotMat, -phi, glm::vec3(1, 0, 0));
			rotMats.emplace_back(rotMat);
		}
	}
};

#endif