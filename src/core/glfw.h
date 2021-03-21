#ifndef GLFW_SELFDEF_H
#define GLFW_SELFDEF_H

#include <glad/glad.h>
#include <glfw/glfw3.h>
#include "camera.h"
#include "config.h"
#include <iostream>


/// main camera
Camera mainCam = glm::vec3(0.0f, 1.5f, 9.0f);

/// event callback
bool enableMouseCallback = false;
bool firstMouse = true;
float lastX;
float lastY;

/// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow * window, int width, int height)
{
	// make sure the viewport matches the new window dimensions; note that width and 
	// height will be significantly larger than specified on retina displays.
	if (width == 0 && height == 0)
	{
		return;      // fix bug when minimize windows causing aspect==0 which is the parameter of glm::perspective
	}
	glViewport(0, 0, width, height);
	SRC_WIDTH = width;
	SRC_HEIGHT = height;
}

/// glfw: whenever the mouse moves in the case of mouse button pressed, this callback is called
void mouse_callback(GLFWwindow * window, double xpos, double ypos)
{
	if (enableMouseCallback)
	{
		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

		lastX = xpos;
		lastY = ypos;

		mainCam.processMouseMovement(xoffset, yoffset, false);   // the last para means that no limit of the view rotation
	}
	else
	{
		firstMouse = true;
	}
}
/// glfw: whenever the mouse button pressed, this callback is called
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
		enableMouseCallback = true;
	else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
		enableMouseCallback = false;
}
/// glfw: whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow * window, double xoffset, double yoffset)
{
	mainCam.processMouseScroll(yoffset);
}


/// GLFW windows
class Glfw
{
public:
	GLFWwindow * window;

	/// mainCamera data
	glm::mat4 mainCamProjection;
	glm::mat4 mainCamView;
	glm::vec3 mainCamPos;
	/// light
	glm::vec3 lightPos;
	/// timing
	float deltaTime;
	float lastFrame;
	/// Frame sign
	bool lastFrameSign;
	bool curFrameSign;
	/// controller
	bool blinn;
	bool polygonMode;
	bool switchDisplay;
	bool testMode;
	bool bbcFitMode1;
	bool bbcFitMode2;
	bool bbcFitMode3;

	/// default constructor
	Glfw()
		:mainCamProjection(glm::perspective(glm::radians(mainCam.zoom), (float)SRC_WIDTH / (float)SRC_HEIGHT, 0.1f, 1000.0f)),
		mainCamView(mainCam.getViewMatrix()),
		mainCamPos(mainCam.position),
		lightPos(glm::vec3(10, 15, 10)),
		deltaTime(0.0f),
		lastFrame(0.0f),
		lastFrameSign(false),
		curFrameSign(false),
		blinn(false),
		polygonMode(false),
		switchDisplay(false),
		testMode(false),
		bbcFitMode1(true),
		bbcFitMode2(false),
		bbcFitMode3(false)
	{
	}

	void init()
	{
		// glfw: initialize and configure
		glfwInit();
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
		glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

		// glfw window creation
		window = glfwCreateWindow(SRC_WIDTH, SRC_HEIGHT, "billboard clouds", NULL, NULL);
		glfwSetWindowCenter(window);
		if (window == NULL)
		{
			std::cout << "Failed to create GLFW window" << std::endl;
			glfwTerminate();
			return;
		}
		glfwMakeContextCurrent(window);
		glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
		glfwSetCursorPosCallback(window, mouse_callback);
		glfwSetMouseButtonCallback(window, mouse_button_callback);
		glfwSetScrollCallback(window, scroll_callback);

		// tell GLFW to capture our mouse
		glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, 1);
		//glfwSetInputMode(window, GLFW_STICKY_KEYS, 1);
		//glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

