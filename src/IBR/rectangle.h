#ifndef RECTANGLE_H
#define RECTANGLE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Rect
{
public:
	glm::vec3 p0;      // top right
	glm::vec3 p1;      // bottom right
	glm::vec3 p2;      // bottom left
	glm::vec3 p3;      // top left 
	glm::vec3 center;
	glm::vec3 axisX;
	glm::vec3 axisY;
	float axisXLength;
	float axisYLength;

	Rect()
		:p0(glm::vec3(1.0f, 1.0f, 0.0f)),
		p1(glm::vec3(1.0f, -1.0f, 0.0f)),
		p2(glm::vec3(-1.0f, -1.0f, 0.0f)),
		p3(glm::vec3(-1.0f, 1.0f, 0.0f))
	{
		update();
	}

	Rect(glm::vec3 _p0, glm::vec3 _p1, glm::vec3 _p2, glm::vec3 _p3)
		:p0(_p0),
		p1(_p1),
		p2(_p2),
		p3(_p3)
	{
		update();
	}

	void update()
	{
		glm::vec3 tmp = p2 - p0;
		center = glm::vec3(p0.x + tmp.x / 2, p0.y + tmp.y / 2, p0.z + tmp.z / 2);
		axisX = glm::normalize(p1 - p0);
		axisY = glm::normalize(p2 - p1);
		axisXLength = glm::length(p1 - p0);
		axisYLength = glm::length(p2 - p1);
	}

	/// get the mat 
	/// trans the point in the world coordinate to the point in the plane_rectangle_pos coordinate
	/// plane rectangle_pos coordinate define as follow: 
	/// z-axis aligned to the plane normal
	/// x-axis aligned to the direction of the plane_rectangle[1]-plane_rectangle[0]
	/// y-axis aligned to the cross_product(z-axis,x-axis) direction
	glm::mat4 getTransMat(glm::vec3 normal) const
	{
		glm::mat4 mat;
		glm::vec3 x_aixs = axisX;
		glm::vec3 z_aixs = normal;
		glm::vec3 y_aixs = glm::normalize(glm::cross(z_aixs, x_aixs));
		mat[0] = glm::vec4(x_aixs, 0);
		mat[1] = glm::vec4(y_aixs, 0);
		mat[2] = glm::vec4(z_aixs, 0);
		mat[3] = glm::vec4(0, 0, 0, 1);
		mat = glm::transpose(mat);
		mat = glm::translate(mat, -center);

		return mat;
	}
};

#endif // !RECTANGLE_H
