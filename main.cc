#include "window.hh"
#include "utils.hh"
#include <fstream>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <algorithm>

#include <GL/glew.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <freetype2/ft2build.h>
#include FT_FREETYPE_H

program *prog = nullptr;
GLint attribute_coord;
GLint uniform_texture;
GLint uniform_color;

struct point {
	GLfloat x;
	GLfloat y;
	GLfloat s;
	GLfloat t;
};

GLuint vbo;

FT_Library ft;
FT_Face face;

#define MAXWIDTH 128

struct glyph_info {
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

struct atlas {
	std::vector<GLuint> textures;

	unsigned int font_height;

	int texture_last_x;
	int texture_last_y;
	unsigned int row_height;

	std::vector<glyph_info> glyphs;

	atlas(FT_Face face, int height) {
		font_height = height;
		puts("INIT glyphs 1");
		glyphs.resize(1, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 });

		add_new_texture();

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		query('\0');
	}

	const glyph_info* query(unsigned int codepoint) {
		// printf("query glyph %c\n", codepoint);
		if (codepoint >= glyphs.size()) {
			glyphs.resize(codepoint+1);
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

		puts("textures.push_back(texture)");
		textures.push_back(texture);

		puts("coords reset");
		texture_last_x = 0;
		texture_last_y = 0;
		row_height = 0;
	}

	void render_char(unsigned int i) {
		printf("render char %c\n", i);
		FT_Set_Pixel_Sizes(face, 0, font_height);
		FT_GlyphSlot g = face->glyph;

		if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			fprintf(stderr, "Loading character '%c' failed!\n", i);
			return;
		}

		if (texture_last_x + g->bitmap.width + 1 >= MAXWIDTH) {
			puts("x overflow");
			if (texture_last_y + row_height >= MAXWIDTH) {
				puts("y overflow");
				puts("creating new texture");
				add_new_texture();
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

atlas *a48;
atlas *a24;
atlas *a12;

void init_resources() {
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
	a24 = new atlas(face, 24);
	a12 = new atlas(face, 12);
}

void render_text(const char *text, atlas *a, float x, float y, float sx, float sy) {
	const uint8_t *p;

	/* Set up the VBO for our vertex data */
	glEnableVertexAttribArray(attribute_coord);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

	point coords[6 * strlen(text)];
	int c = 0;

	/* Loop through all characters */
	for (p = (const uint8_t *)text; *p; p++) {
		const glyph_info *gi = a->query(*p);

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

		coords[c++] = (point) {
			x2, -y2, gi->texture_offset_x, gi->texture_offset_y};
		coords[c++] = (point) {
			x2 + w, -y2, gi->texture_offset_x + gi->bitmap_w / (float)MAXWIDTH, gi->texture_offset_y};
		coords[c++] = (point) {
			x2, -y2 - h, gi->texture_offset_x, gi->texture_offset_y + gi->bitmap_h / (float)MAXWIDTH};
		coords[c++] = (point) {
			x2 + w, -y2, gi->texture_offset_x + gi->bitmap_w / (float)MAXWIDTH, gi->texture_offset_y};
		coords[c++] = (point) {
			x2, -y2 - h, gi->texture_offset_x, gi->texture_offset_y + gi->bitmap_h / (float)MAXWIDTH};
		coords[c++] = (point) {
			x2 + w, -y2 - h, gi->texture_offset_x + gi->bitmap_w / (float)MAXWIDTH, gi->texture_offset_y + gi->bitmap_h / (float)MAXWIDTH};
	}

	/* Draw all the character on the screen in one go */
	glBufferData(GL_ARRAY_BUFFER, sizeof coords, coords, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_TRIANGLES, 0, c);

	glDisableVertexAttribArray(attribute_coord);
}

void display() {
	float sx = 2.0 / 800.0;
	float sy = 2.0 / 600.0;

	glUseProgram(prog->id);

	/* White background */
	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	GLfloat black[4] = { 0, 0, 0, 1 };
	glUniform4fv(uniform_color, 1, black);

	render_text("ABCDEFGHIJKLMNOPQRSTUVWXYZ", a48, -1 + 8 * sx, 1 - 50 * sy, sx, sy);
	render_text("abcdefghijklmnopqrstuvwxyz", a48, -1 + 8 * sx, 1 - 150 * sy, sx, sy);

	render_text("The Quick Brown Fox Jumps", a48, -1 + 8 * sx, 1 - 250 * sy, sx, sy);
	render_text("Over The Lazy Dog", a48, -1 + 8 * sx, 1 - 290 * sy, sx, sy);
}

int main(int argc, char **argv) {
	try {
		window window;

		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		init_resources();

		while (!glfwWindowShouldClose(window.glfw_window)) {
			display();
			glfwSwapBuffers(window.glfw_window);
			glfwPollEvents();
		}
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

