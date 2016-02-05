#include "gl.hh"
#include "gfx.hh"

atlas::atlas(FT_Face face, int height) {
  faceptr = face;
  font_height = height;
  texture_last_x = texture_last_y = row_height = 0;
  glyphs.resize(1, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });

  add_new_texture();

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}

atlas::~atlas() {
  for (size_t i = 0; i < textures.size(); i++)
    glDeleteTextures(1, &textures[i]);
}

void atlas::add_new_texture() {
  GLuint texture;
  glGenTextures(1, &texture);
  glActiveTexture(GL_TEXTURE0 + texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, MAXWIDTH, MAXWIDTH, 0,
      GL_ALPHA, GL_UNSIGNED_BYTE, 0);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

  textures.push_back(texture);
}

const glyph* atlas::query(unsigned int codepoint) {
  if (codepoint >= glyphs.size())
    glyphs.resize(codepoint+1, { false });
  if (!glyphs[codepoint].rendered)
    render_glyph(codepoint);
  return &glyphs[codepoint];
}

void atlas::render_glyph(unsigned int i) {
  FT_Set_Pixel_Sizes(faceptr, 0, font_height);
  FT_GlyphSlot g = faceptr->glyph;

  if (FT_Load_Char(faceptr, i, FT_LOAD_RENDER))
    die("Rendering character '%c' failed!\n", i);

  if (texture_last_x + g->bitmap.width >= MAXWIDTH) {
    if (texture_last_y + g->bitmap.rows + row_height >= MAXWIDTH) {
      // y overflow
      add_new_texture();
      texture_last_x = 0;
      texture_last_y = 0;
      row_height = 0;
    } else {
      // x overflow
      texture_last_x = 0;
      texture_last_y += row_height;
      row_height = 0;
    }
  }

  glyphs[i].gltexture_rendered_to = textures.back();

  glyphs[i].rendered = true;

  glActiveTexture(GL_TEXTURE0 + textures.back());
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
  texture_last_x += g->bitmap.width;
}

gfx::gfx()
{
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  if (FT_Init_FreeType(&ft))
    die("could not init freetype library");

  if (FT_New_Face(ft, "font.ttf", 0, &face))
    die("could not open font");

  prog = new program("vert.glsl", "frag.glsl");

  attribute_coord = glGetAttribLocation(prog->id, "coord");
  uniform_texture = glGetUniformLocation(prog->id, "texture");
  uniform_color = glGetUniformLocation(prog->id, "color");

  glGenBuffers(1, &vbo);

  a48 = new atlas(face, 48);
}

gfx::~gfx()
{
  delete prog;
  delete a48;
  FT_Done_Face(face);
  FT_Done_FreeType(ft);
  glDeleteBuffers(1, &vbo);
}