		// glad: load all OpenGL function pointers
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			std::cout << "Failed to initialize GLAD" << std::endl;
			return;
		}
	}

	float getTime()
	{
		return glfwGetTime();
	}

	void updateState()
	{
		// swap buffers
		glfwSwapBuffers(window);

		// camera state update
		mainCamProjection = glm::perspective(glm::radians(mainCam.zoom), (float)SRC_WIDTH / (float)SRC_HEIGHT, 0.1f, 1000.0f);
		mainCamView = mainCam.getViewMatrix();
		mainCamPos = mainCam.position;

		// poll IO events (keys pressed/released, mouse moved etc.)
		glfwPollEvents();
		processInput();

		// per-frame time logic
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		// per-frame sign logic
		lastFrameSign = curFrameSign;
		curFrameSign = false;

		// draw in wireframe
		if (polygonMode)
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		}
		else
		{
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}

		// clear
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glViewport(0, 0, SRC_WIDTH, SRC_HEIGHT);
	}

	bool shouldClose()
	{
		return glfwWindowShouldClose(window);
	}

	void destroy()
	{
		// glfw: terminate, clearing all previously allocated GLFW resources.
		glfwTerminate();
	}

private:
	/// glfw: set the center of the glfw windows to the center of the monitor
	void glfwSetWindowCenter(GLFWwindow* window) {
		// Get window position and size
		int window_x, window_y;
		glfwGetWindowPos(window, &window_x, &window_y);

		int window_width, window_height;
		glfwGetWindowSize(window, &window_width, &window_height);

		// Halve the window size and use it to adjust the window position to the center of the window
		window_width *= 0.5;
		window_height *= 0.5;

		window_x += window_width;
		window_y += window_height;

		// Get the list of monitors
		int monitors_length;
		GLFWmonitor **monitors = glfwGetMonitors(&monitors_length);

		if (monitors == NULL) {
			// Got no monitors back
			return;
		}

		// Figure out which monitor the window is in
		GLFWmonitor *owner = NULL;
		int owner_x, owner_y, owner_width, owner_height;

		for (int i = 0; i < monitors_length; i++) {
			// Get the monitor position
			int monitor_x, monitor_y;
			glfwGetMonitorPos(monitors[i], &monitor_x, &monitor_y);

			// Get the monitor size from its video mode
			int monitor_width, monitor_height;
			GLFWvidmode *monitor_vidmode = (GLFWvidmode*)glfwGetVideoMode(monitors[i]);

			if (monitor_vidmode == NULL) {
				// Video mode is required for width and height, so skip this monitor
				continue;

			}
			else {
				monitor_width = monitor_vidmode->width;
				monitor_height = monitor_vidmode->height;
			}

			// Set the owner to this monitor if the center of the window is within its bounding box
			if ((window_x > monitor_x && window_x < (monitor_x + monitor_width)) && (window_y > monitor_y && window_y < (monitor_y + monitor_height))) {
				owner = monitors[i];

				owner_x = monitor_x;
				owner_y = monitor_y;

				owner_width = monitor_width;
				owner_height = monitor_height;
			}
		}

		if (owner != NULL) {
			// Set the window position to the center of the owner monitor
			glfwSetWindowPos(window, owner_x + (owner_width * 0.5) - window_width, owner_y + (owner_height * 0.5) - window_height);
		}
	}

	/// glfw: process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
	void processInput()
	{
		if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
			glfwSetWindowShouldClose(window, true);

		if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
			mainCam.processKeyboard(FORWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
			mainCam.processKeyboard(BACKWARD, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
			mainCam.processKeyboard(LEFT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
			mainCam.processKeyboard(RIGHT, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
			mainCam.processKeyboard(UP, deltaTime);
		if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
			mainCam.processKeyboard(DOWN, deltaTime);

		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		{
			blinn = false;
			testMode = false;
			bbcFitMode1 = false;
			bbcFitMode2 = false;
			bbcFitMode3 = false;
			polygonMode = false;
		}

		if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		{
			bbcFitMode1 = true;
			bbcFitMode2 = false;
			bbcFitMode3 = false;
		}
		if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		{
			bbcFitMode1 = false;
			bbcFitMode2 = true;
			bbcFitMode3 = false;
		}
		if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		{
			bbcFitMode1 = false;
			bbcFitMode2 = false;
			bbcFitMode3 = true;
		}

		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)
		{
			if (testMode)
			{
				switchDisplay = true;
			}
		}
		if (glfwGetKey(window, GLFW_KEY_C) == GLFW_RELEASE)
		{
			if (testMode)
			{
				switchDisplay = false;
			}
		}
		if (glfwGetKey(window, GLFW_KEY_T) == GLFW_PRESS)
		{
			testMode = true;
		}
		if (glfwGetKey(window, GLFW_KEY_B) == GLFW_PRESS)
		{
			blinn = true;
		}
		if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS)
		{
			polygonMode = true;
		}
	}
};

#endif