#pragma once

#include <glm\vec3.hpp>
#include <glm\mat4x4.hpp>

class Camera_orbit
{
public:
	Camera_orbit(float radius);

	void compute_matrix();
	glm::mat4 get_world_view_matrix() { return world_view_matrix; }
	glm::vec3 get_position() { return position; }

	void on_mousewheel_scroll_callback(double yoffset);
	void on_mouse_move_callback(double x, double y);

private:
	glm::vec3 position = glm::vec3(0, 0, 0);
	glm::vec3 look_at_position = glm::vec3(0, 0, 0);

	float horizontalAngle = 0.0f;
	float verticalAngle = 0.0f;

	float speed = 10.0f; // units / second
	float mouseSpeed = 0.0005f;
	float radius = 10.0f;

	glm::mat4 world_view_matrix;

	bool mouse_pressed;
	double cx;
	double cy;
};