#pragma once
#include <stdint.h>
#include <lcom/lcf.h>

#define VBE_SET_MODE    0x4F02
#define VBE_LINEAR_BIT  BIT(14)
#define BIOS_VIDEO_INT  0x10

int video_set_mode(uint16_t mode);
