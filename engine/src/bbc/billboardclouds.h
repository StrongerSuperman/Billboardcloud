#ifndef BILLBOARDCLOUDS_H
#define BILLBOARDCLOUDS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "core/mesh.h"
#include "core/glfw.h"
#include "core/shader.h"
#include "core/debug.h"
#include "core/config.h"
#include "core/texture.h"
#include "math/rotatingcalipers.h"
#include "math/randseed.h"
#include "billboard.h"
#include "discretization.h"
#include "boundingSphere.h"
#include "rectangle.h"
#include "triangle.h"
#include "plane.h"
#include "bin.h"
#include "cluster.h"
#include <time.h>
#include <float.h>
#include <limits.h>
#include <thread>
#include <iostream>
#include <vector>
#include <windows.h>
#include <exception>

class BillboardCloud
{
public:
	glm::vec3 position;
	glm::vec4 rotate;
	glm::vec3 scale;
	float genTime;
	int bbcNum;
	int skipFaceNum;
	std::string algorithmType;
	std::string meshName;
	bool genComplete;
	bool saveComplete;

	/// constructor
	BillboardCloud(Mesh* _mesh, Shader& _textureGenShader, Glfw& _glfw, std::string _meshName)
		:mesh(_mesh),
		textureGenShader(_textureGenShader),
		glfw(_glfw),
		meshName(_meshName),
		position(glm::vec3(0.0f, 0.0f, 0.0f)),
		rotate(glm::vec4(0.0f, 1.0f, 0.0f, glm::radians(0.0f))),
		scale(glm::vec3(1.0f, 1.0f, 1.0f)),
		bbcNum(0),
		genTime(0.0f),
		skipFaceNum(0),
		planeSearchComplete(false),
		genComplete(false),
		saveComplete(false),
		switchRenderIndex(0)
	{
		init();
	}

	~BillboardCloud()
	{
		for (auto& d : textureAtlas)
		{
			if (d != nullptr)
			{
				free(d);
			}
		}
	}

	/// muti-thread processing
	void generateAsync(int algorithm_type, int num, float epsilon_percentage = 0.01f, int max_iter = 50)
	{
		std::thread t(&BillboardCloud::generate, this, algorithm_type, num, epsilon_percentage, max_iter);
		t.detach();
	}

	/// generate the bbc according to specific algorithm
	void generate(int algorithm_type, int num, float epsilon_percentage = 0.01f, int max_iter = 50)
	{
		updateCache();
		genComplete = false;

		float begin = clock();
		if (algorithm_type == 0)
		{
			algorithmType = "original";
			COUT << "start generating billboard clouds with original algorithm..." << std::endl;
			originalPlaneSearch(epsilon_percentage, num, num);
			// add crack reduction here will cause bad effect
			copyMeshIndicesIndex();
			projTrianglesOntoPlane();
			genBoundingRectangle();
		}
		else if (algorithm_type == 1)
		{
			algorithmType = "stochastic";
			COUT << "start generating billboard clouds with stochastic algorithm..." << std::endl;
			stochasticPlaneSearch(epsilon_percentage, num);
			//crackReduction();
			copyMeshIndicesIndex();
			projTrianglesOntoPlane();
			genBoundingRectangle();
		}
		else if (algorithm_type == 2)
		{
			algorithmType = "kmeans";
			COUT << "start generating billboard clouds with kmeans algorithm..." << std::endl;
			kMeansPlaneSearch(num, max_iter);
			//crackReduction();
			copyMeshIndicesIndex();
			projTrianglesOntoPlane();
			genBoundingRectangle();
		}
		genTime = clock() - begin;

		planeSearchComplete = true;
		printResult();
	}

	/// compute the bbc texture
	/// note: this method must be called in the main thread !!!
	/// since all openGL relevant functions are only valid in the main thread for Glfw
	void computeTexture()
	{
		if (planeSearchComplete)
		{
			// render to texture
			renderToTexture();

			bbcNum = bbs.size();
			planeSearchComplete = false;
			genComplete = true;
			saveComplete = false;
			destroyTmp();

			// generate the texture image by reading pixel from framebuffer
			readPixelFromFramebuffer();
		}
	}

	/// save billboard clouds
	void exportData()
	{
		if (!saveComplete)
		{
			if (genComplete)
			{
				//writeToImgAsync();
				writeToTextureAtlasAsync();
				saveComplete = true;
			}
		}
	}

	/// print the result
	void printResult()
	{
		COUT << std::endl << "------------------------------result-------------------------------" << std::endl;
		COUT << "algorithm: " << algorithmType << std::endl;
		COUT << "time: " << genTime / 1000 << "s" << std::endl;
		COUT << "bbc num: " << bbcNum << std::endl;
		COUT << "skipped face num: " << skipFaceNum << std::endl;
	}

