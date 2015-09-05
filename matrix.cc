#include "matrix.hh"

matrix::matrix()
{
	size_cells_x = size_cells_y = 0;
	size_px_x = size_px_y = 0;
	char_size_x = char_size_y = 0;
	data = nullptr;
}

matrix::~matrix()
{
	deallocate_data();
}

void matrix::deallocate_data()
{
	if (!data)
		return;

	for (unsigned int y = 0; y < size_cells_y; y++)
		delete [] data[y];
	delete [] data;
}

void matrix::tell_character_sizes(float new_char_size_x, float new_char_size_y)
{
	char_size_x = new_char_size_x;
	char_size_y = new_char_size_y;
}

void matrix::set_size(unsigned int new_size_cells_x, unsigned int new_size_cells_y)
{
	size_cells_x = new_size_cells_x;
	size_cells_y = new_size_cells_y;

	size_px_x = size_cells_x * char_size_x;
	size_px_y = size_cells_y * char_size_y;

	deallocate_data();
	data = new cell* [size_cells_y];
	for (unsigned int y = 0; y < size_cells_y; y++)
		data[y] = new cell [size_cells_x];
		// for (unsigned int x = 0; x < size_cells_x; x++)
		// 	data[y][x].init();
}

