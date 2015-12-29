#ifndef MATRIX_HH
#define MATRIX_HH

#include "utils.hh"
#include <string>

struct cell {
  uint32_t fg, bg;
  uint32_t character;
  cell() : character('A') {};
};

class matrix {
  float charsize_x, charsize_y;
  void deallocate_data();
public:
  unsigned int sx, sy;
  cell **data;

  matrix();
  ~matrix();

  void set_size(unsigned int nsx, unsigned int nsy);
};

#endif

// vim: et:sw=2

