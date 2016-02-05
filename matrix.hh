#ifndef MATRIX_HH
#define MATRIX_HH

#include "utils.hh"
#include <string>

template <typename T>
class generic_mat {
  void mat_init();
  void deallocate_data();
  T **data;
public:
  unsigned int sx, sy;
  void set_size(unsigned int nsx, unsigned int nsy);
};

struct cell {
  uint32_t fg, bg;
  uint32_t character;
  cell() : character('A') {};
};

class matrix : public generic_mat<cell> {
  float charsize_x, charsize_y;
public:
  matrix();
  ~matrix();
};

#endif

// vim: et:sw=2

