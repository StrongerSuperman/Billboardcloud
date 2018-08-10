#ifndef DISCRETIZATION_H
#define DISCRETIZATION_H

#include <glm/glm.hpp>
#include "core/debug.h"
#include "math/linearalgebra.h"
#include "triangle.h"
#include "plane.h"
#include "bin.h"
#include <vector>
#include <time.h>
#include <iostream>
#include <windows.h>

#ifndef pi
#define pi 3.1415926f
#endif // !pi

class Discretization
{
public:
	/// bins
	std::vector<std::vector<std::vector<Bin>>> bins;
	/// fail-safe mode para
	bool failSafeModeTriggered;
	/// fitted plane in fail-safe mode 
	std::vector<Triangle> bestFittedPlaneValidTriangle;

	/// constructor
	Discretization(float _roMax, float _epsilon, int _discretize_theta_num, int _discretize_phi_num, int _discretize_ro_num)
		:failSafeModeTriggered(false),
		thetaMin(0),
		thetaMax(2 * pi),
		phiMin(-pi / 2),
		phiMax(pi / 2),
		roMin(0),
		roMax(_roMax),
		epsilon(_epsilon),
		weightPenalty(10),   // recommend "10" in paper
		discretize_theta_num(_discretize_theta_num),
		discretize_phi_num(_discretize_phi_num),
		discretize_ro_num(_discretize_ro_num)    
	{
		initBins();
	}

	/// compute current max density and the corresponding bin index
	std::pair<glm::vec3, float> computeMaxDensity()
	{
		// pick bin with max density
		float maxDensity = -FLT_MAX;
		int maxDensityBin_i = 0;
		int maxDensityBin_j = 0;
		int maxDensityBin_k = 0;
		for (int i = 0; i < discretize_ro_num; i++)
		{
			for (int j = 0; j < discretize_phi_num; j++)
			{
				for (int k = 0; k < discretize_theta_num; k++)
				{
					float tmp = bins[i][j][k].density;
					if (maxDensity < tmp)
					{
						maxDensity = tmp;
						maxDensityBin_i = i;
						maxDensityBin_j = j;
						maxDensityBin_k = k;
					}
				}
			}
		}
		return std::make_pair(glm::vec3(maxDensityBin_i, maxDensityBin_j, maxDensityBin_k), maxDensity);
	}

