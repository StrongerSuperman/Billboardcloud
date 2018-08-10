#ifndef CLUSTER_H
#define CLUSTER_H

#include <glm/glm.hpp>
#include "math/linearalgebra.h"
#include "triangle.h"
#include "plane.h"
#include <vector>
#include <float.h>
#include <limits.h>

// note: this class is used for the k-means algorithm of the bbc generation
class Cluster
{
public:
	std::vector<Triangle> triangles;
	glm::vec3 centriod;
	Plane plane;

	Cluster()
	{
	}

	Cluster(const std::vector<Triangle>& _triangles)
		:triangles(_triangles)
	{
		update();
	}

	Cluster(const std::vector<Triangle>& _triangles, const Plane& _plane)
		:triangles(_triangles), plane(_plane)
	{
	}

	Cluster(const Cluster& cluster)
	{
		triangles = cluster.triangles;
		plane = cluster.plane;
	}

	Cluster& operator=(const Cluster& cluster)
	{
		triangles = cluster.triangles;
		plane = cluster.plane;
		return *this;
	}

	glm::vec3 getCentriod() const
	{
		return centriod;
	}

	Plane getBestFittedPlane() const
	{
		return plane;
	}

	void update()
	{
		// note: can not change the following method's executing order
		updateBestFittedPlane();
		updateCentriod();
	}

private:
	/// using SVD and specific distance metric for triangles
	/// The distance is computed as the sum of the Euclidean distances of the triangle vertices to the plane
	void updateBestFittedPlane()
	{
		std::vector<glm::vec3> points;
		if (triangles.size() != 0)
		{
			for (int i = 0; i < triangles.size(); i++)
			{
				points.emplace_back(triangles[i].p0);
				points.emplace_back(triangles[i].p1);
				points.emplace_back(triangles[i].p2);
			}
			// svd
			auto fitted = best_plane_from_points(points);

			auto centroid = fitted.first;
			auto normal = fitted.second;
			if (glm::dot(centroid, normal) < 0)
			{
				normal = -normal;
			}
			float distance = glm::abs(glm::dot(centroid, normal));

			Plane bestFitted(normal, distance);
			// update current best fitted plane
			plane = bestFitted;
		}
	}

	/// the centroid of a cluster is computed by projecting all triangles onto its best fit plane 
	/// and by determining the centroid of the triangle closest to the centroid of all projected triangle vertices
	void updateCentriod()
	{
		std::vector<glm::vec3> projPoints;
		if (triangles.size() != 0)
		{
			// first collect points which is projected from triangle points onto the best fitted plane
			for (auto& triangle : triangles)
			{
				float t1 = (plane.para.x * triangle.p0.x +
					plane.para.y * triangle.p0.y +
					plane.para.z * triangle.p0.z + plane.para.w) /
					(plane.para.x * plane.para.x +
						plane.para.y * plane.para.y +
						plane.para.z * plane.para.z);
				projPoints.emplace_back(glm::vec3(triangle.p0.x - plane.para.x * t1,
					triangle.p0.y - plane.para.y * t1,
					triangle.p0.z - plane.para.z * t1));

				float t2 = (plane.para.x * triangle.p1.x +
					plane.para.y * triangle.p1.y +
					plane.para.z * triangle.p1.z + plane.para.w) /
					(plane.para.x * plane.para.x +
						plane.para.y * plane.para.y +
						plane.para.z * plane.para.z);
				projPoints.emplace_back(glm::vec3(triangle.p1.x - plane.para.x * t2,
					triangle.p1.y - plane.para.y * t2,
					triangle.p1.z - plane.para.z * t2));

				float t3 = (plane.para.x * triangle.p2.x +
					plane.para.y * triangle.p2.y +
					plane.para.z * triangle.p2.z + plane.para.w) /
					(plane.para.x * plane.para.x +
						plane.para.y * plane.para.y +
						plane.para.z * plane.para.z);
				projPoints.emplace_back(glm::vec3(triangle.p2.x - plane.para.x * t3,
					triangle.p2.y - plane.para.y * t3,
					triangle.p2.z - plane.para.z * t3));
			}
			// then get the centriod of all projected points (we use the average position currently)
			float x_coord = 0.0f;
			float y_coord = 0.0f;
			float z_coord = 0.0f;
			for (auto point : projPoints)
			{
				x_coord += point.x;
				y_coord += point.y;
				z_coord += point.z;
			}
			glm::vec3 centriodTmp = glm::vec3(x_coord / projPoints.size(), y_coord / projPoints.size(), z_coord / projPoints.size());

			// find min dist triangle index
			float minDistance = FLT_MAX;
			int minDistanceIndex = 0;
			for (int i = 0; i < triangles.size(); i++)
			{
				float distanceTmp = glm::length(centriodTmp - triangles[i].getCentriod());
				if (minDistance > distanceTmp)
				{
					minDistance = distanceTmp;
					minDistanceIndex = i;
				}
			}

			centriod = triangles[minDistanceIndex].getCentriod();
		}
	}
};

#endif // !CLUSTER_H
