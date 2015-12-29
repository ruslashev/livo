#include "gl.hh"
#include "gfx.hh"
#include "utils.hh"
#include "matrix.hh"
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

void actual_main();
int main(int argc, char **argv)
{
  try {
    actual_main();
  } catch (std::bad_alloc &e) {
    fputs("out of memory\n", stderr);
    return 1;
  } catch (std::runtime_error &e) {
    fputs("die: ", stderr);
    fputs(e.what(), stderr);
    fputs("\n", stderr);
    return 1;
  } catch (std::exception &e) {
    fputs("uncaught exception: ", stderr);
    return 1;
  }
  return 0;
}

void actual_main()
{
  window window;
  gfx gfx;
  matrix mat;

  mat.set_size(10, 5);
  for (ull y = 0; y < mat.sy; y++) {
    for (ull x = 0; x < mat.sx; x++)
      printf("%c ", mat.data[y][x].character);
    printf("\n");
  }

  while (!glfwWindowShouldClose(window.glfw_window)) {
    glfwPollEvents();
    gfx.display();
    glfwSwapBuffers(window.glfw_window);
  }
}

// vim: et:sw=2

