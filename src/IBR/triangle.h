#ifndef TRIANGLE_H
#define TRIANGLE_H

#include <glm/glm.hpp>

class Triangle
{
public:
	Triangle()
	{
	}

	Triangle(glm::vec3 _p0, glm::vec3 _p1, glm::vec3 _p2, float _distance, glm::vec3 _normal, glm::vec3 _indices)
		:p0(_p0), 
		p1(_p1),
		p2(_p2), 
		distance(_distance), 
		normal(_normal), 
		indices(_indices)
	{
		calcuArea();
		calcuCentriod();
	}

	/// copy constructor
	Triangle(const Triangle& triangle)
	{
		p0 = triangle.p0;
		p1 = triangle.p1;
		p2 = triangle.p2;
		distance = triangle.distance;
		normal = triangle.normal;
		indices = triangle.indices;
		area = triangle.area;
		centriod = triangle.centriod;
		index = triangle.index;
	}

	Triangle& operator=(const Triangle& triangle)
	{
		p0 = triangle.p0;
		p1 = triangle.p1;
		p2 = triangle.p2;
		distance = triangle.distance;
		normal = triangle.normal;
		indices = triangle.indices;
		area = triangle.area;
		centriod = triangle.centriod;
		index = triangle.index;
		return *this;
	}

	float getArea() const
	{
		return area;
	}

	glm::vec3 getCentriod() const
	{
		return centriod;
	}

	glm::vec3 p0;
	glm::vec3 p1;
	glm::vec3 p2;
	float distance;      // distance from origin in normal direction
	glm::vec3 normal;
	glm::vec3 indices;   // indicesIndex in the origin mesh indices
	int index;         // index in the triangles(for original bbc algorithm fail-safe mode use)

private:
	float area;
	glm::vec3 centriod;

	void calcuCentriod()
	{
		centriod = glm::vec3((p0.x + p1.x + p2.x) / 3, (p0.y + p1.y + p2.y) / 3, (p0.z + p1.z + p2.z / 3));
	}
	void calcuArea()
	{
		float a = glm::length(p0 - p1);
		float b = glm::length(p2 - p1);
		float c = glm::length(p0 - p2);
		float d = (a + b + c) / 2;
		area = glm::sqrt(d*(d - a)*(d - b)*(d - c));
	}
};
#endif // !TRIANGLE_H
