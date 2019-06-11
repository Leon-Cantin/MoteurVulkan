#include "Camera_orbit.h"
// Include GLFW
//#include <GLFW/glfw3.h>
//extern GLFWwindow* g_window; // The "extern" keyword here is to access the variable "window" declared in Framework.cpp. This is a hack to keep the tutorials simple. Please avoid this.

#include <glm/gtc/matrix_transform.hpp>


Camera_orbit::Camera_orbit(float radius)
	:radius(radius)
{

}

void Camera_orbit::on_mousewheel_scroll_callback(double yoffset)
{
	radius += static_cast<float>(-yoffset);
}

void Camera_orbit::on_mouse_move_callback(double x, double y)
{
	/*// Get mouse position
	if (glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS && !mouse_pressed) {
		glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		glfwGetCursorPos(g_window, &cx, &cy);
		//glfwSetCursorPos(window, cx, cy);
		mouse_pressed = true;
	}
	else if (glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_1) == GLFW_RELEASE && mouse_pressed) {
		glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		mouse_pressed = false;
	}

	if (mouse_pressed)
	{
		glfwSetCursorPos(g_window, cx, cy);
		// Compute new orientation
		horizontalAngle -= mouseSpeed * float(cx - x);
		verticalAngle -= mouseSpeed * float(cy - y);
		verticalAngle = glm::clamp(verticalAngle, -glm::pi<float>() / 2.0f, glm::pi<float>() / 2.0f);
	}*/
}

void Camera_orbit::compute_matrix() {

	/*// glfwGetTime is called only once, the first time this function is called
	static double lastTime = glfwGetTime();

	// Compute time difference between current and last frame
	double currentTime = glfwGetTime();
	float deltaTime = float(currentTime - lastTime);

	// Direction : Spherical coordinates to Cartesian coordinates conversion
	glm::vec3 location(
		radius * -sin(horizontalAngle),
		radius * sin(verticalAngle) + look_at_position.y,
		radius * -cos(horizontalAngle)
	);

	position = location;

	world_view_matrix = glm::lookAt(location, look_at_position, glm::vec3(0, 1, 0));

	// For the next frame, the "last time" will be "now"
	lastTime = currentTime;*/
};
