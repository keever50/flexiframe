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
  uint8_t start;
  uint16_t frameid;
  uint8_t frame_type;
  uint8_t event;
  uint16_t data_len;
  uint8_t checksum;
  uint8_t data[FLEXIFRAME_MAX_DATA_LEN];
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
  bool ignore_remainder;
  struct flexi_frame_s frame;
};




void flexi_init(struct flexi_instance_s *inst);

enum flexi_status_e flexi_intake(struct flexi_instance_s *inst, uint8_t byte);

int flexi_register_event(struct flexi_instance_s *inst, flexi_event_cb cb, int listener_id, uint8_t event_type, void *user_data);
int flexi_unregister_event(struct flexi_instance_s *inst, int id);

void flexi_publish(struct flexi_instance_s *inst, const struct flexi_frame_s *frame);