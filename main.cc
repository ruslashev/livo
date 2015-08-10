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

// Maximum texture width
#define MAXWIDTH 1024

/**
 * The atlas struct holds a texture that contains the visible US-ASCII characters
 * of a certain font rendered with a certain character height.
 * It also contains an array that contains all the information necessary to
 * generate the appropriate vertex and texture coordinates for each character.
 *
 * After the constructor is run, you don't need to use any FreeType functions anymore.
 */

struct glyph_info {
	bool rendered;

	float advance_x;
	float advance_y;

	float bitmap_w;
	float bitmap_h;

	float bitmap_left;
	float bitmap_top;

	float texture_offset_x;
	float texture_offset_y;
};

struct atlas {
	GLuint texture;

	unsigned int font_height;

	unsigned int texture_width;
	unsigned int texture_height;

	int texture_last_x;
	int texture_last_y;
	unsigned int rowh;

	std::vector<glyph_info> glyphs;

	atlas(FT_Face face, int height) {
		texture = -1;
		font_height = height;
		texture_last_x = 0;
		texture_last_y = 0;
		rowh = 0;
		glyphs.resize(128, { 0, 0, 0, 0, 0, 0, 0, 0, 0 });

		create_texture();

		// required 1 byte alignment
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		for (int i = 0; i < 32; i++)
			render_char(i); // precache ASCII set
	}

	void create_texture() {
		FT_Set_Pixel_Sizes(face, 0, font_height);
		FT_GlyphSlot g = face->glyph;

		texture_width = texture_height = 0;
		unsigned int roww = 0;
		unsigned int rowh = 0;

		for (int i = 0; i < glyphs.size(); i++) {
			if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
				fprintf(stderr, "Loading character %c failed!\n", i);
				continue;
			}
			if (roww + g->bitmap.width + 1 >= MAXWIDTH) {
				texture_width = std::max(texture_width, roww);
				texture_height += rowh;
				roww = 0;
				rowh = 0;
			}
			roww += g->bitmap.width + 1;
			rowh = std::max(rowh, g->bitmap.rows);
		}

		texture_width = std::max(texture_width, roww);
		texture_height += rowh;

		if (texture != -1)
			glDeleteTextures(1, &texture);
		glActiveTexture(GL_TEXTURE0);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		glUniform1i(uniform_texture, 0);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, texture_width, texture_height,
				0, GL_ALPHA, GL_UNSIGNED_BYTE, 0);
	}

	void render_char(unsigned int i) {
		FT_Set_Pixel_Sizes(face, 0, font_height);
		FT_GlyphSlot g = face->glyph;

		// todo someday it's gonna crash

		if (FT_Load_Char(face, i, FT_LOAD_RENDER)) {
			fprintf(stderr, "Loading character '%c' failed!\n", i);
			return;
		}

		if (texture_last_x + g->bitmap.width + 1 >= MAXWIDTH) {
			texture_last_y += rowh;
			rowh = 0;
			texture_last_x = 0;
		}

		glyphs[i].rendered = true;

		glTexSubImage2D(GL_TEXTURE_2D, 0, texture_last_x, texture_last_y, g->bitmap.width, g->bitmap.rows, GL_ALPHA, GL_UNSIGNED_BYTE, g->bitmap.buffer);
		glyphs[i].advance_x = g->advance.x >> 6;
		glyphs[i].advance_y = g->advance.y >> 6;

		glyphs[i].bitmap_w = g->bitmap.width;
		glyphs[i].bitmap_h = g->bitmap.rows;

		glyphs[i].bitmap_left = g->bitmap_left;
		glyphs[i].bitmap_top = g->bitmap_top;

		glyphs[i].texture_offset_x = texture_last_x / (float)texture_width;
		glyphs[i].texture_offset_y = texture_last_y / (float)texture_height;

		rowh = std::max(rowh, g->bitmap.rows);
		texture_last_x += g->bitmap.width + 1;
	}

	~atlas() {
		glDeleteTextures(1, &texture);
	}
};

atlas *a48;
atlas *a24;
atlas *a12;

int init_resources() {
	if (FT_Init_FreeType(&ft))
		die("could not init freetype library");

	if (FT_New_Face(ft, "font.ttf", 0, &face))
		die("could not open font");

	prog = new program("vert.glsl", "frag.glsl");

	attribute_coord = glGetAttribLocation(prog->id, "coord");
	uniform_texture = glGetUniformLocation(prog->id, "texture");
	uniform_color = glGetUniformLocation(prog->id, "color");

	// Create the vertex buffer object
	glGenBuffers(1, &vbo);

	/* Create texture atlasses for several font sizes */
	a48 = new atlas(face, 48);
	a24 = new atlas(face, 24);
	a12 = new atlas(face, 12);

	return 1;
}

/**
 * Render text using the currently loaded font and currently set font size.
 * Rendering starts at coordinates (x, y), z is always 0.
 * The pixel coordinates that the FreeType2 library uses are scaled by (sx, sy).
 */
