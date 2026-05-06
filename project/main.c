#include <lcom/lcf.h>
#include "../pedro/lab5/video.h"

int main(int argc, char *argv[]) {
  lcf_set_language("EN-US");
  lcf_trace_calls("/home/lcom/labs/project/trace.txt");
  lcf_log_output("/home/lcom/labs/project/output.txt");
  if (lcf_start(argc, argv))
    return 1;
  lcf_cleanup();
  return 0;
}

int(proj_main_loop)(int argc, char *argv[]) {
  if (video_map_vram(0x115) != 0) return 1;
  if (video_set_mode(0x115) != 0) return 1;

  video_clear_screen(0x1a1a2e);

  tickdelay(micros_to_ticks(3000000u));

  vg_exit();
  return 0;
}
