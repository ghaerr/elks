#include <string.h>
#include "common/commands.h"
#include "common/replies.h"
#include "common/protocol.h"
#include "pc-commands.h"


int command_ping(ReplyPing *ping) {
  uint8_t cmd = CMD_ping;
  protocol_write_packet(sizeof(cmd), &cmd);
  uint8_t buffer[sizeof(ReplyPing) + 3];
  uint8_t err = protocol_read_packet(sizeof(buffer), buffer);
  if(err != ERROR_NONE) {
    return err | ERROR_LOCAL;
  }

  *ping = *(ReplyPing const*)(buffer+1);
  return ping->error_code;
}


int command_identify(ReplyIdentify *identify) {
  uint8_t cmd = CMD_identify;
  protocol_write_packet(sizeof(cmd), &cmd);
  uint8_t buffer[sizeof(ReplyIdentify) + 3];
  uint8_t err = protocol_read_packet(sizeof(buffer), buffer);
  if(err != ERROR_NONE) {
    return err | ERROR_LOCAL;
  }
  
  *identify = *(ReplyIdentify const*)(buffer+1);
  return identify->error_code;
}


int command_read(uint32_t start_address, uint8_t len, uint8_t *buffer) {
  struct {
    uint8_t cmd_id;
    CommandRead data;
  } __attribute__((packed)) cmd = {
    .cmd_id = CMD_read,
    .data = {
      .read_addr = start_address,
      .read_len  = len
    }
  };
  protocol_write_packet(sizeof(cmd), (uint8_t const*)&cmd);
  uint8_t reply[len + 4];
  int err = protocol_read_packet(sizeof(reply), reply);
  if(err != ERROR_NONE) {
    return err | ERROR_LOCAL;
  }

  if(reply[1] != ERROR_NONE) {
    return reply[1];
  }

  memcpy(buffer, reply+2, len);

  return ERROR_NONE;
}


int command_crc(uint32_t start_address, uint32_t end_address, ReplyCRC *crc) {
  struct {
    uint8_t cmd_id;
    CommandCRC data;
  } __attribute__((packed)) cmd = {
    .cmd_id = CMD_crc,
    .data = {
      .start_addr = start_address,
      .end_addr   = end_address
    }
  };

  protocol_write_packet(sizeof(cmd), (uint8_t const*)&cmd);

  struct {
    uint8_t len;
    ReplyCRC crc;
    uint16_t pkt_crc;
  } __attribute__((packed)) reply;

  int err = protocol_read_packet(sizeof(reply), (uint8_t*)&reply);
  if(err != ERROR_NONE) {
    return err | ERROR_LOCAL;
  }

  if(reply.crc.error_code != ERROR_NONE) {
    return reply.crc.error_code;
  }

  *crc = reply.crc;

  return ERROR_NONE;
}


int command_write(uint32_t start_address, uint8_t len, uint8_t const* buffer) {
  struct {
    uint8_t  cmd_id;
    uint32_t write_addr;
    uint8_t  data[len];
  } __attribute__((packed)) cmd;

  cmd.cmd_id = CMD_write;
  cmd.write_addr = start_address;
  memcpy(cmd.data, buffer, len);

  protocol_write_packet(sizeof(cmd), (uint8_t const*)&cmd);

  uint8_t reply[4];
  int err = protocol_read_packet(sizeof(reply), (uint8_t*)&reply);
  if(err != ERROR_NONE) {
    return err | ERROR_LOCAL;
  }

  return reply[1];
}


int command_erase_sector(uint32_t start_address, uint32_t end_address) {
  struct {
    uint8_t  cmd_id;
    uint32_t start_addr;
    uint32_t  end_addr;
  } __attribute__((packed)) cmd = {
    .cmd_id     = CMD_erase_sector,
    .start_addr = start_address,
    .end_addr   = end_address
  };

  protocol_write_packet(sizeof(cmd), (uint8_t const*)&cmd);
  uint8_t buffer[4];
  int err = protocol_read_packet(sizeof(buffer), buffer);
  if(err != ERROR_NONE) {
    return err | ERROR_LOCAL;
  }
  return buffer[1];
}


int command_erase_chip() {
  uint8_t cmd = CMD_erase_chip;
  protocol_write_packet(sizeof(cmd), &cmd);
  uint8_t buffer[4];
  int err = protocol_read_packet(sizeof(buffer), buffer);
  if(err != ERROR_NONE) {
    return err | ERROR_LOCAL;
  }
  return buffer[1];
}
