#ifndef WINDOW_HH
#define WINDOW_HH

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <string>

struct window {
	GLFWwindow *glfw_window;
	window();
	~window();
};

struct buffer {
	GLuint id;
	buffer(GLenum target, GLsizei size, const void *data);
	~buffer();
};

struct program {
	GLuint id;
	program(const char *vert_path, const char *frag_path);
	~program();
};

#endif

