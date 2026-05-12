#include <lcom/lcf.h>
#include <string.h>
#include <stdio.h>
#include "video.h"

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
