#ifndef VIEWER_H
#define VIEWER_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "object/object.h"
#include "object/skybox.h"
#include "object/floor.h"
#include "object/text.h"
#include "bbc/billboardclouds.h"
#include "bbc/imposter.h"
#include "glfw.h"
#include "shader.h"
#include "model.h"
#include "config.h"
#include <time.h>
#include <thread>
#include <direct.h>
#include <iostream>

extern std::string resPath;

class Viewer
{
public:
	void view()
	{
		char buffer[MAX_PATH_LEN];
		_getcwd(buffer, MAX_PATH_LEN);

		resPath = buffer;
		resPath.append("\\..\\assets\\");

		std::string demo;
		while (demo != "demo1" &&
			demo != "demo2")
		{
			std::cout << "please input: demo + number (number range: 1-2)" << std::endl;
			std::cout << ">> ";
			std::cin >> demo;
		}

		if (demo == "demo1")
		{
			demo1();
		}
		else if (demo == "demo2")
		{
			demo2();
		}
	}

	/// render billboard and imposter (nanosuit)
	void demo1()
	{
		// glfw
		Glfw glfw;
		glfw.init();

		// shaders
		Shader floorShader("shaders\\floor.vs", "shaders\\floor.fs");
		Shader skyboxShader("shaders\\skybox.vs", "shaders\\skybox.fs");
		Shader nanosuitShader("shaders\\nanosuit.vs", "shaders\\nanosuit.fs");
		Shader cameraShader("shaders\\camera.vs", "shaders\\camera.fs");
		Shader bbRenderShader("shaders\\bbRender.vs", "shaders\\bbRender.fs");
		Shader bbTexGenShader("shaders\\bbTextureGen.vs", "shaders\\bbTextureGen.fs");
		Shader imposterRenderShader("shaders\\imposterRender.vs", "shaders\\imposterRender.fs");
		Shader imposterTexGenShader("shaders\\imposterTextureGen.vs", "shaders\\imposterTextureGen.fs");

		// path
		std::string flrPath = resPath + "textures\\chess.jpg";
		std::vector<std::string> skyboxPath;
		skyboxPath.emplace_back(resPath + "textures\\skybox\\right.jpg");
		skyboxPath.emplace_back(resPath + "textures\\skybox\\left.jpg");
		skyboxPath.emplace_back(resPath + "textures\\skybox\\top.jpg");
		skyboxPath.emplace_back(resPath + "textures\\skybox\\bottom.jpg");
		skyboxPath.emplace_back(resPath + "textures\\skybox\\front.jpg");
		skyboxPath.emplace_back(resPath + "textures\\skybox\\back.jpg");
		std::string nanosuitPath = resPath + "objects\\nanosuit\\nanosuit.obj";
		std::string cameraPath = resPath + "objects\\camera\\camera.obj";

		// texture
		unsigned int floorTexture = loadTexture(flrPath.c_str());
		unsigned int skyboxTexture = loadCubemap(skyboxPath);

		// object
		std::cout << "INFO: loading model ..." << std::endl;
		Model nanosuitModel(nanosuitPath);
		Model cameraModel(cameraPath);
		Floor floor;
		Skybox skybox;
		Billboard bb(glfw);
		Imposter imposter(glfw);
		Object nanosuit(&nanosuitModel, glfw);
		Object camera(&cameraModel, glfw);

		// setup camera
		std::vector<Camera> bbcCams;
		glm::vec3 center = glm::vec3(0.0f, 0.0f, 0.0f);
		std::vector<glm::vec3> points = gen_lat_lon_sphere_point(center, 4.0f, 8, 8);
		for (auto& point : points)
		{
			glm::vec3 camLookDir = point - center;
			Camera camera(point, camLookDir);
			bbcCams.emplace_back(camera);
		}

		// setup billboard and imposter
		glm::vec3 nanosuitTranslate = glm::vec3(0.0f, -1.5f, 0.0f);
		glm::vec4 nanosuitRotate = glm::vec4(0.0f, 1.0f, 0.0f, glm::radians(0.0f));
		glm::vec3 nanosuitScale = glm::vec3(0.2f, 0.2f, 0.2f);
		bb.renderTotexture(nanosuit, bbTexGenShader, nanosuitTranslate, nanosuitRotate, nanosuitScale);
		imposter.renderTotexture(nanosuit, imposterTexGenShader, nanosuitTranslate, nanosuitRotate, nanosuitScale);

		// bind to default Framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glEnable(GL_DEPTH_TEST);
		// render loop
		while (!glfw.shouldClose())
		{
			glfw.updateState();

			// render the nanosuit
			nanosuit.position = glm::vec3(0.0f, -1.5f, 0.0f);
			nanosuit.scale = glm::vec3(0.2f, 0.2f, 0.2f);
			nanosuit.render(nanosuitShader);

			// render the camera in the sphere point
			for (int i = 0; i < points.size(); i++)
			{
				camera.position = points[i];
				camera.rotate = glm::vec4(glm::vec3(0, 1, 0), pi);
				camera.rotateMat = imposter.rotMats[i];
				camera.scale = glm::vec3(0.8f, 0.8f, 0.8f);
				camera.render(cameraShader);
			}

			// render the billboard (view-point oriented)
			bb.position = glm::vec3(-8.0f, 0.0f, 0.0f);
			bb.scale = glm::vec3(2.0f, 2.0f, 2.0f);
			bb.render(bbRenderShader, "view point oriented");

			// render the imposter texture atalas (in a billboard)
			imposter.bb.position = glm::vec3(12.0f, 0.5f, 0.0f);
			imposter.bb.scale = glm::vec3(2.0f, 2.0f, 2.0f);
			imposter.bb.render(bbRenderShader, "view point oriented");

			// render the imposter 
			imposter.position = glm::vec3(8.0f, 0.0f, 0.0f);
			imposter.scale = glm::vec3(2.0f, 2.0f, 2.0f);
			imposter.render(imposterRenderShader);

			// render the floor
			glm::mat4 model_floor;
			model_floor = glm::translate(model_floor, glm::vec3(0.0f, -1.75f, 0.0f));
			model_floor = glm::scale(model_floor, glm::vec3(15.0f, 15.0f, 15.0f));
			floorShader.use();
			floorShader.setMat4("projection", glfw.mainCamProjection);
			floorShader.setMat4("view", glfw.mainCamView);
			floorShader.setMat4("model", model_floor);
			floorShader.setVec3("lightPos", glfw.lightPos);
			floorShader.setVec3("viewPos", glfw.mainCamPos);
			floorShader.setBool("blinn", glfw.blinn);
			floor.render(floorShader, floorTexture);

			// render the skybox ( render at last )
				// change depth function so depth test passes when values are equal to depth buffer's content
			glDepthFunc(GL_LEQUAL);
			glm::mat4 model_skybox;
			model_skybox = glm::translate(model_skybox, glm::vec3(0.0f, 0.0f, 0.0f));
			model_skybox = glm::scale(model_skybox, glm::vec3(800.0f, 800.0f, 800.0f));
			skyboxShader.use();
			skyboxShader.setMat4("projection", glfw.mainCamProjection);
			skyboxShader.setMat4("view", glfw.mainCamView);
			skyboxShader.setMat4("model", model_skybox);
			skybox.render(skyboxShader, skyboxTexture);
			// set depth function back to default
			glDepthFunc(GL_LESS);

			// glfw setting
			glfw.lastFrameSign = glfw.curFrameSign;
			glfw.curFrameSign = false;
		}
		glfw.destroy();
	}

