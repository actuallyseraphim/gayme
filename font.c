#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  size_t glyth_count;
  uint64_t* glyth_data;
} bitmap_8x8font;

bitmap_8x8font parse8x8Font(FILE* fptr) {
  char line[256];
  int font_char_count, font_width, font_height;
  while(fgets(line, sizeof(line), fptr)) {
    sscanf(line, "FONTBOUNDINGBOX %i %i", &font_width, &font_height);
    sscanf(line, "CHARS %i", &font_char_count);
  }

  assert(font_width == 8);
  assert(font_height == 8);

  bitmap_8x8font bmf = {0};
  bmf.glyth_count = font_char_count;
  bmf.glyth_data = (uint64_t*)calloc(bmf.glyth_count, sizeof(uint64_t));

  fseek(fptr, 0, SEEK_SET);
  int glyth_id = 0;
  while(fgets(line, sizeof(line), fptr)) {
    char char_name[16];
    int suc = sscanf(line, "STARTCHAR %15s", char_name);
    if (!suc) {
      continue;
    }
    int bbx_w, bbx_h, bbx_x, bbx_y;
    while(fgets(line, sizeof(line), fptr)) {
      if (!strncmp(line, "ENDCHAR", 7)) {
        glyth_id++;
        break;
      };
      sscanf(line, "BBX %i %i %i %i", &bbx_w, &bbx_h, &bbx_x, &bbx_y);
      if (!strncmp(line, "BITMAP\n", 7)) {
        for (int i = 0; i < bbx_h; i++) {
          fgets(line, sizeof(line), fptr);
          unsigned int b = 0;
          if (sscanf(line, "%2X", &b)) {
            // ignore exess bits
            b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
            b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
            b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
            b <<= bbx_x;
            
            ((uint8_t*)bmf.glyth_data)[8*glyth_id+i+8-bbx_h] = b;
          } else break;
        }
      }
    }
  }
  return bmf;
}
