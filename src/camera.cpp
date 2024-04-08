#include <includes.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>

#include "camera.h"
#include "shader.h"
#include "window.h"

camera::camera(window* win, glm::vec3* target) {
	win_ptr = win;
	// pos + offset
	glm::vec3 offset(0.0f, 5.0f, 3.0f);
	pos = *target + offset;
	// front
	float yaw_angle = -35.0f;
	glm::vec3 direction(0.0f, sin(glm::radians(yaw_angle)), -1.0f * cos(glm::radians(yaw_angle)));
	front = glm::normalize(direction);
	// up
	up = glm::vec3(0.0f, 1.0f, 0.0f);
	view = glm::lookAt(pos, pos + front, up);
}
void camera::update() {
	const float camera_speed = 2.5f * win_ptr->delta_time;
	// move dir
	glm::vec3 move_vec(0.0f, 0.0f, -1.0f);
	if (glfwGetKey(win_ptr->ptr, GLFW_KEY_W) == GLFW_PRESS)
		pos +=  move_vec * camera_speed;
	if (glfwGetKey(win_ptr->ptr, GLFW_KEY_S) == GLFW_PRESS)
		pos -= move_vec * camera_speed;
	if (glfwGetKey(win_ptr->ptr, GLFW_KEY_A) == GLFW_PRESS)
		pos -= glm::normalize(glm::cross(move_vec, up)) * camera_speed;
	if (glfwGetKey(win_ptr->ptr, GLFW_KEY_D) == GLFW_PRESS)
		pos += glm::normalize(glm::cross(move_vec, up)) * camera_speed;
}

void camera::set_view(shader* p) {
	// look at update
	view = glm::lookAt(pos, pos + front, up);
	int view_loc = glGetUniformLocation(p->p_id, "view");
	glUniformMatrix4fv(view_loc, 1, GL_FALSE, glm::value_ptr(view));
}
