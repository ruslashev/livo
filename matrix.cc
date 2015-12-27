#include "matrix.hh"

matrix::matrix()
{
  sx = sx = 0;
  charsize_x = charsize_y = 0;
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
  for (ull y = 0; y < sy; y++)
    delete [] data[y];
  delete [] data;
}

void matrix::set_size(unsigned int nsx, unsigned int nsy)
{
  sx = nsx;
  sy = nsy;

  deallocate_data();
  data = new cell* [sy];
  for (ull y = 0; y < sy; y++)
    data[y] = new cell [sx];
}

// vim: et:sw=2

