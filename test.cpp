#ifdef FLEXIFRAME_TESTS
#include <stdio.h>
#include <stdlib.h>

#include "flexiframe.h"

flexi_instance_s g_flex_inst;

#define EVENTS_TEST 1

void testcb(struct flexi_instance_s *inst, const struct flexi_event_s *event, const struct flexi_info_s *info, const struct flexi_payload_s *payload)
{
  printf("Listener %d: Event %d received. Frame type %d, frame ID %d\n\r", event->listener_id, info->event, info->frame_type, info->frameid);
  for (size_t i = 0; i < payload->len; i++)
    {
      printf("0x%X ", payload->data[i]);
    }
  printf("\n\r");
  printf("user_data: %s\n\r", (char *)event->user_data);

  flexi_send(&g_flex_inst, info->frameid, FLEXI_TYPE_RESPONSE, EVENTS_TEST, (uint8_t *)event->user_data, 4);
}

int txcb(struct flexi_instance_s *inst, const uint8_t *buf, size_t len)
{
  printf("TX>");
  for (size_t i = 0; i < len; i++)
    {
      printf("0x%X ", buf[i]);
    }
  printf("\n\r");
  return 0;
}

int main()
{
  flexi_init(&g_flex_inst);
  flexi_set_tx_cb(&g_flex_inst, txcb);
  flexi_register_event(&g_flex_inst, testcb, 0, EVENTS_TEST, (void *)"hello!");

  uint8_t dat[] = {5,6,7,8};
  flexi_send(&g_flex_inst, 1, FLEXI_TYPE_COMMAND, EVENTS_TEST, dat, sizeof(dat));

  /* For testing, read internal TX buffer */

  for (size_t i = 0; i < g_flex_inst.txlen; i++)
    {
      printf("RX<0x%X: ", g_flex_inst.txbuf[i]);
      flexi_feed(&g_flex_inst, g_flex_inst.txbuf[i]);
    }
  printf("\n\r");

  return 0;
}


#endif