	void render(Shader& renderShader, Shader& modelShader)
	{
		if (genComplete)
		{
			if (glfw.testMode)
			{
				// press key to display next bbc
				if (glfw.switchDisplay)
				{
					glfw.curFrameSign = true;
					if (!glfw.lastFrameSign)
					{
						switchRenderIndex++;
						if (switchRenderIndex > bbs.size() - 1)
						{
							switchRenderIndex = 0;
						}
					}
				}

				// three display mode(testMode)
				if (glfw.bbcFitMode1)
				{
					for (int i = 0; i < bbMeshes.size(); i++)
					{
						glm::mat4 _model;
						_model = glm::translate(_model, position);
						_model = glm::rotate(_model, rotate.w, glm::vec3(rotate.x, rotate.y, rotate.z));
						_model = glm::scale(_model, scale);
						modelShader.use();
						modelShader.setMat4("projection", glfw.mainCamProjection);
						modelShader.setMat4("view", glfw.mainCamView);
						modelShader.setMat4("model", _model);
						modelShader.setVec3("lightPos", glfw.lightPos);
						modelShader.setVec3("viewPos", glfw.mainCamPos);
						modelShader.setBool("blinn", glfw.blinn);
						bbMeshes[i].render(modelShader);
					}

					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					for (int i = 0; i < bbs.size(); i++)
					{
						bbs[i].position = position;
						bbs[i].rotate = rotate;
						bbs[i].scale = scale;
						bbs[i].isTest = true;
						bbs[i].render(renderShader, "bbc");
					}
					glDisable(GL_BLEND);
				}
				else if (glfw.bbcFitMode2)
				{
					glm::mat4 _model;
					_model = glm::translate(_model, position);
					_model = glm::rotate(_model, rotate.w, glm::vec3(rotate.x, rotate.y, rotate.z));
					_model = glm::scale(_model, scale);
					modelShader.use();
					modelShader.setMat4("projection", glfw.mainCamProjection);
					modelShader.setMat4("view", glfw.mainCamView);
					modelShader.setMat4("model", _model);
					modelShader.setVec3("lightPos", glfw.lightPos);
					modelShader.setVec3("viewPos", glfw.mainCamPos);
					modelShader.setBool("blinn", glfw.blinn);
					bbMeshes[switchRenderIndex].render(modelShader);

					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					bbs[switchRenderIndex].position = position;
					bbs[switchRenderIndex].rotate = rotate;
					bbs[switchRenderIndex].scale = scale;
					bbs[switchRenderIndex].isTest = true;
					bbs[switchRenderIndex].render(renderShader, "bbc");
					glDisable(GL_BLEND);
				}
				else if (glfw.bbcFitMode3)
				{
					bbs[switchRenderIndex].position = position;
					bbs[switchRenderIndex].rotate = rotate;
					bbs[switchRenderIndex].scale = scale;
					bbs[switchRenderIndex].isTest = false;
					bbs[switchRenderIndex].render(renderShader, "bbc");

					glEnable(GL_BLEND);
					glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
					bbs[switchRenderIndex].position = position;
					bbs[switchRenderIndex].rotate = rotate;
					bbs[switchRenderIndex].scale = scale;
					bbs[switchRenderIndex].isTest = true;
					bbs[switchRenderIndex].render(renderShader, "bbc");
					glDisable(GL_BLEND);
				}
			}
			else
			{
				for (int i = 0; i < bbs.size(); i++)
				{
					bbs[i].position = position;
					bbs[i].rotate = rotate;
					bbs[i].scale = scale;
					bbs[i].isTest = false;
					bbs[i].render(renderShader, "bbc");
				}
			}
		}
	}

private:
	/// tmp variable for switch display
	int switchRenderIndex;
	bool planeSearchComplete;
	/// input
	const Mesh* mesh;
	Shader& textureGenShader;
	Glfw& glfw;
	/// output
	std::vector<Billboard> bbs;
	std::vector<Plane> bbc;
	std::vector<Rect> bbcRectangle;
	std::vector<Mesh> bbMeshes;
	std::vector<unsigned int> bbsWidthResolution;
	std::vector<unsigned int> bbsHeightResolution;
	std::vector<BYTE*> textureAtlas;
	BoundingSphere boundingSphere;
	/// tmp 
	std::vector<Triangle> trianglesOrg;
	std::vector<std::vector<Triangle>> trianglesBeforeProj;
	std::vector<std::vector<Triangle>> trianglesAfterProj;
	std::vector<std::vector<unsigned int>> bbcMeshIndicesIndex;  // indicesIndex in the mesh indices

	/// trans mesh to triangles
	void init()
	{
		for (int i = 1; i <= mesh->indices.size(); i++)
		{
			if (i % 3 == 0)
			{
				glm::vec3 p0 = mesh->vertices[mesh->indices[i - 3]].Position;
				glm::vec3 p1 = mesh->vertices[mesh->indices[i - 2]].Position;
				glm::vec3 p2 = mesh->vertices[mesh->indices[i - 1]].Position;
				glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
				if (glm::dot(p0, normal) < 0)
				{
					normal = -normal;
				}
				float distance = glm::abs(glm::dot(p0, normal));
				glm::vec3 indices = glm::vec3(i - 3, i - 2, i - 1);
				Triangle triangle(p0, p1, p2, distance, normal, indices);
				trianglesOrg.emplace_back(triangle);
			}
		}
		boundingSphere.init(trianglesOrg);
	}