void render_text(const char *text, atlas *a, float x, float y, float sx, float sy) {
	const uint8_t *p;

	/* Use the texture containing the atlas */
	glBindTexture(GL_TEXTURE_2D, a->texture);
	glUniform1i(uniform_texture, 0);

	/* Set up the VBO for our vertex data */
	glEnableVertexAttribArray(attribute_coord);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glVertexAttribPointer(attribute_coord, 4, GL_FLOAT, GL_FALSE, 0, 0);

	point coords[6 * strlen(text)];
	int c = 0;

	/* Loop through all characters */
	for (p = (const uint8_t *)text; *p; p++) {
		/* Calculate the vertex and texture coordinates */
		if (!a->glyphs[*p].rendered)
			a->render_char(*p);
		float x2 = x + a->glyphs[*p].bitmap_left * sx;
		float y2 = -y - a->glyphs[*p].bitmap_top * sy;
		float w = a->glyphs[*p].bitmap_w * sx;
		float h = a->glyphs[*p].bitmap_h * sy;

		/* Advance the cursor to the start of the next character */
		x += a->glyphs[*p].advance_x * sx;
		y += a->glyphs[*p].advance_y * sy;

		/* Skip glyphs that have no pixels */
		if (!w || !h)
			continue;

		coords[c++] = (point) {
			x2, -y2, a->glyphs[*p].texture_offset_x, a->glyphs[*p].texture_offset_y};
		coords[c++] = (point) {
			x2 + w, -y2, a->glyphs[*p].texture_offset_x + a->glyphs[*p].bitmap_w / a->texture_width, a->glyphs[*p].texture_offset_y};
		coords[c++] = (point) {
			x2, -y2 - h, a->glyphs[*p].texture_offset_x, a->glyphs[*p].texture_offset_y + a->glyphs[*p].bitmap_h / a->texture_height};
		coords[c++] = (point) {
			x2 + w, -y2, a->glyphs[*p].texture_offset_x + a->glyphs[*p].bitmap_w / a->texture_width, a->glyphs[*p].texture_offset_y};
		coords[c++] = (point) {
			x2, -y2 - h, a->glyphs[*p].texture_offset_x, a->glyphs[*p].texture_offset_y + a->glyphs[*p].bitmap_h / a->texture_height};
		coords[c++] = (point) {
			x2 + w, -y2 - h, a->glyphs[*p].texture_offset_x + a->glyphs[*p].bitmap_w / a->texture_width, a->glyphs[*p].texture_offset_y + a->glyphs[*p].bitmap_h / a->texture_height};
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
	GLfloat red[4] = { 1, 0, 0, 1 };
	GLfloat transparent_green[4] = { 0, 1, 0, 0.5 };

	/* Set color to black */
	glUniform4fv(uniform_color, 1, black);

	/* Effects of alignment */
	render_text("The Quick Brown Fox Jumps Over The Lazy Dog", a48, -1 + 8 * sx, 1 - 50 * sy, sx, sy);
	render_text("The Misaligned Fox Jumps Over The Lazy Dog", a48, -1 + 8.5 * sx, 1 - 100.5 * sy, sx, sy);

	/* Scaling the texture versus changing the font size */
	render_text("The Small Texture Scaled Fox Jumps Over The Lazy Dog", a48, -1 + 8 * sx, 1 - 175 * sy, sx * 0.5, sy * 0.5);
	render_text("The Small Font Sized Fox Jumps Over The Lazy Dog", a24, -1 + 8 * sx, 1 - 200 * sy, sx, sy);
	render_text("The Tiny Texture Scaled Fox Jumps Over The Lazy Dog", a48, -1 + 8 * sx, 1 - 235 * sy, sx * 0.25, sy * 0.25);
	render_text("The Tiny Font Sized Fox Jumps Over The Lazy Dog", a12, -1 + 8 * sx, 1 - 250 * sy, sx, sy);

	/* Colors and transparency */
	render_text("The Solid Black Fox Jumps Over The Lazy Dog", a48, -1 + 8 * sx, 1 - 430 * sy, sx, sy);

	glUniform4fv(uniform_color, 1, red);
	render_text("The Solid Red Fox Jumps Over The Lazy Dog", a48, -1 + 8 * sx, 1 - 330 * sy, sx, sy);
	render_text("The Solid Red Fox Jumps Over The Lazy Dog", a48, -1 + 28 * sx, 1 - 450 * sy, sx, sy);

	glUniform4fv(uniform_color, 1, transparent_green);
	render_text("The Transparent Green Fox Jumps Over The Lazy Dog", a48, -1 + 8 * sx, 1 - 380 * sy, sx, sy);
	render_text("The Transparent Green Fox Jumps Over The Lazy Dog", a48, -1 + 18 * sx, 1 - 440 * sy, sx, sy);
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

