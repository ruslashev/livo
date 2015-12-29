#ifndef GFX_HH
#define GFX_HH

#include "matrix.hh"

#include <GL/glew.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <GLFW/glfw3.h>
#include <vector>

struct glyph {
  GLuint texture;
  bool rendered;

  float advance_x;
  float advance_y;

  unsigned int bitmap_w;
  unsigned int bitmap_h;

  float bitmap_left;
  float bitmap_top;

  float texture_offset_x;
  float texture_offset_y;
};

#define MAXWIDTH 1024

struct atlas {
  FT_Face faceptr;
  std::vector<GLuint> textures;

  unsigned int font_height;

  int texture_last_x;
  int texture_last_y;
  unsigned int row_height;

  std::vector<glyph> glyphs;

  atlas(FT_Face face, int height);
  ~atlas();
  void add_new_texture();
  const glyph* query(unsigned int codepoint);
  void render_char(unsigned int i);
};

class gfx {
  program *prog;
  atlas *a48;
  GLint attribute_coord;
  GLint uniform_texture;
  GLint uniform_color;
  GLuint vbo;
  FT_Library ft;
  FT_Face face;
  // std::map<GLuint,std::vector<std::pair<ull,ull>>> texture_to_cells;
  void render_text(const char *text, atlas *a, float x, float y,
      float sx, float sy);
public:
  gfx();
  ~gfx();
  void display();
  void draw(const matrix &mat);
};

#endif

// vim: et:sw=2

