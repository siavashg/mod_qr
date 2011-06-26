#include <png.h>
#include <qrencode.h>

struct mem_encode
{
  char *buffer;
  size_t size;
};

void qrPNG(const char *intext, struct mem_encode *png);
