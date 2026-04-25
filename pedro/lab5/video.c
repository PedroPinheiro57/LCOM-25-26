#include <lcom/lcf.h>
#include <machine/int86.h>
#include <sys/mman.h>
#include <string.h>
#include <stdio.h>
#include "video.h"

static uint8_t *video_mem   = NULL;
static uint16_t h_res       = 0;
static uint16_t v_res       = 0;
static uint8_t  bits_per_px = 0;

int video_set_mode(uint16_t mode) {
  vbe_mode_info_t info;
  memset(&info, 0, sizeof(info));

  if (vbe_get_mode_info(mode, &info) != 0) {
    printf("video_set_mode: vbe_get_mode_info() failed\n");
    return 1;
  }

  reg86_t reg;
  memset(&reg, 0, sizeof(reg));
  reg.intno = BIOS_VIDEO_INT;
  reg.ax    = VBE_SET_MODE;
  reg.bx    = mode | VBE_LINEAR_BIT;

  if (sys_int86(&reg) != OK) {
    printf("video_set_mode: sys_int86() failed\n");
    return 1;
  }

  if (reg.ax != 0x004F) {
    printf("video_set_mode: VBE error, AX=0x%04X\n", reg.ax);
    return 1;
  }

  h_res       = info.XResolution;
  v_res       = info.YResolution;
  bits_per_px = info.BitsPerPixel;

  /* Map the physical frame buffer into our virtual address space */
  struct minix_mem_range mr;
  unsigned int vram_size = (unsigned int)h_res * v_res * ((bits_per_px + 7) / 8);

  mr.mr_base = info.PhysBasePtr;
  mr.mr_limit = mr.mr_base + vram_size;

  if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &mr) != OK) {
    printf("video_set_mode: sys_privctl() failed\n");
    return 1;
  }

  video_mem = (uint8_t *) vm_map_phys(SELF, (void *)(uintptr_t) info.PhysBasePtr, vram_size);
  if (video_mem == MAP_FAILED) {
    printf("video_set_mode: vm_map_phys() failed\n");
    return 1;
  }

  return 0;
}