	/// render billboard clouds (tree1)
	void demo2()
	{
		// glfw
		Glfw glfw;
		glfw.init();

		// path
		std::string treePath = resPath + "objects\\tree1\\tree.obj";
		std::string fontPath = resPath + "fonts\\arial.ttf";

		// shaders
		Shader textShader("shaders\\text.vs", "shaders\\text.fs");
		Shader treeShader("shaders\\tree1.vs", "shaders\\tree1.fs");
		Shader bbcRenderShader("shaders/bbRender.vs", "shaders\\bbRender.fs");
		Shader bbcTextureGenShader("shaders\\bbTextureGen.vs", "shaders\\bbTextureGen.fs");

		// object
		std::cout << "INFO: loading model..." << std::endl;
		TextASCII text(fontPath, glfw);
		Model treeModel(treePath);
		Object tree(&treeModel, glfw);
		BillboardCloud bbc_leaf_original(&treeModel.meshes[1], bbcTextureGenShader, glfw, "leaf");
		BillboardCloud bbc_stem_original(&treeModel.meshes[0], bbcTextureGenShader, glfw, "stem");
		BillboardCloud bbc_leaf_stochastic(&treeModel.meshes[1], bbcTextureGenShader, glfw, "leaf");
		BillboardCloud bbc_stem_stochastic(&treeModel.meshes[0], bbcTextureGenShader, glfw, "stem");
		BillboardCloud bbc_leaf_kmeans(&treeModel.meshes[1], bbcTextureGenShader, glfw, "leaf");
		BillboardCloud bbc_stem_kmeans(&treeModel.meshes[0], bbcTextureGenShader, glfw, "stem");

		// defalut generation
		bbc_leaf_original.generateAsync(0, 10, 0.005);
		bbc_stem_original.generateAsync(0, 10, 0.01);
		bbc_leaf_stochastic.generateAsync(1, 500, 0.021);
		bbc_stem_stochastic.generateAsync(1, 500, 0.017);
		bbc_leaf_kmeans.generateAsync(2, 50, 0.0, 100);
		bbc_stem_kmeans.generateAsync(2, 50, 0.0, 100);

		// user specify input to generate bbc
		std::thread pollInput(&Viewer::pollEvent, this,
			&bbc_leaf_original, &bbc_stem_original,
			&bbc_leaf_stochastic, &bbc_stem_stochastic,
			&bbc_leaf_kmeans, &bbc_stem_kmeans);
		pollInput.detach();

		// bind to default Framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glEnable(GL_DEPTH_TEST);

		// render loop
		while (!glfw.shouldClose())
		{
			// update glfw state
			glfw.updateState();

			float r = glm::sin(glfw.getTime());
			float g = glm::sin(glfw.getTime() / 2);
			float b = glm::sin(glfw.getTime() / 3);

			// text
			text.text = "billboard clouds";
			text.position = glm::vec3(-3.7f, 4.7f, 0.0f);
			text.scale = glm::vec3(2.0f, 2.0f, 2.0f);
			text.color = glm::vec3(r, g, b);
			text.render(textShader);
			text.scale = glm::vec3(1.0f, 1.0f, 1.0f);

			// text
			text.text = "mesh";
			text.position = glm::vec3(-8.2f, 3.0f, 0.0f);
			text.color = glm::vec3(0.7, 0.7f, 0.7f);
			text.render(textShader);
			text.text = "face num: " + std::to_string(treeModel.getFaceNum());
			text.position = glm::vec3(-9.3f, 2.0f, 0.0f);
			text.color = glm::vec3(0.5, 0.5f, 0.5f);
			text.render(textShader);
			// mesh
			tree.position = glm::vec3(-7.5f, -2.2f, 0.0f);
			tree.render(treeShader);

			// text
			text.text = "original";
			text.position = glm::vec3(-3.4f, 3.0f, 0.0f);
			text.color = glm::vec3(0.7, 0.7f, 0.7f);
			text.render(textShader);
			text.text = "bbc num: " + std::to_string(bbc_leaf_original.bbcNum + bbc_stem_original.bbcNum);
			text.position = glm::vec3(-3.8f, 2.0f, 0.0f);
			text.color = glm::vec3(0.5f, 0.5f, 0.5f);
			text.render(textShader);
			// bbc original
			bbc_leaf_original.computeTexture();
			bbc_leaf_original.position = glm::vec3(-2.5f, -2.2f, 0.0f);
			bbc_leaf_original.render(bbcRenderShader, treeShader);
			bbc_stem_original.computeTexture();
			bbc_stem_original.position = glm::vec3(-2.5f, -2.2f, 0.0f);
			bbc_stem_original.render(bbcRenderShader, treeShader);

			// text
			text.text = "stochastic";
			text.position = glm::vec3(1.4f, 3.0f, 0.0f);
			text.color = glm::vec3(0.7, 0.7f, 0.7f);
			text.render(textShader);
			text.text = "bbc num: " + std::to_string(bbc_leaf_stochastic.bbcNum + bbc_stem_stochastic.bbcNum);
			text.position = glm::vec3(1.3f, 2.0f, 0.0f);
			text.color = glm::vec3(0.5f, 0.5f, 0.5f);
			text.render(textShader);
			// bbc stochastic
			bbc_leaf_stochastic.computeTexture();
			bbc_leaf_stochastic.position = glm::vec3(2.5f, -2.2f, 0.0f);
			bbc_leaf_stochastic.render(bbcRenderShader, treeShader);
			bbc_stem_stochastic.computeTexture();
			bbc_stem_stochastic.position = glm::vec3(2.5f, -2.2f, 0.0f);
			bbc_stem_stochastic.render(bbcRenderShader, treeShader);

			// text kmeans
			text.text = "kmeans";
			text.position = glm::vec3(6.4f, 3.0f, 0.0f);
			text.color = glm::vec3(0.7, 0.7f, 0.7f);
			text.render(textShader);
			text.text = "bbc num: " + std::to_string(bbc_leaf_kmeans.bbcNum + bbc_stem_kmeans.bbcNum);
			text.position = glm::vec3(6.0f, 2.0f, 0.0f);
			text.color = glm::vec3(0.5f, 0.5f, 0.5f);
			text.render(textShader);
			// bbc
			bbc_leaf_kmeans.computeTexture();
			bbc_leaf_kmeans.position = glm::vec3(7.5f, -2.2f, 0.0f);
			bbc_leaf_kmeans.render(bbcRenderShader, treeShader);
			bbc_stem_kmeans.computeTexture();
			bbc_stem_kmeans.position = glm::vec3(7.5f, -2.2f, 0.0f);
			bbc_stem_kmeans.render(bbcRenderShader, treeShader);
		}
		glfw.destroy();
	}

private:
	/// note: we need pass the pointer of the object to it instead of its reference
	/// since the muti-pass-by-reference will refer not the same original object
	void pollEvent(BillboardCloud* bbc_leaf_original, BillboardCloud* bbc_stem_original,
		BillboardCloud* bbc_leaf_stochastic, BillboardCloud* bbc_stem_stochastic,
		BillboardCloud* bbc_leaf_kmeans, BillboardCloud* bbc_stem_kmeans)
	{
		while (true)
		{
			std::cout << "-----------------------------------------" << std::endl;
			std::cout << "INFO: save or regenerate?" << std::endl;
			std::cout << "INFO: 0: regenerate, 1: save" << std::endl;
			int mode;
			std::cout << ">> ";
			std::cin >> mode;
			if (mode == 0)
			{
				std::cout << "-----------------------------------------" << std::endl;
				std::cout << "INFO: input algorithm number" << std::endl;
				std::cout << "INFO: 0: original, 1: stochastic, 2: kmeans" << std::endl;
				int algorithmType;
				std::cout << ">> ";
				std::cin >> algorithmType;
				if (algorithmType != 0 &&
					algorithmType != 1 &&
					algorithmType != 2)
				{
					std::cout << "ERROR: wrong number!" << std::endl;
				}
				else
				{
					while (true)
					{
						std::cout << "INFO: input mesh number" << std::endl;
						std::cout << "INFO: 0: leaf, 1: stem" << std::endl;
						int meshType;
						std::cout << ">> ";
						std::cin >> meshType;
						if (meshType != 0 &&
							meshType != 1)
						{
							std::cout << "ERROR: wrong number!" << std::endl;
						}
						else
						{
							if (algorithmType == 0 && meshType == 0)
							{
								std::thread t1_1(&Viewer::executeGenerateAsync, this, bbc_leaf_original, 0);
								t1_1.join();
								break;
							}
							else if (algorithmType == 0 && meshType == 1)
							{
								std::thread t1_2(&Viewer::executeGenerateAsync, this, bbc_stem_original, 0);
								t1_2.join();
								break;
							}
							else if (algorithmType == 1 && meshType == 0)
							{
								std::thread t2_1(&Viewer::executeGenerateAsync, this, bbc_leaf_stochastic, 1);
								t2_1.join();
								break;
							}
							else if (algorithmType == 1 && meshType == 1)
							{
								std::thread t2_2(&Viewer::executeGenerateAsync, this, bbc_stem_stochastic, 1);
								t2_2.join();
								break;
							}
							else if (algorithmType == 2 && meshType == 0)
							{
								std::thread t3_1(&Viewer::executeGenerateAsync, this, bbc_leaf_kmeans, 2);
								t3_1.join();
								break;
							}
							else if (algorithmType == 2 && meshType == 1)
							{
								std::thread t3_2(&Viewer::executeGenerateAsync, this, bbc_stem_kmeans, 2);
								t3_2.join();
								break;
							}
						}
					}
				}
			}
			else if (mode == 1)
			{
				std::cout << "INFO: which algorithm result to save?" << std::endl;
				std::cout << "INFO: 0: original, 1: stochastic, 2: kmeans" << std::endl;
				int algorithmType;
				std::cout << ">> ";
				std::cin >> algorithmType;
				if (algorithmType != 0 &&
					algorithmType != 1 &&
					algorithmType != 2)
				{
					std::cout << "ERROR: wrong number!" << std::endl;
				}
				else
				{
					std::cout << "INFO: input mesh number" << std::endl;
					std::cout << "INFO: 0: leaf, 1: stem" << std::endl;
					int meshType;
					std::cout << ">> ";
					std::cin >> meshType;
					if (meshType != 0 &&
						meshType != 1)
					{
						std::cout << "ERROR: wrong number!" << std::endl;
					}
					else
					{
						if (algorithmType == 0)
						{
							if (meshType == 0)
							{
								executeSaveAsync(bbc_leaf_original);
							}
							else if (meshType == 1)
							{
								executeSaveAsync(bbc_stem_original);
							}
						}
						else if (algorithmType == 1)
						{
							if (meshType == 0)
							{
								executeSaveAsync(bbc_leaf_stochastic);
							}
							else if (meshType == 1)
							{
								executeSaveAsync(bbc_stem_stochastic);
							}
						}
						else if (algorithmType == 2)
						{
							if (meshType == 0)
							{
								executeSaveAsync(bbc_leaf_kmeans);
							}
							else if (meshType == 1)
							{
								executeSaveAsync(bbc_stem_kmeans);
							}
						}
					}
				}

			}
			else
			{
				std::cout << "ERROR: wrong number!" << std::endl;
			}
		}
	}

