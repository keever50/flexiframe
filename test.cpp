#include <stdio.h>
#include <stdlib.h>

#include "flexiframe.h"

flexi_instance_s g_flex_inst;

#define EVENTS_FOPEN 1
#define EVENTS_FWRITE 2
#define EVENTS_FCLOSE 3

void fs_fopen_cb(struct flexi_instance_s *inst, const struct flexi_event_s *event, const struct flexi_frame_s *frame)
{
  printf("FOPEN\n\r");


}

void fs_fclose_cb(struct flexi_instance_s *inst, const struct flexi_event_s *event, const struct flexi_frame_s *frame)
{
  printf("FCLOSE\n\r");
}

void fs_fwrite_cb(struct flexi_instance_s *inst, const struct flexi_event_s *event, const struct flexi_frame_s *frame)
{
  printf("FWRITE\n\r");
}

int main()
{
  flexi_init(&g_flex_inst);

  flexi_register_event(&g_flex_inst, fs_fopen_cb,  0, EVENTS_FOPEN,  NULL);
  flexi_register_event(&g_flex_inst, fs_fclose_cb, 1, EVENTS_FWRITE, NULL);
  flexi_register_event(&g_flex_inst, fs_fwrite_cb, 2, EVENTS_FCLOSE, NULL);

  /* Test */

  uint8_t *testpacket = NULL;
  size_t len;

  uint8_t args[] = "test.txt";
  flexi_allocate_frame(&g_flex_inst, &testpacket, &len, FLEXI_TYPE_COMMAND, EVENTS_FOPEN, args, sizeof(args));
  
  for (size_t i = 0; i < len; i++)
    {
      flexi_intake(&g_flex_inst, testpacket[i]);
    }

  flexi_free(&g_flex_inst, &testpacket);

  return 0;
}

// void testcb(struct flexi_instance_s *inst, const struct flexi_event_s *event, const struct flexi_frame_s *frame)
// {
//   printf("EVENT %d for %d. frametype %d. frameid %d\n\r", event->event_type, event->listener_id, frame->frame_type, frame->frameid);
// }

// int main ()
// {
//   flexi_init(&g_flex_inst);

//   flexi_register_event(&g_flex_inst, testcb, 1, 55, NULL);
//   flexi_register_event(&g_flex_inst, testcb, 3, 0xBB, NULL);

//   for (size_t tries = 0; tries < 40000; tries++)
//     {
//       uint8_t *testpacket = NULL;
//       size_t testpacket_len = 0;
//       uint8_t data[] = {50, 40, 42, 2, 0, 10};
//       flexi_allocate_frame(&g_flex_inst, &testpacket, &testpacket_len, FLEXI_TYPE_COMMAND, 55, data, sizeof(data));
      
//       for (size_t i = 0; i < testpacket_len; i++)
//         {
//           flexi_intake(&g_flex_inst, testpacket[i]);
//         }
      
//       flexi_free(&g_flex_inst, &testpacket);
//     }


//   return 0;
// }