	/// update all bins density (mode type: "add(0)"¡¢"remove(1)")
	void updateDensity(const std::vector<Triangle>& triangles, int mode)
	{
		if (triangles.empty())
		{
			COUT << "ERROR: the size of the input triangles is 0!" << std::endl;
			COUT << "ERROR: that means in the last iter, there is no fitted plane found!" << std::endl;
			return;
		}

		float time = clock();
		for (auto& triangle : triangles)
		{
			for (int i = 0; i < discretize_phi_num; i++)        // phiCoord
			{
				for (int j = 0; j < discretize_theta_num; j++)  // thetaCoord
				{
					glm::vec2 roMaxMin = computeRoMinMax(triangle,
						thetaMin + j * thetaGap,
						thetaMin + (j + 1) * thetaGap,
						phiMin + i * phiGap,
						phiMin + (i + 1) * phiGap);
					if (roMaxMin.x == -1 && roMaxMin.y == -1)
						continue;

					float ro_min = roMaxMin.x;   // ro_max_p_min_n
					float ro_max = roMaxMin.y;   // ro_min_p_max_n

					// add coverage ,bins between (ro_min, ro_max)
					int roCoordMin = (ro_min - roMin) / roGap;
					int roCoordMax = (ro_max - roMin) / roGap;
					if (roCoordMin > discretize_ro_num - 1)
					{
						roCoordMin = discretize_ro_num - 1;
					}
					if (roCoordMax > discretize_ro_num - 1)
					{
						roCoordMax = discretize_ro_num - 1;
					}
					if (roCoordMax - roCoordMin > 2)
					{
						if (mode == 0)
						{
							// use the ceterPoint's normal of the bin to calculate the projected triangle area
							bins[roCoordMin][i][j].density += triangle.getArea()*
								glm::abs(glm::dot(triangle.normal, bins[roCoordMin][i][j].centerNormal))*
								(((roMin + (roCoordMin + 1)*roGap) - ro_min) / roGap);
						}
						else if (mode == 1)
						{
							bins[roCoordMin][i][j].density -= triangle.getArea()*
								glm::abs(glm::dot(triangle.normal, bins[roCoordMin][i][j].centerNormal))*
								(((roMin + (roCoordMin + 1)*roGap) - ro_min) / roGap);
						}
						for (int k = roCoordMin + 1; k < roCoordMax; k++)
						{
							if (mode == 0)
							{
								bins[k][i][j].density += triangle.getArea()*
									glm::abs(glm::dot(triangle.normal, bins[k][i][j].centerNormal));
							}
							else if (mode == 1)
							{
								bins[k][i][j].density -= triangle.getArea()*
									glm::abs(glm::dot(triangle.normal, bins[k][i][j].centerNormal));
							}
						}
						if (mode == 0)
						{
							bins[roCoordMax][i][j].density += triangle.getArea()*
								glm::abs(glm::dot(triangle.normal, bins[roCoordMax][i][j].centerNormal))*
								((ro_max - (roCoordMax * roGap + roMin)) / roGap);
						}
						else if (mode == 1)
						{
							bins[roCoordMax][i][j].density -= triangle.getArea()*
								glm::abs(glm::dot(triangle.normal, bins[roCoordMax][i][j].centerNormal))*
								((ro_max - (roCoordMax * roGap + roMin)) / roGap);
						}
					}
					else if (roCoordMax - roCoordMin == 1)
					{
						if (mode == 0)
						{
							bins[roCoordMin][i][j].density += triangle.getArea()*
								glm::abs(glm::dot(triangle.normal, bins[roCoordMin][i][j].centerNormal))*
								(((roMin + (roCoordMin + 1)*roGap) - ro_min) / roGap);
						}
						else if (mode == 1)
						{
							bins[roCoordMin][i][j].density -= triangle.getArea()*
								glm::abs(glm::dot(triangle.normal, bins[roCoordMin][i][j].centerNormal))*
								(((roMin + (roCoordMin + 1)*roGap) - ro_min) / roGap);
						}

						if (mode == 0)
						{
							bins[roCoordMax][i][j].density += triangle.getArea()*
								glm::abs(glm::dot(triangle.normal, bins[roCoordMax][i][j].centerNormal))*
								((ro_max - (roCoordMax * roGap + roMin)) / roGap);
						}
						else if (mode == 1)
						{
							bins[roCoordMax][i][j].density -= triangle.getArea()*
								glm::abs(glm::dot(triangle.normal, bins[roCoordMax][i][j].centerNormal))*
								((ro_max - (roCoordMax * roGap + roMin)) / roGap);
						}
					}
					else if (roCoordMax - roCoordMin == 0)
					{
						if (mode == 0)
						{
							bins[roCoordMin][i][j].density += triangle.getArea()*
								glm::abs(glm::dot(triangle.normal, bins[roCoordMin][i][j].centerNormal));
						}
						else if (mode == 1)
						{
							bins[roCoordMin][i][j].density -= triangle.getArea()*
								glm::abs(glm::dot(triangle.normal, bins[roCoordMin][i][j].centerNormal));
						}
					}

					// add penalty ,bins between (ro_min - epsilon, ro_min)
					if ((ro_min - epsilon - roMin) > 0)
					{
						int roMinMinusEpsilonCoord = (ro_min - epsilon - roMin) / roGap;
						for (int m = roMinMinusEpsilonCoord; m <= roCoordMin; m++)
						{
							if (mode == 0)
							{
								bins[m][i][j].density -= triangle.getArea()*
									glm::abs(glm::dot(triangle.normal, bins[m][i][j].centerNormal))*
									weightPenalty;
							}
							else if (mode == 1)
							{
								bins[m][i][j].density += triangle.getArea()*
									glm::abs(glm::dot(triangle.normal, bins[m][i][j].centerNormal))*
									weightPenalty;
							}
						}
					}
				}
			}
		}
		COUT << "updating_density...  time :" << (clock() - time) / 1000 << "s" << std::endl;
	}

