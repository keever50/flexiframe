#pragma once

#include <stdio.h>
#include <stdint.h>

#define FLEXIFRAME_MAX_EVENTS 32
#define FLEXIFRAME_MAX_DATA_LEN 0xff
#define FLEXIFRAME_HEADER_LEN   7
#define FLEXIFRAME_OVERHEAD     8
#define FLEXIFRAME_MAX_FRAME_LEN FLEXIFRAME_MAX_DATA_LEN + FLEXIFRAME_OVERHEAD

typedef void (*flexi_event_cb)(struct flexi_instance_s *inst, const struct flexi_event_s *event, const struct flexi_info_s *info, const struct flexi_payload_s *payload);
typedef int (*flexi_tx_cb)(struct flexi_instance_s *inst, const uint8_t *buf, size_t len);

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

struct flexi_payload_s
{
  const uint8_t *data;
  size_t len;
};

struct flexi_info_s
{
  uint16_t frameid;
  uint8_t frame_type;
  uint8_t event;
  uint8_t data_len; /* This does not have to passed to the user. Should be in payload_s */
  uint8_t checksum; /* This does not have to passed to the user */
};

struct flexi_event_s
{
  flexi_event_cb cb;
  uint8_t event_type;
//  enum flexi_frame_type_e frame_type;
  int listener_id;
  void *user_data;
};

struct flexi_instance_s
{
  struct flexi_event_s events[FLEXIFRAME_MAX_EVENTS];
  uint8_t txbuf[FLEXIFRAME_MAX_FRAME_LEN]; /* Maybe allocate this later? */
  size_t txlen;
  uint8_t rxpayload[FLEXIFRAME_MAX_DATA_LEN];
  enum flexi_status_e state;
  int headerpos;
  int datapos;
  struct flexi_info_s info;
  uint16_t last_id;
  uint8_t sum;
  flexi_tx_cb tx_cb;
};


void flexi_init(struct flexi_instance_s *inst);


enum flexi_status_e flexi_feed(struct flexi_instance_s *inst, uint8_t byte);

void flexi_set_tx_cb(struct flexi_instance_s *inst, flexi_tx_cb cb);
int flexi_send(struct flexi_instance_s *inst, uint16_t frame_id, enum flexi_frame_type_e type, uint8_t event, const uint8_t *data, size_t data_len);

int flexi_register_event(struct flexi_instance_s *inst, flexi_event_cb cb, int listener_id, uint8_t event_type, void *user_data);
int flexi_unregister_event(struct flexi_instance_s *inst, int id);

// void flexi_publish(struct flexi_instance_s *inst, const struct flexi_info_s *frame);