void gfx::display() {
  float sx = 2.0 / 1008.0;
  float sy = 2.0 / 567.0;

  glUseProgram(prog->id);

  glClearColor(1, 1, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  const GLfloat black[4] = { 0, 0, 0, 1 };
  glUniform4fv(uniform_color, 1, black);

  render_text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", a48, -1 + 8 * sx, 1 - 50 * sy, sx, sy);
  render_text("abcdefghijklmnopqrstuvwxyz", a48, -1 + 8 * sx, 1 - 150 * sy, sx, sy);

  render_text("THE QUICK BROWN FOX JUMPS", a48, -1 + 8 * sx, 1 - 250 * sy, sx, sy);
  render_text("OVER THE LAZY DOG", a48, -1 + 8 * sx, 1 - 290 * sy, sx, sy);

  render_text("the quick brown fox jumps", a48, -1 + 8 * sx, 1 - 390 * sy, sx, sy);
  render_text("over the lazy dog", a48, -1 + 8 * sx, 1 - 430 * sy, sx, sy);
}

void gfx::render_text(const char *text, atlas *a, float x, float y,
    float sx, float sy) {
  const uint8_t *p;

  /* Loop through all characters */
  for (p = (const uint8_t *)text; *p; p++) {
    const glyph *gi = a->query(*p);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glActiveTexture(GL_TEXTURE0 + gi->gltexture_rendered_to);
    glBindTexture(GL_TEXTURE_2D, gi->gltexture_rendered_to);
    glEnableVertexAttribArray(attribute_coord);
    glUniform1i(uniform_texture, gi->gltexture_rendered_to);
    glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

    /* Calculate the vertex and texture coordinates */
    float x2 = x + gi->bitmap_left * sx;
    float y2 = -y - gi->bitmap_top * sy;
    float w = gi->bitmap_w * sx;
    float h = gi->bitmap_h * sy;

    /* Advance the cursor to the start of the next character */
    x += gi->advance_x * sx;
    y += gi->advance_y * sy;

    /* Skip glyphs that have no pixels */
    if (!w || !h)
      continue;

    struct {
      GLfloat x;
      GLfloat y;
      GLfloat s;
      GLfloat t;
    } coords[6];

    coords[0] = {
      x2, -y2, gi->texture_offset_x, gi->texture_offset_y};
    coords[1] = {
      x2 + w, -y2, gi->texture_offset_x + gi->bitmap_w / (float)MAXWIDTH, gi->texture_offset_y};
    coords[2] = {
      x2, -y2 - h, gi->texture_offset_x, gi->texture_offset_y + gi->bitmap_h / (float)MAXWIDTH};
    coords[3] = {
      x2 + w, -y2, gi->texture_offset_x + gi->bitmap_w / (float)MAXWIDTH, gi->texture_offset_y};
    coords[4] = {
      x2, -y2 - h, gi->texture_offset_x, gi->texture_offset_y + gi->bitmap_h / (float)MAXWIDTH};
    coords[5] = {
      x2 + w, -y2 - h, gi->texture_offset_x + gi->bitmap_w / (float)MAXWIDTH, gi->texture_offset_y + gi->bitmap_h / (float)MAXWIDTH};

    glBufferData(GL_ARRAY_BUFFER, sizeof(coords), coords, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(attribute_coord);
  }
}

void render_cell::set_pos_and_model_mat(ull x, ull y, ui charsize_x, ui charsize_y)
{
  ui screen_pos_x = x * charsize_x, screen_pos_y = y * charsize_y;
}

render_matrix::set_matrices_sizes(const matrix &mat)
{
  cells.set_size(mat.sx, mat.sy);
  needs_redrawing.set_size(mat.sx, mat.sy);
  for (ull y = 0; y < mat.sy; y++)
    for (ull x = 0; x < mat.sx; x++) {
      cells.set_pos_and_model_mat(x, y, mat.charsize_x, mat.charsize_y);
      needs_redrawing = { 1, 1, 1 };
    }
}

void render_matrix::update()
{
  for (ull y = 0; y < mat.sy; y++)
    for (ull x = 0; x < mat.sx; x++) {
}

void render_matrix::copy_update_and_draw(const matrix& mat)
{
  for (ull y = 0; y < mat.sy; y++)
    for (ull x = 0; x < mat.sx; x++) {
      if (cells[y][x].contained_cell.background_color != mat[y][x].background_color) {
        needs_redrawing[y][x].background_color = 1;
        cells[y][x].contained_cell.background_color = mat[y][x].background_color;
      }
      if (cells[y][x].contained_cell.foreground_color != mat[y][x].foreground_color) {
        needs_redrawing[y][x].foreground_color = 1;
        cells[y][x].contained_cell.foreground_color = mat[y][x].foreground_color;
      }
      if (cells[y][x].contained_cell.character != mat[y][x].character) {
        needs_redrawing[y][x].character = 1;
        cells[y][x].contained_cell.character != mat[y][x].character;
      }
    }
  draw();
}

render_matrix::render_matrix(const matrix& mat)
{
  set_matrices_sizes(mat);
}

// vim: et:sw=2