	/// refine bin to plane
	Plane refineBin(const std::vector<Triangle>& validSet, const Bin& maxDensityBin)
	{
		COUT << std::endl;
		COUT << "refine bin ..." << std::endl;

		Plane centerPlane = Plane(maxDensityBin.centerNormal, maxDensityBin.roCenter);
		std::vector<int> centerPlaneValidSetIndex = computePlaneValidSetIndex(validSet, centerPlane);

		COUT << "current_set_num: " << validSet.size() << std::endl;
		COUT << "current_center_plane_valid_set_num: " << centerPlaneValidSetIndex.size() << std::endl;

		if (centerPlaneValidSetIndex.size() == validSet.size())
		{
			COUT << "refine complete!" << std::endl;
			COUT << std::endl;

			// fix bugs (why occur this kind of situation???)
			if (maxDensityBin.thetaCenter > pi)
			{
				centerPlane = Plane(-centerPlane.normal, centerPlane.distance);
			}

			float maxDis = 0.0f;
			for (auto& triangle : validSet)
			{
				float d0 = centerPlane.calcuPointDistance(triangle.p0);
				float d1 = centerPlane.calcuPointDistance(triangle.p1);
				float d2 = centerPlane.calcuPointDistance(triangle.p2);
				maxDis = d0 > d1 ? d0 : d1;
				maxDis = maxDis > d2 ? maxDis : d2;
			}
			COUT << "plane_validSets_max_distance: " << maxDis << std::endl;
			COUT << "plane_distance: " << centerPlane.distance << std::endl;
			COUT << "plane_normal: (" << centerPlane.normal.x << ", " << centerPlane.normal.y << ", " << centerPlane.normal.z << ")" << std::endl;
			COUT << "plane_sphere_coord: (" << maxDensityBin.thetaCenter << ", " << maxDensityBin.phiCenter << ", " << maxDensityBin.roCenter << ")" << std::endl << std::endl;
			return centerPlane;
		}

		Bin binMax;
		binMax.density = FLT_MIN;
		// pick the bin and its 26 neighbors (if have)
		std::vector<Bin> neighborBins = getBinNeighbors(maxDensityBin);
		for (auto& neighborBin : neighborBins)
		{
			// subdivide the bin into 8 bins
			std::vector<Bin> subdividBins = subdivideBin(neighborBin);
			for (auto& subdividBin : subdividBins)
			{
				// pick the subdivide bin with max density
				computeDensity(validSet, subdividBin);
				if (subdividBin.density > binMax.density)
				{
					binMax = subdividBin;
				}
			}
		}
		std::vector<Triangle> binMaxValidSet = computeBinValidSet(validSet, binMax);

		COUT << "max_density_subBin valid trianle num: " << binMaxValidSet.size() << std::endl;
		COUT << "max_density_subBin density: " << binMax.density << std::endl;

		if (binMaxValidSet.size() == 0)
		{
			COUT << "ERROR: subBinMax has no valid set, we will simply return the last densiest bin's center plane!" << std::endl;

			// if the centerPlane has no valid set in the current remain sets, the iteration will end up with infinite loop!!!
			if (centerPlaneValidSetIndex.size() != 0)
			{
				COUT << "INFO: but last densiest bin's center plane has valid set" << std::endl;
				COUT << "INFO: so we can simply return the last densiest bin's center plane!" << std::endl;

				return centerPlane;
			}
			else
			{
				COUT << "ERROR: the centerPlane has no valid set in the current remain sets too" << std::endl;
				COUT << "INFO: so we return the best fitted plane of the last densiest bin's valid set " << std::endl;

				failSafeModeTriggered = true;
				bestFittedPlaneValidTriangle = validSet;
				std::vector<glm::vec3> points;
				for (auto& triangle : validSet)
				{
					points.emplace_back(triangle.p0);
					points.emplace_back(triangle.p1);
					points.emplace_back(triangle.p2);
				}
				auto fitted = best_plane_from_points(points);
				auto centroid = fitted.first;
				auto normal = fitted.second;
				if (glm::dot(centroid, normal) < 0)
				{
					normal = -normal;
				}
				float distance = glm::abs(glm::dot(centroid, normal));

				return Plane(normal, distance);
			}
		}
		return refineBin(binMaxValidSet, binMax);
	}

