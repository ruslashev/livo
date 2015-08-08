#include "window.hh"
#include <fstream>

int main()
{
	window window;

	GLuint text_atlas;
	program program("vertex.glsl", "fragment.glsl");
	GLuint texture_uniform = glGetAttribLocation(program.id, "text_atlas");

	while (!glfwWindowShouldClose(window.glfw_window)) {

	}

	glDeleteTextures(1, &text_atlas);

	return 0;
}