	void executeSaveAsync(BillboardCloud* bbc)
	{
		if (bbc->genComplete && !bbc->saveComplete)
		{
			bbc->exportData();
		}
		else if (!bbc->genComplete)
		{
			std::cout << "ERROR: last " << bbc->algorithmType << "_" << bbc->meshName << "_bbc save not complete!" << std::endl;
			std::cout << "ERROR: please wait for a while ..." << std::endl;
		}
		else if (bbc->saveComplete)
		{
			std::cout << "ERROR: last " << bbc->algorithmType << "_" << bbc->meshName << "_bbc has been saved!" << std::endl;
			std::cout << "ERROR: please check the dirctory that store the result" << std::endl;
			std::cout << "INFO: before you regenerate the bbc for this mesh, this operation will be disabled all the time" << std::endl;
		}
	}

	/// execute the bbc generation algorithm with another thread
	void executeGenerateAsync(BillboardCloud* bbc, int algorithmType)
	{
		if (bbc->genComplete)
		{
			if (algorithmType == 0)
			{
				int firstPara = 0;
				float secondPara = 0.0f;
				std::cout << "INFO: input first parameter (the discretization degree of the plane space)" << std::endl;
				std::cout << "INFO: suggest: 10  (range: 10-20)" << std::endl;
				while (firstPara < 10 || firstPara > 20)
				{
					std::cout << ">> ";
					std::cin >> firstPara;
					if (firstPara < 10 || !firstPara > 20)
					{
						std::cout << "ERROR: input value out of range!  (range: 10-20)" << std::endl;
					}
				}
				while (secondPara < 0.003 || secondPara > 0.015)
				{
					std::cout << "INFO: input second parameter (the error percentage)" << std::endl;
					std::cout << "INFO: suggest: 0.01  (range: 0.003-0.015)" << std::endl;
					std::cout << ">> ";
					std::cin >> secondPara;
					if (secondPara < 0.003 || secondPara > 0.015)
					{
						std::cout << "ERROR: input value out of range!" << std::endl;
					}
				}

				bbc->generateAsync(0, firstPara, secondPara);
			}
			else if (algorithmType == 1)
			{
				int firstPara = 0;
				float secondPara = 0.0f;
				while (firstPara < 400 || firstPara > 600)
				{
					std::cout << "INFO: input first parameter (the max iteration of the algorithm)" << std::endl;
					std::cout << "INFO: suggest: 500  (range: 400-600)" << std::endl;
					std::cout << ">> ";
					std::cin >> firstPara;
					if (firstPara < 400 || firstPara > 600)
					{
						std::cout << "ERROR: input value out of range!" << std::endl;
					}
				}
				while (secondPara < 0.015 || secondPara > 0.025)
				{
					std::cout << "INFO: input second parameter (the error between the faces and its corresponding fitted plane)" << std::endl;
					std::cout << "INFO: suggest: 0.017  (range: 0.015-0.025)" << std::endl;
					std::cout << ">> ";
					std::cin >> secondPara;
					if (secondPara < 0.015 || secondPara > 0.025)
					{
						std::cout << "ERROR: input value out of range!" << std::endl;
					}
				}

				bbc->generateAsync(1, firstPara, secondPara);
			}
			else if (algorithmType == 2)
			{
				int firstPara = 0;
				int secondPara = 0;
				while (firstPara < 20 || firstPara > 150)
				{
					std::cout << "INFO: input first parameter (the bbc number you wana to generate)" << std::endl;
					std::cout << "INFO: suggest: 50  (range: 20-150)" << std::endl;
					std::cout << ">> ";
					std::cin >> firstPara;
					if (firstPara < 20 || firstPara > 150)
					{
						std::cout << "ERROR: input value out of range!" << std::endl;
					}
				}

				while (secondPara < 100 || secondPara > 300)
				{
					std::cout << "INFO: input second parameter (the max iteration of the algorithm)" << std::endl;
					std::cout << "INFO: suggest: 200  (range: 100-300)" << std::endl;
					std::cout << ">> ";
					std::cin >> secondPara;
					if (secondPara < 100 || secondPara > 300)
					{
						std::cout << "ERROR: input value out of range!" << std::endl;
					}
				}

				bbc->generateAsync(2, firstPara, 0.0f, secondPara);
			}
		}
		else
		{
			std::cout << "ERROR: last " << bbc->algorithmType << "_" << bbc->meshName << "_bbc generation not complete!" << std::endl;
			std::cout << "ERROR: please wait for a while ..." << std::endl;
		}
	}
};

#endif