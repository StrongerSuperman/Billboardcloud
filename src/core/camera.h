#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

/// Defines several possible options for camera movement. Used as abstraction to stay away from window-system specific input methods
enum Camera_Movement {
	FORWARD,
	BACKWARD,
	LEFT,
	RIGHT,
	UP,
	DOWN
};

/// Default camera values
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float SPEED = 10.0f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 90.0f;
const float ZOOMSPEED = 5.0f;
const float ZOOMMIN = 1.0f;
const float ZOOMMAX = 90.0f;


/// An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
class Camera
{
public:
	// Camera Attributes
	glm::vec3 position;
	glm::vec3 front;
	glm::vec3 up;
	glm::vec3 right;
	glm::vec3 worldUp;
	// Euler Angles
	float yaw;
	float pitch;
	// Camera options
	float movementSpeed;
	float mouseSensitivity;
	float zoom;

	/// Constructor with vectors
	Camera(glm::vec3 _position = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 _Front = glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3 _up = glm::vec3(0.0f, 1.0f, 0.0f), float _yaw = YAW, float _pitch = PITCH)
		: position(_position),
		front(_Front),
		worldUp(_up),
		yaw(_yaw),
		pitch(_pitch),
		movementSpeed(SPEED),
		mouseSensitivity(SENSITIVITY),
		zoom(ZOOM)
	{
		updateCameraVectors();
	}
	/// Constructor with scalar values
	Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float _yaw, float _pitch)
		: yaw(_yaw),
		pitch(_pitch),
		front(glm::vec3(0.0f, 0.0f, -1.0f)),
		movementSpeed(SPEED),
		mouseSensitivity(SENSITIVITY),
		zoom(ZOOM)
	{
		position = glm::vec3(posX, posY, posZ);
		worldUp = glm::vec3(upX, upY, upZ);
		updateCameraVectors();
	}

	/// Returns the view matrix calculated using Euler Angles and the LookAt Matrix
	glm::mat4 getViewMatrix()
	{
		return glm::lookAt(position, position + front, up);
	}

	/// Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
	void processKeyboard(Camera_Movement direction, float deltaTime)
	{
		float velocity = movementSpeed * deltaTime;
		if (direction == FORWARD)
			position += front * velocity;
		if (direction == BACKWARD)
			position -= front * velocity;
		if (direction == LEFT)
			position -= right * velocity;
		if (direction == RIGHT)
			position += right * velocity;
		if (direction == UP)
			position += up * velocity;
		if (direction == DOWN)
			position -= up * velocity;
	}

	/// Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
	void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true)
	{
		xoffset *= mouseSensitivity;
		yoffset *= mouseSensitivity;

		yaw += xoffset;
		pitch += yoffset;

		// Make sure that when pitch is out of bounds, screen doesn't get flipped
		if (constrainPitch)
		{
			if (pitch > 89.0f)
				pitch = 89.0f;
			if (pitch < -89.0f)
				pitch = -89.0f;
		}

		// Update Front, Right and Up Vectors using the updated Euler angles
		updateCameraVectors();
	}

	/// Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void processMouseScroll(float yoffset)
	{
		if (zoom >= ZOOMMIN && zoom <= ZOOMMAX)
			zoom -= yoffset * ZOOMSPEED;
		if (zoom <= ZOOMMIN)
			zoom = ZOOMMIN;
		if (zoom >= ZOOMMAX)
			zoom = ZOOMMAX;
	}

private:
	/// Calculates the front vector from the Camera's (updated) Euler Angles
	void updateCameraVectors()
	{
		// Calculate the new Front vector
		glm::vec3 _front;
		_front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		_front.y = sin(glm::radians(pitch));
		_front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		front = glm::normalize(_front);
		// Also re-calculate the Right and Up vector
		right = glm::normalize(glm::cross(front, worldUp));
		up = glm::normalize(glm::cross(right, front));
	}
};
#endif