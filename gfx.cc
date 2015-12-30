#include "gl.hh"
#include "gfx.hh"

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

  glUniform1i(uniform_texture, 0);

  glGenBuffers(1, &vbo);

  a48 = new atlas(face, 48);
}

gfx::~gfx()
{
  delete prog;
  delete a48;
  FT_Done_Face(face);
  FT_Done_FreeType(ft);
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

  render_text("The Quick Brown Fox Jumps", a48, -1 + 8 * sx, 1 - 250 * sy, sx, sy);
  render_text("Over The Lazy Dog", a48, -1 + 8 * sx, 1 - 290 * sy, sx, sy);
}

void gfx::render_text(const char *text, atlas *a, float x, float y,
    float sx, float sy) {
  const uint8_t *p;

  /* Loop through all characters */
  for (p = (const uint8_t *)text; *p; p++) {
    const glyph *gi = a->query(*p);

    glEnableVertexAttribArray(attribute_coord);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glBindTexture(GL_TEXTURE_2D, gi->texture);

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

    glBufferData(GL_ARRAY_BUFFER, sizeof coords, coords, GL_DYNAMIC_DRAW);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(attribute_coord);
  }
}

#if 0
void gfx::draw(const matrix &mat)
{
  for (ull y = 0; y < mat.sy; y++)
    for (ull x = 0; x < mat.sx; x++) {
      screen_x = (x+1) * mat.
    }
}
#endif

// vim: et:sw=2

