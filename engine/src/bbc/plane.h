#ifndef PLANE_H
#define PLANE_H

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>

class Plane
{
public:
	Plane()
	{
	}

	Plane(glm::vec3 _normal, float _distance)
		:normal(_normal),
		distance(_distance)
	{
		calcuPlanePara();
	}

	/// copy constructor
	Plane(const Plane& _plane)
	{
		normal = _plane.normal;
		distance = _plane.distance;
		para = _plane.para;
	}

	Plane& operator=(const Plane& _plane)
	{
		normal = _plane.normal;
		distance = _plane.distance;
		para = _plane.para;
		return *this;
	}

	/// specific distance metric in the paper
	/// omit the division, since linearly scaling the metric does not influence the result
	float calcuTotalDistance(Triangle& t) const
	{
		return calcuPointDistance(t.p0) +calcuPointDistance(t.p1) +calcuPointDistance(t.p2);
	}

	/// calculate the max distance from a triangle
	float calcuMaxDistance(Triangle& t) const
	{
		float maxDist = 0.0f;
		float dist0 = calcuPointDistance(t.p0);
		float dist1 = calcuPointDistance(t.p1);
		float dist2 = calcuPointDistance(t.p2);
		maxDist = dist0 > dist1 ? dist0 : dist1;
		maxDist = maxDist > dist2 ? maxDist : dist2;
		return maxDist;
	}

	/// calculate the min distance from a triangle
	float calcuMinDistance(Triangle& t) const
	{
		float minDist = 0.0f;
		float dist0 = calcuPointDistance(t.p0);
		float dist1 = calcuPointDistance(t.p1);
		float dist2 = calcuPointDistance(t.p2);
		minDist = dist0 > dist1 ? dist1 : dist0;
		minDist = minDist > dist2 ? dist2 : minDist;
		return minDist;
	}

	/// calculate the distance from a point to the plane
	float calcuPointDistance(glm::vec3 p) const
	{
		return glm::abs(para.x*p.x + para.y*p.y + para.z*p.z + para.w) /
			glm::sqrt(para.x*para.x + para.y*para.y + para.z*para.z);
	}

	
	glm::vec3 normal;
	float distance;   // distance from origin in normal direction
	glm::vec4 para;   // AX+BY+CZ+D=0  (A,B,C,D)

private:
	/// calculate the plane parameter according to the normal and distance value
	void calcuPlanePara()
	{
		// caculate plane expression: AX+BY+CZ+D=0
		// A(X-X0) + B(Y-Y0) + C(Z-Z0)= 0
		// (A,B,C)=normal,  (X0,Y0,Z0)=normal*distantFromOrigin
		float D = -(normal.x*(normal*distance).x + normal.y * (normal*distance).y + normal.z * (normal*distance).z);
		para = glm::vec4(normal, D);
	}
};

#endif // !PLANE_H