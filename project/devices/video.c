#include <lcom/lcf.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "video.h"

static uint8_t        *img_buffer  = NULL;
static uint8_t        *vram        = NULL;
static vbe_mode_info_t vmi;
static unsigned        bytes_per_pixel;
static unsigned        buffer_size;

int video_init(uint16_t mode) {
  if (vbe_get_mode_info(mode, &vmi) != 0) {
    printf("video_init: vbe_get_mode_info() failed\n");
    return 1;
  }

  bytes_per_pixel = (vmi.BitsPerPixel + 7) / 8;
  buffer_size     = vmi.XResolution * vmi.YResolution * bytes_per_pixel;

  struct minix_mem_range mr;
  mr.mr_base  = (phys_bytes) vmi.PhysBasePtr;
  mr.mr_limit = mr.mr_base + buffer_size;

  int r;
  if (OK != (r = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr))) {
    panic("sys_privctl (ADD_MEM) failed: %d\n", r);
    return 1;
  }

  vram = vm_map_phys(SELF, (void *) mr.mr_base, buffer_size);
  if (vram == MAP_FAILED) {
    panic("couldn't map video memory");
    return 1;
  }

  img_buffer = malloc(buffer_size);
  if (img_buffer == NULL) {
    panic("couldn't allocate image buffer");
    return 1;
  }
  memset(img_buffer, 0, buffer_size);

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

  return 0;
}

int video_swap_buffers(void) {
  memcpy(vram, img_buffer, buffer_size);
  return 0;
}

/* --- internal fast helpers --- */

/* Write one pixel directly — no bounds check, caller must ensure validity. */
static inline void _put_pixel(uint16_t x, uint16_t y, uint32_t color) {
  uint8_t *dst = img_buffer + (vmi.XResolution * y + x) * bytes_per_pixel;
  memcpy(dst, &color, bytes_per_pixel);
}

/* Fill a horizontal run of `len` pixels with `color` in one tight loop. */
static void _fill_hrun(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  uint8_t *dst = img_buffer + (vmi.XResolution * y + x) * bytes_per_pixel;
  for (uint16_t i = 0; i < len; i++, dst += bytes_per_pixel)
    memcpy(dst, &color, bytes_per_pixel);
}

/* --- public API --- */

int vg_draw_pixel(uint16_t x, uint16_t y, uint32_t color) {
  if (x >= vmi.XResolution || y >= vmi.YResolution) return 1;
  _put_pixel(x, y, color);
  return 0;
}

/*
 * vg_draw_pixel_fast — bounds-checked but inlined write.
 * Use this from font/sprite code instead of vg_draw_pixel so the
 * compiler can inline the bounds check and eliminate the function
 * call overhead from the inner loops.
 */
void vg_draw_pixel_fast(uint16_t x, uint16_t y, uint32_t color) {
  if (x >= vmi.XResolution || y >= vmi.YResolution) return;
  _put_pixel(x, y, color);
}

int(vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  if (y >= vmi.YResolution) return 1;
  if (x >= vmi.XResolution) return 1;
  if (x + len > vmi.XResolution) len = vmi.XResolution - x;
  _fill_hrun(x, y, len, color);
  return 0;
}

int(vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
  if (x >= vmi.XResolution || y >= vmi.YResolution) return 1;
  if (x + w > vmi.XResolution) w = vmi.XResolution - x;
  if (y + h > vmi.YResolution) h = vmi.YResolution - y;
  for (uint16_t row = 0; row < h; row++)
    _fill_hrun(x, y + row, w, color);
  return 0;
}

void video_clear_screen(uint32_t color) {
  (void) color;              /* color is ignored — always clear to black */
  memset(img_buffer, 0, buffer_size);
}

uint16_t video_get_hres(void) { return vmi.XResolution; }
uint16_t video_get_vres(void) { return vmi.YResolution; }
