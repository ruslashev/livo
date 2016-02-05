#include "matrix.hh"

template <typename T>
void generic_mat::mat_init()
{
  sx = sy = 0;
  data = nullptr;
}

template <typename T>
void generic_mat::deallocate_data()
{
  if (!data)
    return;
  for (ull y = 0; y < sy; y++)
    delete [] data[y];
  delete [] data;
}

template <typename T>
void generic_mat::set_size(unsigned int nsx, unsigned int nsy)
{
  sx = nsx;
  sy = nsy;

  deallocate_data();
  data = new T* [sy];
  for (ull y = 0; y < sy; y++)
    data[y] = new T [sx];
}

matrix::matrix()
{
  mat_init();
  charsize_x = charsize_y = 0;
}

matrix::~matrix()
{
  deallocate_data();
}

// vim: et:sw=2

