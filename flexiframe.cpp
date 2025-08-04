#include "flexiframe.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#define FLEXI_MAGIC_START 0x42

#ifdef FLEXI_CONFIG_DEBUG
#define FLEXI_PRINTF(...)  printf(__VA_ARGS__)
#else
#define FLEXI_PRINTF(...)  ((void)0)
#endif

enum flexi_info_section_e
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
      inst->events[i].called = false;
      inst->events[i].returned = 0;
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

void flexi_publish(struct flexi_instance_s *inst, const struct flexi_info_s *info)
{
  for (size_t i = 0; i < FLEXIFRAME_MAX_EVENTS; i++)
    {
      if (inst->events[i].cb == NULL) continue;
      if (inst->events[i].event_type != info->event) continue;
      struct flexi_payload_s payload;
      payload.data = inst->rxpayload;
      payload.len = info->data_len;
      inst->events[i].returned = inst->events[i].cb(inst, &inst->events[i], info, &payload);
      inst->events[i].called = true;
    }
}



enum flexi_status_e flexi_feed(struct flexi_instance_s *inst, uint8_t byte)
{
  

  switch (inst->headerpos)
    {
      case FLEXI_FRAME_START:
        if (byte != FLEXI_MAGIC_START) break;
        inst->headerpos = FLEXI_FRAME_ID_LOW;
        FLEXI_PRINTF("\n\rSTART %d\n\r", byte);
        inst->sum = 0;
      break;

      case FLEXI_FRAME_ID_LOW:
        ((uint8_t *)&inst->info.frameid)[0] = byte;
        inst->headerpos = FLEXI_FRAME_ID_HIGH;
        FLEXI_PRINTF("IDL %d\n\r", byte);
      break;

      case FLEXI_FRAME_ID_HIGH:
        ((uint8_t *)&inst->info.frameid)[1] = byte;
        inst->headerpos = FLEXI_FRAME_TYPE;
        FLEXI_PRINTF("IDH %d\n\r", byte);
      break;

      case FLEXI_FRAME_TYPE:
        inst->info.frame_type = byte;
        inst->headerpos = FLEXI_FRAME_EVENT;
        FLEXI_PRINTF("FRAMETYPE %d\n\r", byte);
      break;

      case FLEXI_FRAME_EVENT:
        inst->info.event = byte;
        inst->headerpos = FLEXI_FRAME_DATA_LEN;
        FLEXI_PRINTF("EVENT %d\n\r", byte);
      break;

      case FLEXI_FRAME_DATA_LEN:
        if (byte > FLEXIFRAME_MAX_DATA_LEN)
          {
            FLEXI_PRINTF("LEN bigger than maximum data len %d/%d. Ignoring remainder\n\r", byte, FLEXIFRAME_MAX_DATA_LEN);
          }

        inst->info.data_len = byte;
        inst->headerpos = FLEXI_FRAME_INV_DATA_LEN;
        FLEXI_PRINTF("LEN %d\n\r", byte);
      break;

      case FLEXI_FRAME_INV_DATA_LEN:
        {
          FLEXI_PRINTF("INVLEN %d (inverted %d)\n\r", byte, (uint8_t)~byte);
          if (inst->info.data_len != (uint8_t)~byte)
            {
              FLEXI_PRINTF("Length did not match inverted length\n\r");
              inst->headerpos = FLEXI_FRAME_START;
              break;
            }
          inst->headerpos = FLEXI_FRAME_PAYLOAD;
        }
      break;
      
      case FLEXI_FRAME_PAYLOAD:
        if (inst->datapos < FLEXIFRAME_MAX_DATA_LEN) {
          inst->rxpayload[inst->datapos] = byte;
        }
        inst->datapos++;        
        // inst->frame.data[inst->datapos] = byte;
        // if (inst->datapos < FLEXIFRAME_MAX_DATA_LEN) inst->datapos++;

        FLEXI_PRINTF("PAYLOAD %d (%d / %d)\n\r", byte, inst->datapos, inst->info.data_len);

        if (inst->datapos >= inst->info.data_len )
          {
            inst->datapos = 0;
            inst->headerpos = FLEXI_FRAME_CHECKSUM;
            FLEXI_PRINTF("End of payload\n\r");
          }
      break;

      case FLEXI_FRAME_CHECKSUM:
        {
          FLEXI_PRINTF("CHECKSUM %d (should be %d)\n\r", byte, (uint8_t)(inst->sum));
          if (byte != (uint8_t)(inst->sum))
            {
              inst->headerpos = FLEXI_FRAME_START;
              FLEXI_PRINTF("Incorrect checksum\n\r\n\r");
              break;
            }

          inst->headerpos = FLEXI_FRAME_START;

          FLEXI_PRINTF("Publishing\n\r\n\r");
          flexi_publish(inst, &inst->info);
        }
      break;

      default:
        FLEXI_PRINTF("Unknown parser state\n\r");
      break;
    }

