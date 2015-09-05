#include "window.hh"
#include "utils.hh"
#include <cstdlib>
#include <stdexcept>

window::window()
{
	if (!glfwInit())
		die("Failed to initialze GLFW");

	glfwWindowHint(GLFW_RESIZABLE, 0);

	glfw_window = glfwCreateWindow(800, 600, "livo", NULL, NULL);
	if (!glfw_window)
		die("Failed to open window");

	glfwMakeContextCurrent(glfw_window);

	GLenum err = glewInit();
	if (err != GLEW_OK)
		die("Failed to initialze GLEW");
}

window::~window() {
	glfwTerminate();
}

buffer::buffer(GLenum target, GLsizei size, const void *data) {
	glGenBuffers(1, &id);
	glBindBuffer(target, id);
	glBufferData(target, size, data, GL_STATIC_DRAW);
	glBindBuffer(target, 0);
}

buffer::~buffer() {
	glDeleteBuffers(1, &id);
}

GLuint create_shader(GLenum type, const char *path) {
	FILE *file = fopen(path, "rb");
	if (!file) {
		std::string msg("no such file: \"");
		msg += std::string(path);
		msg += "\"";
		die(msg);
	}
	fseek(file, 0, SEEK_END);
	const size_t length = ftell(file);
	rewind(file);
	char *source = (char*)malloc((length + 1) * sizeof(char));
	fread(source, 1, length, file);
	source[length] = '\0';
	fclose(file);

	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);
	GLint status;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		char *info = (char*)malloc(length * sizeof(char));
		glGetShaderInfoLog(shader, length, NULL, info);
		char buf[4096];
		snprintf(buf, 4096, "%s shader compiling failed:\n%s",
				type == GL_VERTEX_SHADER ? "vertex" : "fragment",
				info);
		free(info);
		free(source);
		die(std::string(buf));
	}
	free(source);

	return shader;
}

program::program(const char *vert_path, const char *frag_path) {
	id = glCreateProgram();
	GLuint shader1 = create_shader(GL_VERTEX_SHADER, vert_path);
	GLuint shader2 = create_shader(GL_FRAGMENT_SHADER, frag_path);
	glAttachShader(id, shader1);
	glAttachShader(id, shader2);
	glLinkProgram(id);
	GLint status;
	glGetProgramiv(id, GL_LINK_STATUS, &status);
	if (status == GL_FALSE) {
		GLint length;
		glGetProgramiv(id, GL_INFO_LOG_LENGTH, &length);
		GLchar *info = (char*)malloc(length * sizeof(char));
		glGetProgramInfoLog(id, length, NULL, info);
		char buf[4096];
		snprintf(buf, 4096, "program linking failed:\n%s",
				info);
		free(info);
		die(std::string(buf));
	}
	glDetachShader(id, shader1);
	glDetachShader(id, shader2);
	glDeleteShader(shader1);
	glDeleteShader(shader2);
}

program::~program()
{
	glDeleteProgram(id);
}