	/// compute the valid set of a bin
	std::vector<Triangle> computeBinValidSet(const std::vector<Triangle>& triangles, const Bin& bin)
	{
		std::vector<Triangle> binValidSet;
		for (auto& triangle : triangles)
		{
			// we use the notion of "simple validity":
			// that is a bin is valid for a triangle as long as there exists a valid plane for the triangle in the bin 
			// if the ro min and ro max is in the range of bin's ro range, we think this triangle is valid for the bin
			glm::vec2 roMinMax = computeRoMinMax(triangle, bin.thetaMin, bin.thetaMax, bin.phiMin, bin.phiMax);
			if (roMinMax.x == -1 && roMinMax.y == -1)
				continue;

			if (!(roMinMax.y < bin.roMin) &&
				!(roMinMax.x > bin.roMax))
			{
				binValidSet.emplace_back(triangle);
			}
		}
		return binValidSet;
	}

	/// compute the valid set index of a plane
	std::vector<int> computePlaneValidSetIndex(const std::vector<Triangle>& triangles, const Plane& plane)
	{
		std::vector<int> planeValidSetIndex;
		for (int i = 0; i < triangles.size(); i++)
		{
			float ro_p0_n = glm::abs(glm::dot(triangles[i].p0, plane.normal));
			float ro_p1_n = glm::abs(glm::dot(triangles[i].p1, plane.normal));
			float ro_p2_n = glm::abs(glm::dot(triangles[i].p2, plane.normal));
			float tmp0[] = { ro_p0_n - epsilon ,ro_p1_n - epsilon ,ro_p2_n - epsilon };
			float tmp1[] = { ro_p0_n + epsilon ,ro_p1_n + epsilon ,ro_p2_n + epsilon };

			float roMinTmp = calcuMinMaxValue(tmp0, 3, 0);
			float roMaxTmp = calcuMinMaxValue(tmp1, 3, 1);

			if (plane.distance > roMinTmp&&
				plane.distance < roMaxTmp)
				planeValidSetIndex.emplace_back(i);

			//if (glm::dot(triangles[i].p0, plane.normal) < 0 &&
			//	glm::dot(triangles[i].p1, plane.normal) < 0 &&
			//	glm::dot(triangles[i].p2, plane.normal) < 0)
			//{
			//	if (glm::dot(triangles[i].normal, plane.normal) >= 0)
			//	{
			//		float roMinTmp = calcuMinMaxValue(tmp0, 3, 0);
			//		float roMaxTmp = calcuMinMaxValue(tmp1, 3, 1);
			//		if (plane.distance > roMinTmp&&
			//			plane.distance < roMaxTmp)
			//			planeValidSetIndex.emplace_back(i);
			//	}
			//	else
			//	{
			//		float tmp = glm::abs(calcuMinMaxValue(tmp0, 3, 0));
			//		if (plane.distance < tmp)
			//			planeValidSetIndex.emplace_back(i);
			//	}
			//}
			//else
			//{
			//	// we should use the following noted code according to the paper implementation
			//	// but it shows this restriction is too strong that little triangles fitted in the initial iteration
			//	// so we relax the restriction, employ the minimal value for min and maximal value for max
			//	//float roMinTmp = calcuMinMaxValue(tmp0, 3, 1);
			//	//float roMaxTmp = calcuMinMaxValue(tmp1, 3, 0);
			//	float roMinTmp = calcuMinMaxValue(tmp0, 3, 0);
			//	float roMaxTmp = calcuMinMaxValue(tmp1, 3, 1);
			//	if (plane.distance > roMinTmp&&
			//		plane.distance < roMaxTmp)
			//		planeValidSetIndex.emplace_back(i);
			//}
		}
		return planeValidSetIndex;
	}

private:
	/// para
	float epsilon;
	float weightPenalty;     
	/// discretization num
	int discretize_theta_num;
	int discretize_phi_num;
	int discretize_ro_num;
	/// range
	float thetaMax;
	float thetaMin;
	float phiMax;
	float phiMin;
	float roMax;
	float roMin;
	/// gap
	float thetaGap;
	float phiGap;
	float roGap;