  inst->sum += byte;
  return inst->state;
}

int flexi_allocate_frame(struct flexi_instance_s *inst,
                         uint8_t **frame_alloc, size_t *alloc_len,
                         enum flexi_frame_type_e frame_type, uint8_t event,
                         const uint8_t *data, size_t data_len)
{

  if ((*frame_alloc) != NULL)
    {
      FLEXI_PRINTF("frame allocation was not freed or is not NULL\n\r");
      return -2;
    }

  // if (frame_type == FLEXI_TYPE_COMMAND)
  //   inst->last_id++;

  /* Allocation size is data length + header before payload + checksum */

  size_t len = data_len + FLEXI_FRAME_PAYLOAD + 1;
  (*frame_alloc) = (uint8_t *)malloc(len);
  if ((*frame_alloc) == NULL)
    {
      FLEXI_PRINTF("flexi malloc fail\n\r");
      return -1;
    }
  FLEXI_PRINTF("Allocated %d\n\r", len);

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
      FLEXI_PRINTF("frame allocation was already freed\n\r");
      return -2;
    }
  free((*frame_alloc));
  (*frame_alloc) = NULL;

  return 0;
}


int flexi_create_static_frame(struct flexi_instance_s *inst,
                         enum flexi_frame_type_e frame_type, uint8_t event,
                         const uint8_t *data, size_t data_len)
{

  /* size is data length + header before payload + checksum */

  inst->txlen = data_len + FLEXI_FRAME_PAYLOAD + 1;
  
  size_t i = 0;
  inst->txbuf[i++] = FLEXI_MAGIC_START;
  inst->txbuf[i++] = ((uint8_t *)&inst->last_id)[0];
  inst->txbuf[i++] = ((uint8_t *)&inst->last_id)[1];
  inst->txbuf[i++] = frame_type;
  inst->txbuf[i++] = event;
  inst->txbuf[i++] = data_len;
  inst->txbuf[i++] = (uint8_t)~data_len;
  for (size_t i_data = 0; i_data < data_len; i_data++)
    inst->txbuf[i++] = data[i_data];

  uint8_t sum = 0;
  for (size_t sumi = 0; sumi < i; sumi++)
      sum += inst->txbuf[sumi];

  inst->txbuf[i++] = sum;

  return inst->last_id;
}

void flexi_set_tx_cb(struct flexi_instance_s *inst, flexi_tx_cb cb)
{
  inst->tx_cb = cb;
}

int flexi_send(struct flexi_instance_s *inst, uint16_t frame_id, enum flexi_frame_type_e type, uint8_t event, const uint8_t *data, size_t data_len)
{
  inst->last_id = frame_id;
  flexi_create_static_frame(inst, type, event, data, data_len);
  return inst->tx_cb(inst, inst->txbuf, inst->txlen);
}

struct flexi_event_s *flexi_get_event(struct flexi_instance_s *inst, int id)
{
    for (size_t i = 0; i < FLEXIFRAME_MAX_EVENTS; i++)
    {
      if (inst->events[i].listener_id != id ) continue;
      return &inst->events[i];
    }
  return NULL;
}