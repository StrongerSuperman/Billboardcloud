#ifndef BOUNDINGSPHERE_H
#define BOUNDINGSPHERE_H

#include <glm/glm.hpp>
#include "math/spherepointsampling.h"
#include "triangle.h"
#include "plane.h"
#include <vector>

class BoundingSphere
{
public:
	glm::vec3 center;
	float radius;

	BoundingSphere()
	{
	}

	BoundingSphere(const std::vector<Triangle>& triangles)
	{
		init(triangles);
	}

	void init(const std::vector<Triangle>& triangles)
	{
		calcuCenter(triangles);
		calcuRadius(triangles);
	}

	/// calculate the tangent planes by specify the k points on the sphere surface
	std::vector<Plane> calcuKTangenPlanes(int k)
	{
		std::vector<Plane> tangenPlanes;
		// currently we employ the "Fibonacci sphere point" instead of the "minimal discrete energy method" mentioned in the paper
		std::vector<glm::vec3> samplePoints = gen_fibonacci_sphere_point(center, radius, k);
		for (auto point : samplePoints)
		{
			glm::vec3 normal = glm::normalize(point - center);
			if (glm::dot(point, normal) < 0)
			{
				normal = -normal;
			}
			float distance = glm::abs(glm::dot(point, normal));
			Plane p(normal, distance);
			tangenPlanes.emplace_back(p);
		}
		return tangenPlanes;
	}

private:
	void calcuCenter(const std::vector<Triangle>& triangles)
	{
		float xTmp = 0.0f;
		float yTmp = 0.0f;
		float zTmp = 0.0f;
		for (auto triangle : triangles)
		{
			xTmp += triangle.getCentriod().x;
			yTmp += triangle.getCentriod().y;
			zTmp += triangle.getCentriod().z;
		}
		center = glm::vec3(xTmp / triangles.size(), yTmp / triangles.size(), zTmp / triangles.size());
	}

	void calcuRadius(const std::vector<Triangle>& triangles)
	{
		for (auto& triangle : triangles)
		{
			float maxTmp;
			float tmp0 = glm::length(triangle.p0 - center);
			float tmp1 = glm::length(triangle.p1 - center);
			float tmp2 = glm::length(triangle.p2 - center);
			maxTmp = tmp0 > tmp1 ? tmp0 : tmp1;
			maxTmp = maxTmp > tmp2 ? maxTmp : tmp2;
			radius = radius > maxTmp ? radius : maxTmp;
		}
	}
};
#endif // !BOUNDINGSPHERE_H