	/// initially separate the 3d space of specified range into bins
	void initBins()
	{
		thetaGap = (thetaMax - thetaMin) / discretize_theta_num;
		phiGap = (phiMax - phiMin) / discretize_phi_num;
		roGap = (roMax - roMin) / discretize_ro_num;

		for (int i = 0; i < discretize_ro_num; i++)
		{
			std::vector<std::vector<Bin>> tmp1;
			for (int j = 0; j < discretize_phi_num; j++)
			{
				std::vector<Bin> tmp2;
				for (int k = 0; k < discretize_theta_num; k++)
				{
					float thetaMinTmp = thetaGap * k + thetaMin;
					float thetaMaxTmp = thetaMinTmp + thetaGap;
					float phiMinTmp = phiGap * j + phiMin;
					float phiMaxTmp = phiMinTmp + phiGap;
					float roMinTmp = roGap * i + roMin;
					float roMaxTmp = roMinTmp + roGap;
					Bin bin(thetaMinTmp, thetaMaxTmp, phiMinTmp, phiMaxTmp, roMinTmp, roMaxTmp);
					tmp2.emplace_back(bin);
				}
				tmp1.emplace_back(tmp2);
			}
			bins.emplace_back(tmp1);
		}
	}

	/// get the 26 neighbor bins which is the same size as itself (return value include itself)
	std::vector<Bin> getBinNeighbors(const Bin& bin)
	{
		float curBinThetaGap = bin.thetaMax - bin.thetaMin;
		float curBinPhiGap = bin.phiMax - bin.phiMin;
		float curBinRoGap = bin.roMax - bin.roMin;

		std::vector<Bin> binsTmp;
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				for (int k = -1; k <= 1; k++)
				{
					float neighborThetaMin = bin.thetaMin + curBinThetaGap * k;
					float neighborThetaMax = neighborThetaMin + curBinThetaGap;
					float neighborPhiMin = bin.phiMin + curBinPhiGap * j;
					float neighborPhiMax = neighborPhiMin + curBinPhiGap;
					float neighborRoMin = bin.roMin + curBinRoGap * i;
					float neighborRoMax = neighborRoMin + curBinRoGap;

					if (neighborPhiMin < phiMin)
					{
						neighborPhiMin = -(neighborPhiMin - phiMin);
						neighborThetaMin = neighborThetaMin + pi;
					}
					if (neighborPhiMax > phiMax)
					{
						neighborPhiMax = pi / 2 - (neighborPhiMax - phiMax);
						neighborThetaMin = neighborThetaMin + pi;
					}

					if (neighborThetaMin > 2 * pi)
					{
						neighborThetaMin = neighborThetaMin - 2 * pi;
					}
					if (neighborThetaMax > 2 * pi)
					{
						neighborThetaMax = neighborThetaMax - 2 * pi;
					}

					// if the bin's ro range is outside of the specific range, discard it!
					if (neighborRoMin < roMin || neighborRoMax > roMax)
						continue;

					Bin binTmp(neighborThetaMin, neighborThetaMax, neighborPhiMin, neighborPhiMax, neighborRoMin, neighborRoMax);
					binsTmp.emplace_back(binTmp);
				}
			}
		}
		return binsTmp;
	}

	/// subdivide a bin into 8 bins
	std::vector<Bin> subdivideBin(const Bin& bin)
	{
		std::vector<Bin> binsTmp;
		for (int i = 0; i <= 1; i++)
		{
			for (int j = 0; j <= 1; j++)
			{
				for (int k = 0; k <= 1; k++)
				{
					float curThetaMin = bin.thetaMin + (bin.thetaMax - bin.thetaMin) / 2 * k;
					float curThetaMax = curThetaMin + (bin.thetaMax - bin.thetaMin) / 2;
					float curPhiMin = bin.phiMin + (bin.phiMax - bin.phiMin) / 2 * j;
					float curPhiMax = curPhiMin + (bin.phiMax - bin.phiMin) / 2;
					float curRoMin = bin.roMin + (bin.roMax - bin.roMin) / 2 * i;
					float curRoMax = curRoMin + (bin.roMax - bin.roMin) / 2;

					Bin binTmp(curThetaMin, curThetaMax, curPhiMin, curPhiMax, curRoMin, curRoMax);
					binsTmp.emplace_back(binTmp);
				}
			}
		}
		return binsTmp;
	}

	/// compute single bin density for specific triangles (for sub bin density calculation use)
	void computeDensity(const std::vector<Triangle>& triangles, Bin& bin)
	{
		for (auto& triangle : triangles)
		{
			glm::vec2 roMinMax = computeRoMinMax(triangle, bin.thetaMin, bin.thetaMax, bin.phiMin, bin.phiMax);
			if (roMinMax.x == -1 && roMinMax.y == -1)
				continue;

			// add coverage
			float curRoMin = roMinMax.x;
			float curRoMax = roMinMax.y;
			float curRoGap = bin.roMax - bin.roMin;
			if (curRoMin < bin.roMin&&
				curRoMax > bin.roMin&&
				curRoMax < bin.roMax)
			{
				bin.density += triangle.getArea()*
					glm::abs(glm::dot(triangle.normal, bin.centerNormal))*
					(curRoMax - bin.roMin) / curRoGap;
			}
			else if (curRoMin > bin.roMin&&
				curRoMin < bin.roMax&&
				curRoMax > bin.roMax)
			{
				bin.density += triangle.getArea()*
					glm::abs(glm::dot(triangle.normal, bin.centerNormal))*
					(bin.roMax - curRoMin) / curRoGap;
			}
			else if (curRoMin >= bin.roMin&&
				curRoMax <= bin.roMax)
			{
				bin.density += triangle.getArea()*
					glm::abs(glm::dot(triangle.normal, bin.centerNormal));
			}

			//// add penalty
			//float curRoMinMinusEpsilon = curRoMin - epsilon;
			//if (curRoMinMinusEpsilon < roMin)
			//	curRoMinMinusEpsilon = roMin;
			//if (!(curRoMinMinusEpsilon > bin.roMax) &&
			//	!(curRoMin < bin.phiMin))
			//{
			//	bin.density -= triangle.getArea()*
			//		glm::abs(glm::dot(triangle.normal, sphereCoordToNormal((bin.thetaMin + bin.thetaMin) / 2, (bin.phiMin + bin.phiMin) / 2)))*
			//		weightPenalty;
			//}
		}
	}

	/// compute the min and max value of ro in the case of triangle is valid for the specific theta and phi range
	glm::vec2 computeRoMinMax(const Triangle& triangle, float curThetaMin, float curThetaMax, float curPhiMin, float curPhiMax)
	{
		glm::vec3 normal_1 = sphericalCoordToNormal(curThetaMin, curPhiMin);
		glm::vec3 normal_2 = sphericalCoordToNormal(curThetaMin, curPhiMax);
		glm::vec3 normal_3 = sphericalCoordToNormal(curThetaMax, curPhiMin);
		glm::vec3 normal_4 = sphericalCoordToNormal(curThetaMax, curPhiMax);

		float ro_p0_n1 = glm::dot(triangle.p0, normal_1);
		float ro_p0_n2 = glm::dot(triangle.p0, normal_2);
		float ro_p0_n3 = glm::dot(triangle.p0, normal_3);
		float ro_p0_n4 = glm::dot(triangle.p0, normal_4);
		float ro_p1_n1 = glm::dot(triangle.p1, normal_1);
		float ro_p1_n2 = glm::dot(triangle.p1, normal_2);
		float ro_p1_n3 = glm::dot(triangle.p1, normal_3);
		float ro_p1_n4 = glm::dot(triangle.p1, normal_4);
		float ro_p2_n1 = glm::dot(triangle.p2, normal_1);
		float ro_p2_n2 = glm::dot(triangle.p2, normal_2);
		float ro_p2_n3 = glm::dot(triangle.p2, normal_3);
		float ro_p2_n4 = glm::dot(triangle.p2, normal_4);

		float tmp0[] = { ro_p0_n1 - epsilon ,ro_p0_n2 - epsilon ,ro_p0_n3 - epsilon ,ro_p0_n4 - epsilon };
		float tmp1[] = { ro_p1_n1 - epsilon ,ro_p1_n2 - epsilon ,ro_p1_n3 - epsilon ,ro_p1_n4 - epsilon };
		float tmp2[] = { ro_p2_n1 - epsilon ,ro_p2_n2 - epsilon ,ro_p2_n3 - epsilon ,ro_p2_n4 - epsilon };
		float ro_p0_min_n = calcuMinMaxValue(tmp0, 4, 0);
		float ro_p1_min_n = calcuMinMaxValue(tmp1, 4, 0);
		float ro_p2_min_n = calcuMinMaxValue(tmp2, 4, 0);
		float tmp3[] = { ro_p0_min_n ,ro_p1_min_n ,ro_p2_min_n };
		//ro_min
		float ro_max_p_min_n = calcuMinMaxValue(tmp3, 3, 1);
		if (ro_max_p_min_n < roMin)
			ro_max_p_min_n = roMin;
		else if (ro_max_p_min_n > roMax)
			ro_max_p_min_n = roMax;

		float tmp4[] = { ro_p0_n1 + epsilon ,ro_p0_n2 + epsilon ,ro_p0_n3 + epsilon ,ro_p0_n4 + epsilon };
		float tmp5[] = { ro_p1_n1 + epsilon ,ro_p1_n2 + epsilon ,ro_p1_n3 + epsilon ,ro_p1_n4 + epsilon };
		float tmp6[] = { ro_p2_n1 + epsilon ,ro_p2_n2 + epsilon ,ro_p2_n3 + epsilon ,ro_p2_n4 + epsilon };
		float ro_p0_max_n = calcuMinMaxValue(tmp4, 4, 1);
		float ro_p1_max_n = calcuMinMaxValue(tmp5, 4, 1);
		float ro_p2_max_n = calcuMinMaxValue(tmp6, 4, 1);
		float tmp7[] = { ro_p0_max_n ,ro_p1_max_n ,ro_p2_max_n };
		//ro_max
		float ro_min_p_max_n = calcuMinMaxValue(tmp7, 3, 0);

		if (ro_min_p_max_n < roMin)    // it means the triangle is not valid for the current bin
			return glm::vec2(-1, -1);

		if (ro_min_p_max_n < roMin)
			ro_min_p_max_n = roMin;
		else if (ro_min_p_max_n > roMax)
			ro_min_p_max_n = roMax;

		return glm::vec2(ro_max_p_min_n, ro_min_p_max_n);
	}

	/// trans the spherical coordinate of a plane into the normal vector
	glm::vec3 sphericalCoordToNormal(float theta, float phi)
	{
		float z = glm::sin(phi);
		float xy = glm::cos(phi);
		float x = xy * glm::cos(theta);
		float y = xy * glm::sin(theta);

		return glm::vec3(x, y, z);
	}

	/// calculate the min value or max value of the array
	float calcuMinMaxValue(float* data, int num, int mode)
	{
		float tmp = data[0];
		if (mode == 0)        // min
		{
			for (int i = 0; i < num; i++)
			{
				if (data[i] < tmp)
				{
					tmp = data[i];
				}
			}
		}
		else if (mode == 1)   // max
		{
			for (int i = 0; i < num; i++)
			{
				if (data[i] > tmp)
				{
					tmp = data[i];
				}
			}
		}
		return tmp;
	}
};

#endif // !DISCRETIZATION_H
