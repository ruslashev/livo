#ifndef GL_HH
#define GL_HH

#include <GL/glew.h>
#include <GLFW/glfw3.h>

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
  GLint bind_attribute(const char *name);
};

#endif

// vim: et:sw=2

