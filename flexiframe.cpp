#include "flexiframe.h"
#include <string.h>
#include <endian.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define FLEXI_MAGIC_START 0x42

enum flexi_frame_section_e
{
  FLEXI_FRAME_START,
  FLEXI_FRAME_ID_LOW,
  FLEXI_FRAME_ID_HIGH,
  FLEXI_FRAME_TYPE,
  FLEXI_FRAME_EVENT,
  FLEXI_FRAME_DATA_LEN,
  FLEXI_FRAME_INV_DATA_LEN,
  FLEXI_FRAME_PAYLOAD,
  FLEXI_FRAME_CHECKSUM
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
  inst->sum += byte;

  switch (inst->headerpos)
    {
      case FLEXI_FRAME_START:
        if (byte != FLEXI_MAGIC_START) break;
        inst->headerpos = FLEXI_FRAME_ID_LOW;
        printf("\n\rSTART %d\n\r", byte);
        inst->sum = byte;
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
        inst->headerpos = FLEXI_FRAME_INV_DATA_LEN;
        printf("LEN %d\n\r", byte);
      break;

      case FLEXI_FRAME_INV_DATA_LEN:
        {
          printf("INVLEN %d (inverted %d)\n\r", byte, (uint8_t)~byte);
          if (inst->frame.data_len != (uint8_t)~byte)
            {
              printf("Length did not match inverted length\n\r");
              inst->headerpos = FLEXI_FRAME_START;
              break;
            }
          inst->headerpos = FLEXI_FRAME_PAYLOAD;
        }
      break;
      
      case FLEXI_FRAME_PAYLOAD:
        
        inst->frame.data[inst->datapos] = byte;
        if (inst->datapos < FLEXIFRAME_MAX_DATA_LEN) inst->datapos++;

        printf("PAYLOAD %d (%d / %d)\n\r", byte, inst->datapos, inst->frame.data_len);

        if (inst->datapos >= inst->frame.data_len )
          {
            inst->datapos = 0;
            inst->headerpos = FLEXI_FRAME_CHECKSUM;
            printf("End of payload\n\r");
          }
      break;

      case FLEXI_FRAME_CHECKSUM:
        {
          printf("CHECKSUM %d (should be %d)\n\r", byte, (uint8_t)(inst->sum - byte));
          if (byte != (uint8_t)(inst->sum - byte))
            {
              inst->headerpos = FLEXI_FRAME_START;
              printf("Incorrect checksum\n\r\n\r");
              break;
            }

          inst->headerpos = FLEXI_FRAME_START;

          printf("Publishing\n\r\n\r");
          flexi_publish(inst, &inst->frame);
        }
      break;

      default:
        printf("Unknown parser state\n\r");
      break;
    }

  return inst->state;
}

int flexi_allocate_frame(struct flexi_instance_s *inst,
                         uint8_t **frame_alloc, size_t *alloc_len,
                         enum flexi_frame_type_e frame_type, uint8_t event,
                         const uint8_t *data, size_t data_len)
{

  if ((*frame_alloc) != NULL)
    {
      printf("frame allocation was not freed or is not NULL\n\r");
      return -2;
    }

  if (frame_type == FLEXI_TYPE_COMMAND)
    inst->last_id++;

  /* Allocation size is data length + header before payload + checksum */

  size_t len = data_len + FLEXI_FRAME_PAYLOAD + 1;
  (*frame_alloc) = (uint8_t *)malloc(len);
  if ((*frame_alloc) == NULL)
    {
      printf("flexi malloc fail\n\r");
      return -1;
    }
  printf("Allocated %d\n\r", len);

  (*alloc_len) = len;
  
  size_t i = 0;
  (*frame_alloc)[i++] = FLEXI_MAGIC_START;
  (*frame_alloc)[i++] = ((uint8_t *)&inst->last_id)[0];
  (*frame_alloc)[i++] = ((uint8_t *)&inst->last_id)[1];
  (*frame_alloc)[i++] = frame_type;
  (*frame_alloc)[i++] = event;
  (*frame_alloc)[i++] = data_len;
  (*frame_alloc)[i++] = (uint8_t)~data_len;
  for (size_t i_data = 0; i_data < data_len; i_data++)
    (*frame_alloc)[i++] = data[i_data];

  uint8_t sum = 0;
  for (size_t sumi = 0; sumi < i; sumi++)
      sum += (*frame_alloc)[sumi];

  (*frame_alloc)[i++] = sum;

  return inst->last_id;
}

int flexi_free(struct flexi_instance_s *inst, uint8_t **frame_alloc)
{
  if ((*frame_alloc) == NULL)
    {
      printf("frame allocation was already freed\n\r");
      return -2;
    }
  free((*frame_alloc));
  (*frame_alloc) = NULL;

  return 0;
}