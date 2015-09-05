#ifndef MATRIX_HH
#define MATRIX_HH

#include <string>

struct cell {
	uint32_t character;
	cell() : character('A') { puts("init"); };
};

struct matrix {
	unsigned int size_cells_x, size_cells_y,
				 size_px_x, size_px_y;
	cell **data;

	matrix();
	~matrix();

	void tell_character_sizes(float new_char_size_x, float new_char_size_y);
	void set_size(unsigned int new_size_cells_x, unsigned int new_size_cells_y);
private:
	float char_size_x, char_size_y;
	void deallocate_data();
};

#endif

