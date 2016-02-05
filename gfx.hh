#ifndef GFX_HH
#define GFX_HH

#include "matrix.hh"

#include <GL/glew.h>
#include <freetype2/ft2build.h>
#include FT_FREETYPE_H
#include <GLFW/glfw3.h>
#include <vector>

struct glyph {
  bool rendered;
  GLuint gltexture_rendered_to;
  float advance_x;
  float advance_y;
  unsigned int bitmap_w;
  unsigned int bitmap_h;
  float bitmap_left;
  float bitmap_top;
  float texture_offset_x;
  float texture_offset_y;
};

#define MAXWIDTH 128

struct atlas {
  FT_Face faceptr;
  std::vector<GLuint> textures;

  unsigned int font_height;

  int texture_last_x;
  int texture_last_y;
  unsigned int row_height;

  std::vector<glyph> glyphs; // todo: replace with map<codepoint,glyph>

  atlas(FT_Face face, int height);
  ~atlas();
  void add_new_texture();
  const glyph* query(unsigned int codepoint);
  void render_glyph(unsigned int i);
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
  void render_text(const char *text, atlas *a, float x, float y,
      float sx, float sy);
public:
  gfx();
  ~gfx();
  void display();
  void draw(const matrix &mat);
};

struct render_cell {
  GLuint background_vbo;
  cell contained_cell;
  glm::mat4 model_mat;
  void set_pos_and_model_mat();
};

class render {
  generic_mat<render_cell> needs_redrawing;
  generic_mat<bool> needs_redrawing;
public:

#endif

// vim: et:sw=2

