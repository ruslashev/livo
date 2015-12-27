#include "gfx.hh"
#include "utils.hh"
#include <cstdlib>
#include <stdexcept>

window::window()
{
  if (!glfwInit())
    die("Failed to initialze GLFW");

  glfwWindowHint(GLFW_RESIZABLE, 0);

  glfw_window = glfwCreateWindow(1008, 567, "livo", NULL, NULL);
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

static GLuint create_shader(GLenum type, const char *path) {
  FILE *file = fopen(path, "rb");
  if (!file)
    die("no such file: \"%s\"", path);
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
    die(buf);
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
    die(buf);
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
}

void gfx::display() {
  float sx = 2.0 / 800.0;
  float sy = 2.0 / 600.0;

  glUseProgram(prog->id);

  glClearColor(1, 1, 1, 1);
  glClear(GL_COLOR_BUFFER_BIT);

  GLfloat black[4] = { 0, 0, 0, 1 };
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

// void draw(const matrix &mat)
// {
// }

// vim: et:sw=2

