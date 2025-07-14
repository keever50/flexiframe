#include "flexiframe.h"
#include <string.h>
#include <endian.h>

enum flexi_frame_section_e
{
  FLEXI_FRAME_START,
  FLEXI_FRAME_ID_LOW,
  FLEXI_FRAME_ID_HIGH,
  FLEXI_FRAME_TYPE,
  FLEXI_FRAME_EVENT,
  FLEXI_FRAME_DATA_LEN,
  FLEXI_FRAME_PAYLOAD
};


void flexi_init(struct flexi_instance_s *inst)
{
  memset(inst, 0, sizeof(struct flexi_instance_s));
}

int flexi_register_event(struct flexi_instance_s *inst, flexi_event_cb cb, int listener_id, uint8_t event_type, void *user_data)
{
  for (size_t i = 0; i < FLEXIFRAME_MAX_EVENTS; i++)
    {
      if (inst->events[i].cb != NULL) continue;
      inst->events[i].cb = cb;
      inst->events[i].listener_id = listener_id;
      inst->events[i].user_data = user_data;
      inst->events[i].event_type = event_type;
      return 0;
    }
  return -1;
}

int flexi_unregister_event(struct flexi_instance_s *inst, int listener_id)
{
  int res = -1;
  for (size_t i = 0; i < FLEXIFRAME_MAX_EVENTS; i++)
    {
      if (inst->events[i].listener_id != listener_id) continue;
      memset(&inst->events[i], 0, sizeof(struct flexi_event_s));
      res = 0;
    }
  return res;
}

void flexi_publish(struct flexi_instance_s *inst, const struct flexi_frame_s *frame)
{
  for (size_t i = 0; i < FLEXIFRAME_MAX_EVENTS; i++)
    {
      if (inst->events[i].cb == NULL) continue;
      if (inst->events[i].event_type != frame->event) continue;
      inst->events[i].cb(inst, &inst->events[i], frame);
    }
}

enum flexi_status_e flexi_intake(struct flexi_instance_s *inst, uint8_t byte)
{
  switch (inst->headerpos)
    {
      case FLEXI_FRAME_START:
        if (byte != 0x42) break;
        inst->headerpos = FLEXI_FRAME_ID_LOW;
        printf("\n\rSTART\n\r");
      break;

      case FLEXI_FRAME_ID_LOW:
        ((uint8_t *)&inst->frame.frameid)[0] = byte;
        inst->headerpos = FLEXI_FRAME_ID_HIGH;
        printf("IDL %d\n\r", byte);
      break;

      case FLEXI_FRAME_ID_HIGH:
        ((uint8_t *)&inst->frame.frameid)[1] = byte;
        inst->headerpos = FLEXI_FRAME_TYPE;
        printf("IDH %d\n\r", byte);
      break;

      case FLEXI_FRAME_TYPE:
        inst->frame.frame_type = byte;
        inst->headerpos = FLEXI_FRAME_EVENT;
        printf("FRAMETYPE %d\n\r", byte);
      break;

      case FLEXI_FRAME_EVENT:
        inst->frame.event = byte;
        inst->headerpos = FLEXI_FRAME_DATA_LEN;
        printf("EVENT %d\n\r", byte);
      break;

      case FLEXI_FRAME_DATA_LEN:
        if (byte > FLEXIFRAME_MAX_DATA_LEN)
          {
            printf("LEN bigger than maximum data len %d/%d. Ignoring remainder", byte, FLEXIFRAME_MAX_DATA_LEN);
          }

        inst->frame.data_len = byte;
        inst->headerpos = FLEXI_FRAME_PAYLOAD;
        printf("LEN %d\n\r", byte);
      break;

      case FLEXI_FRAME_PAYLOAD:
        
        inst->frame.data[inst->datapos];
        if (inst->datapos < FLEXIFRAME_MAX_DATA_LEN) inst->datapos++;

        printf("PAYLOAD %d (%d / %d)\n\r", byte, inst->datapos, inst->frame.data_len);

        if (inst->datapos >= inst->frame.data_len)
          {
            inst->datapos = 0;
            inst->headerpos = FLEXI_FRAME_START;
            printf("End of frame. Publishing\n\r\n\r");
            flexi_publish(inst, &inst->frame);
          }
      break;


      default:
        printf("Unknown parser state\n\r");
      break;
    }

  return inst->state;
}