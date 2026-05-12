// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#include <lcom/lab5.h>

#include <stdint.h>
#include <stdio.h>

#include "video.h"

// Any header files included below this line should have been created by you

int main(int argc, char *argv[]) {
    lcf_set_language("EN-US");
    lcf_trace_calls("/home/lcom/labs/grupo_2leic01_5/jorge/lab5/trace.txt");
    lcf_log_output("/home/lcom/labs/grupo_2leic01_5/jorge/lab5/output.txt");

    if (lcf_start(argc, argv))
        return 1;

    lcf_cleanup();

    return 0;
}

int(video_test_init)(uint16_t mode, uint8_t delay) {

  if (video_set_mode(mode) != 0) return 1;

  tickdelay(micros_to_ticks((uint32_t)delay * 1000000u));

  if (vg_exit() != 0) return 1;

  return 0;

}

int(video_test_rectangle)(uint16_t mode, uint16_t x, uint16_t y,
                          uint16_t width, uint16_t height, uint32_t color) {
    /* To be completed */
    printf("%s(0x%03X, %u, %u, %u, %u, 0x%08x): under construction\n",
            __func__, mode, x, y, width, height, color);

    return 1;
}

int(video_test_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {
    /* To be completed */
    printf("%s(%8p, %u, %u): under construction\n", __func__, xpm, x, y);

    return 1;
}
