#ifndef SPHEREPOINTSAMPLING_H
#define SPHEREPOINTSAMPLING_H

#include <glm/glm.hpp>
#include <vector>

#ifndef pi
#define pi 3.1415926f
#endif // !pi

/// note: 
/// the point is generated in the longtitude-latitude coordinate,
/// the origin point of the latitude and longtitude is in the up point and front point respectively, 
/// the sphere is in the Euler coordinate, the three aixs is the same as the world coordinate in opengl 
/// the mainCamera looks forward to the center of the sphere through the origin point of the longtitude
/// the stored order of the generated sphere surface point as follow:
/// longtitude coordinate expand from small to large
/// latitude coordinate expand from small to large
static std::vector<glm::vec3> gen_lat_lon_sphere_point(glm::vec3 center, float radius, int latNum, int lonNum)
{
	std::vector<glm::vec3> points;

	glm::vec3 upPoint = center + glm::vec3(0, radius, 0);

	// add the point from the first latitude circle line to the last
	// there are longtitudeLinePointNum/2*latitudeLinePointNum points in the upper-half sphere
	// and the same points in the upper-half sphere
	float latLineGap = 2 * radius / (latNum + 1);
	for (int i = 1; i <= latNum; i++)
	{
		glm::vec3 latLineCenter = upPoint - glm::vec3(0, latLineGap*i, 0);
		float curRadius = glm::sqrt(radius*radius - glm::distance(center, latLineCenter));
		for (int j = 0; j < lonNum; j++)
		{
			float lonLineGapArc = 2 * pi / lonNum;
			float currentArc = lonLineGapArc * j;
			glm::vec3 point = latLineCenter + glm::vec3(glm::sin(currentArc)*curRadius, 0, glm::cos(currentArc)*curRadius);
			points.emplace_back(point);
		}
	}
	return points;
}

/// Fibonacci Sphere with specific point number
static std::vector<glm::vec3> gen_fibonacci_sphere_point(glm::vec3 center, float radius, int k)
{
	std::vector<glm::vec3> points;
	float golden_angle = pi * (3 - sqrt(5));
	float r, theta, z;
	for (int i = 0; i < k; i++)
	{
		theta = golden_angle * i;
		z = (1 - 2.*i / k)*(1 - 1. / k);
		r = sqrt(1 - z * z);

		glm::vec3 point;
		point.x = r * cos(theta)*radius;
		point.y = r * sin(theta)*radius;
		point.z = z * radius;
		point += center;

		points.emplace_back(point);
	}
	return points;
}

static std::vector<glm::vec3> gen_min_discrete_energy_sphere_point(glm::vec3 center, float radius, int k)
{
	// to do...
}

#endif // !SPHEREPOINTSAMPLING_H