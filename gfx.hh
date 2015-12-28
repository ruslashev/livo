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

#define MAXWIDTH 128

struct atlas {
  FT_Face faceptr;
  std::vector<GLuint> textures;

  unsigned int font_height;

  int texture_last_x;
  int texture_last_y;
  unsigned int row_height;

  std::vector<glyph> glyphs;

  atlas(FT_Face face, int height) {
    faceptr = face;
    font_height = height;
    glyphs.resize(1, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });

    add_new_texture();

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    query('\0');
  }

  const glyph* query(unsigned int codepoint) {
    // printf("query glyph %c\n", codepoint);
    if (codepoint >= glyphs.size()) {
      glyphs.resize(codepoint+1, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });
      // for (size_t i = 0; i < glyphs.size(); i++)
      // 	glyphs
    }
    if (!glyphs[codepoint].rendered)
      render_char(codepoint);
    return &glyphs[codepoint];
  }

  void add_new_texture() {
    puts("add_new_texture()");
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, MAXWIDTH, MAXWIDTH, 0,
        GL_ALPHA, GL_UNSIGNED_BYTE, 0);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    textures.push_back(texture);

    texture_last_x = 0;
    texture_last_y = 0;
    row_height = 0;
  }

  void render_char(unsigned int i) {
    FT_Set_Pixel_Sizes(faceptr, 0, font_height);
    FT_GlyphSlot g = faceptr->glyph;

    if (FT_Load_Char(faceptr, i, FT_LOAD_RENDER)) {
      fprintf(stderr, "Loading character '%c' failed!\n", i);
      return;
    }

    if (texture_last_x + g->bitmap.width + 1 >= MAXWIDTH) {
      if (texture_last_y + row_height >= MAXWIDTH) {
        add_new_texture();
        row_height = 0;
      } else {
        texture_last_y += row_height;
        row_height = 0;
        texture_last_x = 0;
      }
    }

    printf("texture %d access\n", textures.back());
    glyphs[i].texture = textures.back();

    glyphs[i].rendered = true;

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textures.back());

    glTexSubImage2D(GL_TEXTURE_2D, 0, texture_last_x, texture_last_y,
        g->bitmap.width, g->bitmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE,
        g->bitmap.buffer);

    glyphs[i].advance_x = g->advance.x >> 6;
    glyphs[i].advance_y = g->advance.y >> 6;

    glyphs[i].bitmap_w = g->bitmap.width;
    glyphs[i].bitmap_h = g->bitmap.rows;

    glyphs[i].bitmap_left = g->bitmap_left;
    glyphs[i].bitmap_top = g->bitmap_top;

    glyphs[i].texture_offset_x = texture_last_x / (float)MAXWIDTH;
    glyphs[i].texture_offset_y = texture_last_y / (float)MAXWIDTH;

    row_height = std::max(row_height, g->bitmap.rows);
    texture_last_x += g->bitmap.width + 1;
  }

  ~atlas() {
    for (size_t i = 0; i < textures.size(); i++)
      glDeleteTextures(1, &textures[i]);
  }
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

