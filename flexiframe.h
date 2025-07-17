#pragma once

#include <stdio.h>
#include <stdint.h>

#define FLEXIFRAME_MAX_EVENTS 32
#define FLEXIFRAME_MAX_DATA_LEN 0xff

typedef void (*flexi_event_cb)(struct flexi_instance_s *inst, const struct flexi_event_s *event, const struct flexi_frame_s *frame);

enum flexi_frame_type_e
{
  FLEXI_TYPE_COMMAND,
  FLEXI_TYPE_RESPONSE,
  FLEXI_TYPE_ERROR
};

enum flexi_status_e
{
  FLEXI_OK,
  FLEXI_PARSING,
  FLEXI_OVERFLOW
};

struct flexi_frame_s
{
  uint16_t frameid;
  uint8_t frame_type;
  uint8_t event;
  uint8_t data_len;
  uint8_t data[FLEXIFRAME_MAX_DATA_LEN];
  uint8_t checksum;
};

struct flexi_event_s
{
  flexi_event_cb cb;
  uint8_t event_type;
  enum flexi_frame_type_e frame_type;
  int listener_id;
  void *user_data;
};

struct flexi_instance_s
{
  struct flexi_event_s events[FLEXIFRAME_MAX_EVENTS];
  enum flexi_status_e state;
  int headerpos;
  int datapos;
  struct flexi_frame_s frame;
  uint16_t last_id;
  uint8_t sum;
};


void flexi_init(struct flexi_instance_s *inst);

enum flexi_status_e flexi_feed(struct flexi_instance_s *inst, uint8_t byte);

int flexi_allocate_frame(struct flexi_instance_s *inst,
                         uint8_t **frame_alloc, size_t *alloc_len,
                         enum flexi_frame_type_e frame_type, uint8_t event,
                         const uint8_t *data, size_t data_len);
int flexi_free(struct flexi_instance_s *inst, uint8_t **frame_alloc);

int flexi_register_event(struct flexi_instance_s *inst, flexi_event_cb cb, int listener_id, uint8_t event_type, void *user_data);
int flexi_unregister_event(struct flexi_instance_s *inst, int id);

void flexi_publish(struct flexi_instance_s *inst, const struct flexi_frame_s *frame);