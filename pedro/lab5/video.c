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
  unsigned int vram_size = vmi.XResolution * vmi.YResolution
                           * ((vmi.BitsPerPixel + 7) / 8);

  bytes_per_pixel = (vmi.BitsPerPixel + 7) / 8;

  struct minix_mem_range mr;
  mr.mr_base  = (phys_bytes) vram_base;
  mr.mr_limit = mr.mr_base + vram_size;

  int r;
  if (OK != (r = sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr))) {
    panic("sys_privctl (ADD_MEM) failed: %d\n", r);
    return 1;
  }

  video_mem = vm_map_phys(SELF, (void *) mr.mr_base, vram_size);
  if (video_mem == MAP_FAILED) {
    panic("couldn't map video memory");
    return 1;
  }

  return 0;
}

int video_set_mode(uint16_t mode) {
  reg86_t reg;
  memset(&reg, 0, sizeof(reg));

  reg.intno = 0x10;
  reg.ah    = 0x4F;
  reg.al    = 0x02;
  reg.bx    = mode | BIT(14);

  if (sys_int86(&reg) != OK) {
    printf("video_set_mode: sys_int86() failed\n");
    return 1;
  }

  return 0;
}

int vg_draw_pixel(uint16_t x, uint16_t y, uint32_t color) {
  if (x >= vmi.XResolution || y >= vmi.YResolution) return 1;

  char *pixel = video_mem + (y * vmi.XResolution + x) * bytes_per_pixel;
  memcpy(pixel, &color, bytes_per_pixel);
  return 0;
}
