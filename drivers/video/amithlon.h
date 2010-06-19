#ifndef AMITHLON_H
#define AMITHLON_H

typedef struct {
  unsigned int sx;
  unsigned int sy;
  unsigned int width;
  unsigned int height;
  unsigned long colour;
  unsigned int vxres;
} amithlon_fill;

typedef struct {
  unsigned int sx;
  unsigned int sy;
  unsigned int dx;
  unsigned int dy;
  unsigned long offset;
  unsigned int Bpp;
  unsigned long colour;
  unsigned int vxres;
} amithlon_line;

typedef struct {
  unsigned int sx;
  unsigned int sy;
  unsigned int dx;
  unsigned int dy;
  unsigned int width;
  unsigned int height;
  unsigned int vxres;
} amithlon_copy;

typedef struct {
  unsigned int sx;
  unsigned int sy;
  unsigned int dx;
  unsigned int dy;
  unsigned int width;
  unsigned int height;
  unsigned int spitch;
  unsigned int dpitch;
  unsigned int op;
  unsigned int oldpitch;
  unsigned int Bpp;
} amithlon_copy_complete;

typedef struct {
  unsigned int sx;
  unsigned int sy;
  unsigned int width;
  unsigned int height;
  unsigned int colour0;
  unsigned int colour1;
  unsigned int offset;
  unsigned int pitch;
  char*    data;
  unsigned char rop3;
  unsigned int vxres;
} amithlon_blittemplate;

typedef struct {
  unsigned int* data;
  unsigned char* red;
  unsigned char* green;
  unsigned char* blue;
} amithlon_setcursor;

typedef struct {
  unsigned int x;
  unsigned int y;
  unsigned int on;
} amithlon_poscursor;

#define AMITHLON_FILL_RECT 91827364
#define AMITHLON_COPY_RECT 91827365
#define AMITHLON_BLIT_TEMP 91827366
#define AMITHLON_SET_CURSOR 91827367
#define AMITHLON_POS_CURSOR 91827368
#define AMITHLON_COPY_RECT_COMPLETE 91827369
#define AMITHLON_DRAW_LINE 91827370
#define MATROX_CROSS_4MB 0x76287492
#define AMITHLON_MAXCLOCK 0x76287493
#endif