	/// original bbc algorithm
	void originalPlaneSearch(float epsilon_percentage, int theta_num, int phi_num)
	{
		int ro_num = 1.5 / epsilon_percentage;   // suggest value in later paper realize
		float epsilon = 2 * boundingSphere.radius*epsilon_percentage;

		int epoch = 0;
		std::vector<Triangle> trianglesTmp = trianglesOrg;

		float maxDistance = 0.0f;
		for (auto& triangle : trianglesTmp)
		{
			if (maxDistance < triangle.distance)
				maxDistance = triangle.distance;
		}

		COUT << "---------------------------------------------------------------------" << std::endl;
		COUT << "initially configure bins ..." << std::endl;
		Discretization discretization(maxDistance, epsilon, theta_num, phi_num, ro_num);
		discretization.updateDensity(trianglesTmp, 0);

		while (!trianglesTmp.empty())
		{
			COUT << "---------------------------------------------------------------------" << std::endl;
			COUT << "current_iteration :" << ++epoch << std::endl;
			COUT << "current_remain_total_triangle_num: " << trianglesTmp.size() << std::endl;

			// update the trianle index in current triangles
			for (int i = 0; i < trianglesTmp.size(); i++)
			{
				trianglesTmp[i].index = i;
			}

			// pick bin with max density
			float maxDensity = discretization.computeMaxDensity().second;
			glm::vec3 maxDensityBinIndex = discretization.computeMaxDensity().first;

			COUT << "current_iter_max_density :" << maxDensity << std::endl;

			// prepare the refine parameter
			Bin maxDensityBin(discretization.bins[maxDensityBinIndex.x][maxDensityBinIndex.y][maxDensityBinIndex.z]);
			std::vector<Triangle> binValidSet = discretization.computeBinValidSet(trianglesTmp, maxDensityBin);

			Plane refinedPlane;
			std::vector<Triangle> planeValidSet;
			if (!binValidSet.size() == 0)
			{
				// refine bin to plane
				refinedPlane = discretization.refineBin(binValidSet, maxDensityBin);

				// fail-safe mode
				if (discretization.failSafeModeTriggered)
				{
					planeValidSet = discretization.bestFittedPlaneValidTriangle;
					discretization.bestFittedPlaneValidTriangle.clear();

					COUT << "fitted_triangle_num: " << planeValidSet.size() << std::endl;

					// update density by removing the fitted triangle 
					discretization.updateDensity(planeValidSet, 1);

					// store bbc and corresponding fitted triangles
					trianglesBeforeProj.emplace_back(planeValidSet);
					//bbc.emplace_back(refinedPlane);

					int k = 0;
					for (auto& triangle : planeValidSet)
					{
						trianglesTmp.erase(trianglesTmp.begin() + (triangle.index - k++));  // note the offset("triangle.index - k++") of the iterator must be calculate beforehand
					}
					discretization.failSafeModeTriggered = false;
				}
				else
				{
					// get the fitted triangles index in the whole triangles
					std::vector<int> planeValidSetIndex = discretization.computePlaneValidSetIndex(trianglesTmp, refinedPlane);

					COUT << "fitted_triangle_num: " << planeValidSetIndex.size() << std::endl;

					for (int index : planeValidSetIndex)
					{
						planeValidSet.emplace_back(trianglesTmp[index]);
					}

					// update density by removing the fitted triangles
					discretization.updateDensity(planeValidSet, 1);

					// store bbc and corresponding fitted triangles
					trianglesBeforeProj.emplace_back(planeValidSet);
					//bbc.emplace_back(refinedPlane);

					// remove the fitted triangles (remove in reverse order))
					std::vector<int>::reverse_iterator riter;
					if (!planeValidSetIndex.empty())
					{
						for (riter = planeValidSetIndex.rbegin(); riter != planeValidSetIndex.rend(); riter++)
						{
							std::vector<Triangle>::const_iterator iter = trianglesTmp.begin() + *riter;
							trianglesTmp.erase(iter);
						}
					}
				}
			}
			else
			{
				// for the last little remain triangles, there's two way to cope with them:
				// 1. return the best fitted plane for the current remain faces
				// 2. simply employ "face skip"
				skipFaceNum = trianglesTmp.size();
				return;
			}
		}
		// find the best fitted plane of these triangles
		for(auto& triangles : trianglesBeforeProj)
		{
			std::vector<glm::vec3> points;
			for (auto& triangle : triangles)
			{
				points.emplace_back(triangle.p0);
				points.emplace_back(triangle.p1);
				points.emplace_back(triangle.p2);
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
			bbc.emplace_back(bestFitted);
		}
	}

	/// stochastic bbc algorithm
	void stochasticPlaneSearch(float epsilon_percentage, int iter)
	{
		float epsilon = 2 * boundingSphere.radius * epsilon_percentage;
		int epoch = 0;
		std::vector<Triangle> trianglesTmp = trianglesOrg;
		while (!trianglesTmp.empty())
		{
			COUT << "epoch: " << ++epoch << std::endl;
			COUT << "current_remain_triangles_num: " << trianglesTmp.size() << std::endl;

			Plane bbMax;
			std::vector<int> bbMaxTriangleIndexes;
			std::vector<Triangle> trianglesMaxBeforeProjTmp;
			for (int i = 0; i < iter; i++)
			{
				float maxArea = 0.0f;

				int seed = gen_rand_int(0, trianglesTmp.size() - 1);
				Triangle seedTriangle(trianglesTmp[seed]);

				// make billboard plane
				float perturb0 = gen_rand_real(-epsilon, epsilon);
				float perturb1 = gen_rand_real(-epsilon, epsilon);
				float perturb2 = gen_rand_real(-epsilon, epsilon);
				glm::vec3 p0 = seedTriangle.p0 + perturb0 * seedTriangle.normal;
				glm::vec3 p1 = seedTriangle.p1 + perturb1 * seedTriangle.normal;
				glm::vec3 p2 = seedTriangle.p2 + perturb2 * seedTriangle.normal;
				glm::vec3 normal = glm::normalize(glm::cross(p1 - p0, p2 - p0));
				if (glm::dot(seedTriangle.normal, normal) < 0)
				{
					normal = -normal;
				}
				float distance = glm::abs(glm::dot(p0, normal));
				Plane bb(normal, distance);
				std::vector<int> bbTriangleIndexes;
				// project triangles onto billboard plane
				float area = 0;
				for (int j = 0; j < trianglesTmp.size(); j++)
				{
					if (glm::abs(bb.distance - glm::dot(trianglesTmp[j].p0, bb.normal)) < epsilon
						&&glm::abs(bb.distance - glm::dot(trianglesTmp[j].p1, bb.normal)) < epsilon
						&&glm::abs(bb.distance - glm::dot(trianglesTmp[j].p2, bb.normal)) < epsilon)
					{
						// increment projected area (Angular area Contribution)
						// use projected area Contribution
						//area += trianglesTmp[j].getArea()*glm::abs(glm::dot(bb.normal, trianglesTmp[j].normal));
						float angle = glm::acos(glm::abs(glm::dot(bb.normal, trianglesTmp[j].normal)));
						float angular = (pi / 2 - angle) / (pi / 2);
						area += trianglesTmp[j].getArea()*angular;

						// save reference to T with billboard plane
						bbTriangleIndexes.emplace_back(j);
					}
				}
				if (area > maxArea)
				{
					maxArea = area;
					bbMax = bb;

					trianglesMaxBeforeProjTmp.clear();
					for (auto index : bbTriangleIndexes)
					{
						trianglesMaxBeforeProjTmp.emplace_back(trianglesTmp[index]);
					}
					bbMaxTriangleIndexes = bbTriangleIndexes;
				}
			}

			if (trianglesMaxBeforeProjTmp.size() == 0)
			{
				skipFaceNum = trianglesTmp.size();
				return;
			}

			// bbc and corresponding fitted triangles
			trianglesBeforeProj.emplace_back(trianglesMaxBeforeProjTmp);
			bbc.emplace_back(bbMax);

			// remove the triangles which fit a billboard plane (remove in reverse order)
			std::vector<int>::reverse_iterator riter;
			if (!bbMaxTriangleIndexes.empty())
			{
				for (riter = bbMaxTriangleIndexes.rbegin(); riter != bbMaxTriangleIndexes.rend(); riter++)
				{
					std::vector<Triangle>::const_iterator iter = trianglesTmp.begin() + *riter;
					trianglesTmp.erase(iter);
				}
			}
		}
	}

	/// k-means bbc algorithm
	void kMeansPlaneSearch(int k, int maxIter)
	{
		std::vector<Triangle> trianglesTmp = trianglesOrg;
		// k clusters
		std::vector<Cluster> clusters(k);

		//************************************
		// initialisation:[step1-step2-step3]
		//************************************
		COUT << "start initialisation" << std::endl;

		// -------------------------------------------------------------------------------------------
		// [step 1] -- Minimal Discrete Energy Distribution of Tangent Planes on a Bounding Sphere
		// -------------------------------------------------------------------------------------------
		COUT << "step 1" << std::endl;

		BoundingSphere bs(trianglesTmp);
		std::vector<Plane> planes = bs.calcuKTangenPlanes(k);
		// initial clusters
		for (int i = 0; i < planes.size(); i++)
		{
			Cluster clusterTmp;
			clusterTmp.plane = planes[i];
			clusters[i] = clusterTmp;
		}
		// assign triangles to these planes according to the specific minimal distance metric
		for (auto& triangle : trianglesTmp)
		{
			float minDistance = FLT_MAX;
			int minDistanceIndex = 0;
			for (int j = 0; j < clusters.size(); j++)
			{
				float distanceTmp = clusters[j].plane.calcuTotalDistance(triangle);
				if (minDistance > distanceTmp)
				{
					minDistance = distanceTmp;
					minDistanceIndex = j;
				}
			}
			clusters[minDistanceIndex].triangles.emplace_back(triangle);
		}
		// update clusters
		for (auto& cluster : clusters)
		{
			cluster.update();
		}

		// -------------------------------------------------------------------------------------------
		//  [step 2] -- Cluster Coverage Variance Reduction
		// -------------------------------------------------------------------------------------------
		COUT << "step 2" << std::endl;

		// used for storing the tmp triangles assignment
		std::vector<std::vector<Triangle>> trianglesContainerTmp(clusters.size());

		// reassign the triangle according to a different distance criterion
		for (int i = 0; i < trianglesTmp.size(); i++)
		{
			float minDistance = FLT_MAX;
			int minDistanceIndex = 0;
			for (int j = 0; j < clusters.size(); j++)
			{
				// The distance metric is given by the average distance of the vertices of a
				// triangle to the centroids of the cluster
				float distanceTmp = glm::length(trianglesTmp[i].p0 - clusters[j].getCentriod()) +
					glm::length(trianglesTmp[i].p1 - clusters[j].getCentriod()) +
					glm::length(trianglesTmp[i].p2 - clusters[j].getCentriod());
				if (minDistance > distanceTmp)
				{
					minDistance = distanceTmp;
					minDistanceIndex = j;
				}
			}
			trianglesContainerTmp[minDistanceIndex].emplace_back(trianglesTmp[i]);
		}
		// erase the original clusters triangles and copy the new assignment to it
		for (int i = 0; i < planes.size(); i++)
		{
			clusters[i].triangles = trianglesContainerTmp[i];
		}

		// -------------------------------------------------------------------------------------------
		//  [step 3] -- Iterative Removal of Minimum Coverage Clusters
		// -------------------------------------------------------------------------------------------
		COUT << "step 3" << std::endl;

		int step3_epoch = 0;
		bool stopSign = false;
		bool firstLoop = true;
		std::vector<float> localMin(clusters.size(), FLT_MAX);
		std::vector<float> localMinTmp(clusters.size(), 0.0f);
		while (!stopSign)
		{
			++step3_epoch;
			COUT << "step3_epoch: " << step3_epoch << std::endl;

			// redistribute the smallest cluster's triangle to the remaining clusters
			int minclusterNum = INT_MAX;
			int smallestClusterIndex = 0;
			for (int i = 0; i < clusters.size(); i++)
			{
				if (minclusterNum > clusters[i].triangles.size())
				{
					minclusterNum = clusters[i].triangles.size();
					smallestClusterIndex = i;
				}
			}
			std::vector<Triangle> redistributeTriangles = clusters[smallestClusterIndex].triangles;
			clusters.erase(clusters.begin() + smallestClusterIndex);
			for (auto& triangle : redistributeTriangles)
			{
				float minDistance = FLT_MAX;
				int minDistaceIndex = 0;
				for (int i = 0; i < clusters.size(); i++)
				{
					float distanceTmp = clusters[i].plane.calcuTotalDistance(triangle);
					if (minDistance > distanceTmp)
					{
						minDistance = distanceTmp;
						minDistaceIndex = i;
					}
				}
				clusters[minDistaceIndex].triangles.emplace_back(triangle);
			}
			// update clusters
			for (auto& cluster : clusters)
			{
				cluster.update();
			}
			// creat new cluster
			// first find the cluster with largest triangles num
			// then pick a triangle with max distance in this cluster
			// lastly move this triangle to the new cluster
			int largestCluster = 0;
			int largestClusterIndex = 0;
			for (int i = 0; i < clusters.size(); i++)
			{
				if (largestCluster < clusters[i].triangles.size())
				{
					largestCluster = clusters[i].triangles.size();
					largestClusterIndex = i;
				}
			}
			float maxDistance = 0.0f;
			int maxDistanceIndex = 0;
			for (int i = 0; i < clusters[largestClusterIndex].triangles.size(); i++)
			{
				float distanceTmp = glm::length(clusters[largestClusterIndex].getCentriod() - clusters[largestClusterIndex].triangles[i].getCentriod());
				if (maxDistance < distanceTmp)
				{
					maxDistance = distanceTmp;
					maxDistanceIndex = i;
				}
			}
			std::vector<Triangle> tmp(1, clusters[largestClusterIndex].triangles[maxDistanceIndex]);
			Cluster clusterTmp(tmp);
			clusters.emplace_back(clusterTmp);
			clusters[largestClusterIndex].triangles.erase(clusters[largestClusterIndex].triangles.begin() + maxDistanceIndex);
			// update clusters
			for (auto& cluster : clusters)
			{
				cluster.update();
			}
			// calculate the current clusters' radius(defined as largest distance between the centroid and any vertex of that cluster)
			for (int i = 0; i < clusters.size(); i++)
			{
				glm::vec3 centriodTmp = clusters[i].getCentriod();
				float radius = 0.0f;
				for (int j = 0; j < clusters[i].triangles.size(); j++)
				{
					float maxDistance;
					float distance0 = glm::length(clusters[i].triangles[j].p0 - centriodTmp);
					float distance1 = glm::length(clusters[i].triangles[j].p1 - centriodTmp);
					float distance2 = glm::length(clusters[i].triangles[j].p2 - centriodTmp);
					maxDistance = distance0 > distance1 ? distance0 : distance1;
					maxDistance = maxDistance > distance2 ? maxDistance : distance2;
					radius = radius > maxDistance ? radius : maxDistance;
				}
				localMinTmp[i] = radius;
			}

			///*for debug use*/
			//std::vector<bool> test(k, false);
			//for (int i = 0; i < k; i++)
			//{
			//	if (localMinTmp[i] <= localMin[i])
			//	{
			//		test[i] = true;
			//	}
			//}

			// verify whether the stop condition is satisfied
			bool stopSignTmp = true;
			for (int i = 0; i < clusters.size(); i++)
			{
				if (localMinTmp[i] > localMin[i])
				{
					stopSignTmp = false;
				}
				localMin[i] = localMinTmp[i];
			}
			stopSign = stopSignTmp;
			if (firstLoop)
			{
				stopSign = false;
				firstLoop = false;
			}
		}

		///*for debug use*/
		//// test whether the direction of the plane's normal is right or not
		//std::vector<float> distance;
		//for (int i = 0; i < clusters.size(); i++)
		//{
		//	float max = 0.0f;
		//	for (auto triangle : clusters[i].triangles)
		//	{
		//		float tmp = clusters[i].plane.calcuDistance(triangle.getCentriod());
		//		if (max < tmp)
		//		{
		//			max = tmp;
		//		}
		//	}
		//	distance.emplace_back(max);
		//}

		//*****************************
		// k means cluster:[main step]
		//*****************************
		COUT << "main step" << std::endl;
		int mainstep_epoch = 0;
		bool stopSign_k = false;
		bool firstLoop_k = true;
		std::vector<float> totalMin_k(clusters.size(), FLT_MAX);
		std::vector<float> totalMinTmp_k(clusters.size(), 0.0f);
		while (!stopSign_k)
		{
			++mainstep_epoch;
			// break the loop according to specified max iteration
			if (mainstep_epoch > maxIter)
				break;
			//std::cout << "main_step_epoch: " << mainstep_epoch << std::endl;

			// first clear the triangles that stored in the cluster
			for (int i = 0; i < clusters.size(); i++)
			{
				clusters[i].triangles.clear();
			}
			// assign all the triangles to the corresponding clusters according to the specific distance metric
			for (auto& triangle : trianglesTmp)
			{
				float minDistance = FLT_MAX;
				int minDistanceIndex = 0;
				for (int i = 0; i < clusters.size(); i++)
				{
					float distanceTmp = clusters[i].plane.calcuTotalDistance(triangle);
					if (minDistance > distanceTmp)
					{
						minDistance = distanceTmp;
						minDistanceIndex = i;
					}
				}
				clusters[minDistanceIndex].triangles.emplace_back(triangle);
			}
			// update clusters
			for (auto& cluster : clusters)
			{
				cluster.update();
			}
			// get all current clusters' total distance
			for (int i = 0; i < clusters.size(); i++)
			{
				Plane bestFittedTmp = clusters[i].getBestFittedPlane();
				float totalTmp = 0.0f;
				for (auto& triangle : clusters[i].triangles)
				{
					totalTmp += bestFittedTmp.calcuPointDistance(triangle.getCentriod());
				}
				totalMinTmp_k[i] = totalTmp;
			}

			///*for debug use*/
			//std::vector<bool> test(k, false);
			//if (!firstLoop)
			//{
			//	for (int i = 0; i < k; i++)
			//	{
			//		if (totalMinTmp_k[i] <= totalMin_k[i])
			//		{
			//			test[i] = true;
			//		}
			//	}
			//}

			// verify whether the stop condition is satisfied
			// it terminates if a local minimum for this total distance is reached
			bool stopSignTmp_k = true;
			for (int i = 0; i < clusters.size(); i++)
			{
				if (totalMinTmp_k[i] > totalMin_k[i])
				{
					stopSignTmp_k = false;
				}
				totalMin_k[i] = totalMinTmp_k[i];
			}
			stopSign_k = stopSignTmp_k;
			if (firstLoop_k)
			{
				stopSign_k = false;
				firstLoop_k = false;
			}
		}

		///*for debug use*/
		//for (auto& cluster : clusters)
		//{
		//	if (cluster.triangles.size() == 0)
		//	{
		//		COUT << "ERROR: there exists empty cluster!" << std::endl;
		//		system("pause");
		//		exit(1);
		//	}
		//}

		//*****************************
		// copy data
		//*****************************

		for (auto& cluster : clusters)
		{
			trianglesBeforeProj.emplace_back(cluster.triangles);
			bbc.emplace_back(cluster.plane);
		}
	}

	/// get the corresponding bbc triangles index in the original mesh
	void copyMeshIndicesIndex()
	{
		for (auto& triangles : trianglesBeforeProj)
		{
			std::vector<unsigned int> indice(triangles.size() * 3);
			for (auto& triangle : triangles)
			{
				indice.emplace_back(triangle.indices.x);
				indice.emplace_back(triangle.indices.y);
				indice.emplace_back(triangle.indices.z);
			}
			bbcMeshIndicesIndex.emplace_back(indice);
		}
	}

	/// project the triangles onto the corresponding bbc plane
	void projTrianglesOntoPlane()
	{
		for (int i = 0; i < bbc.size(); i++)
		{
			std::vector<Triangle> trianglesAfterProjTmp;
			for (auto& triangleBeforeProj : trianglesBeforeProj[i])
			{
				Triangle triangleAfterProj;

				// origin point(X0,Y0,Z0),  projected point(X,Y,Z)
				// X=X0-At, Y=Y0-Bt, Z=Z0-Ct,
				// t=(A*X0+B*Y0+C*Z0+D)/(A*A+B*B+C*C)
				float t1 = (bbc[i].para.x * triangleBeforeProj.p0.x +
					bbc[i].para.y * triangleBeforeProj.p0.y +
					bbc[i].para.z * triangleBeforeProj.p0.z + bbc[i].para.w) /
					(bbc[i].para.x * bbc[i].para.x +
						bbc[i].para.y * bbc[i].para.y +
						bbc[i].para.z * bbc[i].para.z);
				triangleAfterProj.p0 = glm::vec3(triangleBeforeProj.p0.x - bbc[i].para.x * t1,
					triangleBeforeProj.p0.y - bbc[i].para.y * t1,
					triangleBeforeProj.p0.z - bbc[i].para.z * t1);

				float t2 = (bbc[i].para.x * triangleBeforeProj.p1.x +
					bbc[i].para.y * triangleBeforeProj.p1.y +
					bbc[i].para.z * triangleBeforeProj.p1.z + bbc[i].para.w) /
					(bbc[i].para.x * bbc[i].para.x +
						bbc[i].para.y * bbc[i].para.y +
						bbc[i].para.z * bbc[i].para.z);
				triangleAfterProj.p1 = glm::vec3(triangleBeforeProj.p1.x - bbc[i].para.x * t2,
					triangleBeforeProj.p1.y - bbc[i].para.y * t2,
					triangleBeforeProj.p1.z - bbc[i].para.z * t2);

				float t3 = (bbc[i].para.x * triangleBeforeProj.p2.x +
					bbc[i].para.y * triangleBeforeProj.p2.y +
					bbc[i].para.z * triangleBeforeProj.p2.z + bbc[i].para.w) /
					(bbc[i].para.x * bbc[i].para.x +
						bbc[i].para.y * bbc[i].para.y +
						bbc[i].para.z * bbc[i].para.z);
				triangleAfterProj.p2 = glm::vec3(triangleBeforeProj.p2.x - bbc[i].para.x * t3,
					triangleBeforeProj.p2.y - bbc[i].para.y * t3,
					triangleBeforeProj.p2.z - bbc[i].para.z * t3);

				// only just store the p0,p1,p2 data for triangleAfterProj, no normal and indices data
				trianglesAfterProjTmp.emplace_back(triangleAfterProj);
			}
			trianglesAfterProj.emplace_back(trianglesAfterProjTmp);
		}
	}

	/// find minimal bounding box
	void genBoundingRectangle()
	{
		// init bbcRectangle as the size of the bbc
		bbcRectangle.resize(bbc.size());
		point2d *points;
		for (int i = 0; i < bbc.size(); i++)
		{
			std::vector<glm::vec3> proPoints;

			// first trans to billboard plane coordinate
			glm::vec3 z_axis = bbc[i].normal;
			glm::vec3 x_axis_tmp = glm::vec3(0, 0, 1);
			glm::vec3 y_axis = glm::normalize(glm::cross(z_axis, x_axis_tmp));
			glm::vec3 x_axis = glm::normalize(glm::cross(y_axis, z_axis));
			glm::mat4 rotateMat;
			rotateMat[0] = glm::vec4(x_axis, 0.0f);
			rotateMat[1] = glm::vec4(y_axis, 0.0f);
			rotateMat[2] = glm::vec4(z_axis, 0.0f);
			rotateMat[3] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
			glm::mat4 rotateMatReverse = glm::transpose(rotateMat);   // reverse of the matrix
			for (auto& triangleAfterProj : trianglesAfterProj[i])
			{
				glm::vec4 p0_tmp = rotateMatReverse * glm::vec4(triangleAfterProj.p0, 1.0f);
				glm::vec4 p1_tmp = rotateMatReverse * glm::vec4(triangleAfterProj.p1, 1.0f);
				glm::vec4 p2_tmp = rotateMatReverse * glm::vec4(triangleAfterProj.p2, 1.0f);
				proPoints.emplace_back(glm::vec3(p0_tmp.x, p0_tmp.y, p0_tmp.z));
				proPoints.emplace_back(glm::vec3(p1_tmp.x, p1_tmp.y, p1_tmp.z));
				proPoints.emplace_back(glm::vec3(p2_tmp.x, p2_tmp.y, p2_tmp.z));
			}

			int pointNum = proPoints.size();
			points = new point2d[pointNum];
			for (int k = 0; k < pointNum; k++)
			{
				points[k].x = proPoints[k].x;
				points[k].y = proPoints[k].y;
			}
			point2d rectangle[4];
			// "rotating Calipers" algorithm
			rotatingCalipers(points, pointNum, rectangle);

			// then trans back to billboard plane
			for (int m = 0; m < 4; m++)
			{
				glm::vec4 pointTmp = glm::vec4(rectangle[m].x, rectangle[m].y, proPoints[0].z, 1.0f);
				pointTmp = rotateMat * pointTmp;
				glm::vec3 point = glm::vec3(pointTmp.x, pointTmp.y, pointTmp.z);

				if (m == 0)
				{
					bbcRectangle[i].p0 = point;
				}
				else if (m == 1)
				{
					bbcRectangle[i].p1 = point;
				}
				else if (m == 2)
				{
					bbcRectangle[i].p2 = point;
				}
				else if (m == 3)
				{
					bbcRectangle[i].p3 = point;
				}
			}
			bbcRectangle[i].update();

			//// unanotate the following code sometimes will cause bug: it shows that delete the same pointer mutiple times
			//delete []points;
			//points = nullptr;
		}
	}

	/// crack reduction
	void crackReduction()
	{
		std::vector<std::vector<Triangle>> trianglesBeforeProjTmp = trianglesBeforeProj;

		// first calculate the envelop's range
		std::vector<float> envelopsDist(bbc.size());
		for (int i = 0; i < bbc.size(); i++)
		{
			float maxDist = 0.0f;
			for (auto& triangle : trianglesBeforeProjTmp[i])
			{
				maxDist = bbc[i].calcuMaxDistance(triangle);
			}
			envelopsDist[i] = maxDist;
		}

		// For all intersecting envelopes we project those triangles which lie inside their intersection onto both planes
		for (int i = 0; i < envelopsDist.size(); i++)
		{
			for (int j = 0; j < trianglesBeforeProjTmp.size(); j++)
			{
				if (j != i)
				{
					for (auto& triangle : trianglesBeforeProjTmp[j])
					{
						if (bbc[i].calcuMaxDistance(triangle) < envelopsDist[i])
						{
							trianglesBeforeProj[i].emplace_back(triangle);
							// If only a part of a triangle lies within the envelope of another plane we project only that part
							if (bbc[i].calcuMinDistance(triangle) < envelopsDist[i])
							{
								// to do...
								// it should be realized by stencil buffer with the shaders
							}
						}
					}
				}
			}
		}
	}

	/// render to texture
	void renderToTexture()
	{
		// prepare parameter for rendering to texture
		BoundingSphere bs(trianglesOrg);
		bbsWidthResolution.clear();
		bbsHeightResolution.clear();
		for (int i = 0; i < bbc.size(); i++)
		{
			// The target resolution is defined by the configured texture resolution 
			// and the ratio between the bounding sphere diameterd and the extent of the billboard plane
			// Using this equation, all texels will map an area with the same size in object coordinates
			float widthResolution = bbcRectangle[i].axisXLength * IMG_WIDTH / (bs.radius * 2);
			float heightResolution = bbcRectangle[i].axisYLength * IMG_HEIGHT / (bs.radius * 2);
			// if the calculated resolution is 0, that will cause error in the following render to texture step("Framebuffer not complete error")
			// so just skip that bb render
			// but if the skipped num is too large, we should check the generated billboard plane bounding rectangle points
			if (widthResolution == 0 ||
				heightResolution == 0 ||
				widthResolution<1 ||
				widthResolution>1.0e6 ||
				heightResolution<1 ||
				heightResolution>1.0e6)
			{
				skipFaceNum += bbcMeshIndicesIndex[i].size() / 3;
				continue;
			}
			bbsWidthResolution.emplace_back(widthResolution);
			bbsHeightResolution.emplace_back(heightResolution);

			// setup billboards
			Billboard tmp(bbcRectangle[i], glfw);   // note: have OpenGL relevant functions in it !!!
			bbs.emplace_back(tmp);

			// find the indices in the original mesh
			std::vector<Vertex> verticesTmp;
			for (auto& indiceIndex : bbcMeshIndicesIndex[i])
			{
				float indice = mesh->indices[indiceIndex];
				verticesTmp.emplace_back(mesh->vertices[indice]);
			}
			Mesh meshTmp(verticesTmp, mesh->textures);   // note: have OpenGL relevant functions in it !!!
			bbMeshes.emplace_back(meshTmp);
		}
		// render to texture
		for (int i = 0; i < bbs.size(); i++)
		{
			// define The target resolution, two ways:
			// (1). all bbc texture resolution is set as the same
			/*bbs[i].setTextureFromFrameBuffer(IMG_WIDTH, IMG_HEIGHT);
			glBindFramebuffer(GL_FRAMEBUFFER, bbs[i].frameBuffer);
			glViewport(0, 0, IMG_WIDTH, IMG_HEIGHT);*/
			// (2). bbc texture resolution is set acoording to the bbc size
			bbs[i].setTextureFromFrameBuffer(bbsWidthResolution[i], bbsHeightResolution[i]);
			glBindFramebuffer(GL_FRAMEBUFFER, bbs[i].frameBuffer);
			glViewport(0, 0, bbsWidthResolution[i], bbsHeightResolution[i]);

			// set uniform for shader
			float rectangle_x_length = bbcRectangle[i].axisXLength;
			float rectangle_y_length = bbcRectangle[i].axisYLength;
			glm::vec3 camPosition = glm::vec3(0, 0, 100);
			glm::mat4 bbTexGenProjection = glm::ortho(-rectangle_x_length / 2, rectangle_x_length / 2, -rectangle_y_length / 2, rectangle_y_length / 2, 0.1f, 1000.0f);
			glm::mat4 bbTexGenView = glm::lookAt(camPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
			glm::mat4 bbTexGenModel = bbcRectangle[i].getTransMat(bbc[i].normal);
			glm::vec3 lightPos = glm::vec3(10, 15, 10);
			textureGenShader.use();
			textureGenShader.setMat4("projection", bbTexGenProjection);
			textureGenShader.setMat4("view", bbTexGenView);
			textureGenShader.setMat4("model", bbTexGenModel);
			textureGenShader.setVec3("lightPos", lightPos);
			textureGenShader.setVec3("viewPos", camPosition);
			textureGenShader.setBool("blinn", false);
			textureGenShader.setBool("alphaTest", true);
			bbMeshes[i].render(textureGenShader);
		}
		// bind back to default framebuffer !
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}

	/// get the texture image from famebuffer color attachment
	/// note: this method must be called in the main thread!
	void readPixelFromFramebuffer()
	{
		if (!saveComplete)
		{
			for (auto& d : textureAtlas)
			{
				if (d != nullptr)
				{
					free(d);
				}
			}
		}
		textureAtlas.resize(bbs.size());

		for (int i = 0; i < bbs.size(); i++)
		{
			// Method #1: "glReadPixels"
			glBindFramebuffer(GL_FRAMEBUFFER, bbs[i].frameBuffer);
			textureAtlas[i] = (BYTE *)malloc((int)(bbsWidthResolution[i] * bbsHeightResolution[i] * 4));
			if (textureAtlas[i] != nullptr)
			{
				glReadPixels(0, 0, bbsWidthResolution[i], bbsHeightResolution[i], GL_RGBA, GL_UNSIGNED_BYTE, textureAtlas[i]);
			}
			glBindFramebuffer(GL_FRAMEBUFFER, 0);

			//// Method #2: "glGetTexImage" (access violation error?)
			//glBindTexture(GL_TEXTURE_2D, bbs[i].texture);
			//textureData[i] = (BYTE *)malloc((int)(bbsWidthResolution[i] * bbsHeightResolution[i] * 4));
			//if (textureData[i] != nullptr)
			//{
			//	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, textureData[i]);
			//}
			//glBindTexture(GL_TEXTURE_2D, 0);

			// Method #3: "PixelBufferObjects" (slightly faster method than the previous two)
			// to do...
		}
	}

	/// write into texture atlas image file with sub-thread
	/// note: this method can only be called one time after "getTexImage" method being called one time
	void writeToTextureAtlasAsync()
	{
		std::string filename = bbcPath + algorithmType + "/" + meshName + "_pack.png";
		try
		{
			auto& complete = saveComplete;
			auto& width = bbsWidthResolution;
			auto& height = bbsHeightResolution;

			// copy the data pointer (the memory pointer)
			// every writeToTextureAtlasPng operation will move the textureAtlas data, and then will free it automatically
			std::vector<BYTE*> data = textureAtlas;  
			for (auto& texture : textureAtlas)
			{
				texture = nullptr;    // set the original data pointer to null
			}

			// note: we specify the packed image's width and height instead of defining it adaptively in the algorithm
			// since the current packing algorithm implemented have no ability to do it
			// if the width and height you choose is smaller than the texture which need to be packed, then the unpacked texture will be skipped
			// so curently we choose a big enough size to pack all the texture
			int packWidth = 4096;
			int packHeight = 4096;

			std::thread t([filename, width, height, data, packWidth, packHeight] {
				writeToTextureAtlasPng(filename.c_str(), width, height, data, packWidth, packHeight);
				for (auto d : data)
				{
					free(d);
				}
			});
			t.detach();
		}
		catch (const std::exception& ex)
		{
			std::cout << ex.what() << std::endl;
		}
	}

	/// update the data while regenerating
	void updateCache()
	{
		bbc.clear();
		bbc.shrink_to_fit();
		bbs.clear();
		bbs.shrink_to_fit();
		bbMeshes.clear();
		bbMeshes.shrink_to_fit();
		bbcRectangle.clear();
		bbcRectangle.shrink_to_fit();
	}

	/// destroy tmp variable after generating billboard clouds
	void destroyTmp()
	{
		trianglesBeforeProj.clear();
		trianglesBeforeProj.shrink_to_fit();
		trianglesAfterProj.clear();
		trianglesAfterProj.shrink_to_fit();
		bbcMeshIndicesIndex.clear();
		bbcMeshIndicesIndex.shrink_to_fit();
	}
};

#endif // !BILLBOARDCLOUDS_H
