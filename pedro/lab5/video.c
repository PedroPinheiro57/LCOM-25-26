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



// PROJECT
void video_clear_screen(uint32_t color) {
  for (uint16_t y = 0; y < vmi.YResolution; y++)
    for (uint16_t x = 0; x < vmi.XResolution; x++)
      vg_draw_pixel(x, y, color);
}

uint16_t video_get_hres(void) { return vmi.XResolution; }
uint16_t video_get_vres(void) { return vmi.YResolution; }
