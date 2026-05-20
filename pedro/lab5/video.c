#include <lcom/lcf.h>
#include <string.h>
#include <stdio.h>
#include "video.h"

static char           *video_mem;
static vbe_mode_info_t vmi;
static unsigned        bytes_per_pixel;

int video_map_vram(uint16_t mode) {
  if (vbe_get_mode_info(mode, &vmi) != 0) {
    printf("video_map_vram: vbe_get_mode_info() failed\n");
    return 1;
  }

  unsigned int vram_base = vmi.PhysBasePtr;
  unsigned int vram_size = vmi.XResolution * vmi.YResolution * ((vmi.BitsPerPixel + 7) / 8);

  bytes_per_pixel = (vmi.BitsPerPixel + 7) / 8;

  /* Grant memory mapping permissions */
  struct minix_mem_range mr;
  mr.mr_base  = (phys_bytes) vram_base;
  mr.mr_limit = mr.mr_base + vram_size;

  int r;
  if (OK != (r = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr))) {
    panic("sys_privctl (ADD_MEM) failed: %d\n", r);
    return 1;
  }

  /* Map memory */
  video_mem = vm_map_phys(SELF, (void *) mr.mr_base, vram_size);
  if (video_mem == MAP_FAILED) {
    panic("couldn't map video memory");
    return 1;
  }

  return 0;
}

// Switches the graphics card into a given video mode (like 0x105) using BIOS interrupt 0x10.
int video_set_mode(uint16_t mode) {
  reg86_t reg; // reg86_t is a struct that represents CPU registers for a BIOS interrupt call
  memset(&reg, 0, sizeof(reg));

  reg.intno = 0x10;
  reg.ah    = 0x4F;
  reg.al    = 0x02;
  reg.bx    = mode | BIT(14);

  // Call BIOS interrupt; executes the int 0x10 "syscall"
  if (sys_int86(&reg) != OK) {
    printf("video_set_mode: sys_int86() failed\n");
    return 1;
  }

  return 0;
}

int(vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  for (uint16_t row = 0; row < height; row++) {
    if (vg_draw_hline(x, y + row, width, color) != 0) return 1;
  }
  return 0;
}

int(vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  for (uint16_t i = 0; i < len; i++) {
    if (vg_draw_pixel(x + i, y, color) != 0) return 1;
  }
  return 0;
}

int vg_draw_pixel(uint16_t x, uint16_t y, uint32_t color) {
  if (x >= vmi.XResolution || y >= vmi.YResolution) return 1;

  char *pixel = video_mem + (y * vmi.XResolution + x) * bytes_per_pixel;
  memcpy(pixel, &color, bytes_per_pixel);
  return 0;
}



/* PROJECT */

#define NUM_BUFFERS 2

static uint8_t        *vram = NULL;
static unsigned        buffer_size;
static int             current_view_buffer = 0;
static int             current_draw_buffer = 1;

static inline void _put_pixel(uint16_t x, uint16_t y, uint32_t color) {
  unsigned int buffer_offset = current_draw_buffer * buffer_size;
  unsigned int index = buffer_offset + (vmi.XResolution * y + x) * bytes_per_pixel;
  uint8_t *dst = vram + index;
  memcpy(dst, &color, bytes_per_pixel);
}

static void _fill_hrun(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  unsigned int buffer_offset = current_draw_buffer * buffer_size;
  unsigned int index = buffer_offset + (vmi.XResolution * y + x) * bytes_per_pixel;
  uint8_t *dst = vram + index;
  for (uint16_t i = 0; i < len; i++, dst += bytes_per_pixel)
    memcpy(dst, &color, bytes_per_pixel);
}

void video_clear_screen(uint32_t color) {
  (void) color;  /* always clear to black */
  unsigned int buffer_offset = current_draw_buffer * buffer_size;
  memset(vram + buffer_offset, 0, buffer_size);
}

uint16_t video_get_hres(void) { return vmi.XResolution; }
uint16_t video_get_vres(void) { return vmi.YResolution; }


int(vg_draw_rectangle_project)(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
  if (x >= vmi.XResolution || y >= vmi.YResolution) return 1;
  if (x + w > vmi.XResolution) w = vmi.XResolution - x;
  if (y + h > vmi.YResolution) h = vmi.YResolution - y;
  for (uint16_t row = 0; row < h; row++)
    _fill_hrun(x, y + row, w, color);
  return 0;
}

void vg_draw_pixel_fast(uint16_t x, uint16_t y, uint32_t color) {
  if (x >= vmi.XResolution || y >= vmi.YResolution) return;
  _put_pixel(x, y, color);
}

int(vg_draw_hline_project)(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  if (y >= vmi.YResolution) return 1;
  if (x >= vmi.XResolution) return 1;
  if (x + len > vmi.XResolution) len = vmi.XResolution - x;
  _fill_hrun(x, y, len, color);
  return 0;
}

int video_init(uint16_t mode) {
  if (vbe_get_mode_info(mode, &vmi) != 0) {
    printf("video_init: vbe_get_mode_info() failed\n");
    return 1;
  }

  bytes_per_pixel = (vmi.BitsPerPixel + 7) / 8;
  buffer_size     = vmi.XResolution * vmi.YResolution * bytes_per_pixel;

  struct minix_mem_range mr;
  mr.mr_base  = (phys_bytes) vmi.PhysBasePtr;
  mr.mr_limit = mr.mr_base + buffer_size * NUM_BUFFERS;

  int r;
  if (OK != (r = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr))) {
    panic("sys_privctl (ADD_MEM) failed: %d\n", r);
    return 1;
  }

  vram = vm_map_phys(SELF, (void *) mr.mr_base, buffer_size * NUM_BUFFERS);
  if (vram == MAP_FAILED) {
    panic("couldn't map video memory");
    return 1;
  }

  reg86_t reg;
  memset(&reg, 0, sizeof(reg));
  reg.intno = 0x10;
  reg.ah    = 0x4F;
  reg.al    = 0x02;
  reg.bx    = mode | BIT(14);
  if (sys_int86(&reg) != OK) {
    printf("video_init: sys_int86() failed\n");
    return 1;
  }

  current_view_buffer = 0;
  current_draw_buffer = 1;

  return 0;
}

static void set_display_buffer(int buffer_index) {
  reg86_t r;
  memset(&r, 0, sizeof(r));
  r.ax    = 0x4F07;                              /* VBE function 07h */
  r.bx    = 0x0080;                              /* wait for vertical retrace */
  r.cx    = 0;                                   /* X offset */
  r.dx    = buffer_index * vmi.YResolution;      /* Y offset determines buffer */
  r.intno = 0x10;
  sys_int86(&r);
}

int video_swap_buffers(void) {
  set_display_buffer(current_draw_buffer);
  current_view_buffer = current_draw_buffer;
  current_draw_buffer = (current_draw_buffer + 1) % NUM_BUFFERS;
  return 0;
}


/* --- public API --- */

int vg_draw_pixel_project(uint16_t x, uint16_t y, uint32_t color) {
  if (x >= vmi.XResolution || y >= vmi.YResolution) return 1;
  _put_pixel(x, y, color);
  return 0;
}

