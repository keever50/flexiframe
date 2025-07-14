#include <stdio.h>
#include <stdlib.h>

#include "flexiframe.h"

flexi_instance_s g_flex_inst;

void testcb(struct flexi_instance_s *inst, const struct flexi_event_s *event, const struct flexi_frame_s *frame)
{
  printf("EVENT %d for %d. frametype %d\n\r", event->event_type, event->listener_id, frame->frame_type);
}

int main ()
{
  flexi_init(&g_flex_inst);

  flexi_register_event(&g_flex_inst, testcb, 1, 55, NULL);
  flexi_register_event(&g_flex_inst, testcb, 3, 0xBB, NULL);

  //flexi_unregister_event(&g_flex_inst, 1);

  uint8_t testdata[] = {0xAA, 0xAA, 0x42, 12, 0, 0, 0xBB, 2, 20, 30,      0xAA, 0xAA, 0x42, 13, 0, 0, 55, 2, 50, 60};
  for (size_t i = 0; i < sizeof(testdata); i++)
    {
      flexi_intake(&g_flex_inst, testdata[i]);
    }

  return 0;
}