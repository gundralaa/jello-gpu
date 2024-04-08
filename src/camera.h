#ifndef CAMERA_H
#define CAMERA_H

#include <glm/glm.hpp>
#include "shader.h"
#include "window.h"

class camera {
public:
	camera(window* win, glm::vec3* target);
	void update();
	void set_view(shader* p);
private:
	glm::vec3 pos;
	glm::vec3 front;
	glm::vec3 up;
	glm::mat4 view;
	window* win_ptr;
};

#endif // !CAMERA_H
