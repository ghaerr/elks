#include <string.h>
#include "commands.h"
#include "flash.h"
#include "common/protocol.h"
#include "common/commands.h"
#include "common/replies.h"
#include "version.h"
#include "crc.h"

void command_generate_error_reply(uint8_t err) {
  protocol_write_packet(1, &err);
}


void command_ping(unsigned data_size, uint8_t const* cmd) {
  static ReplyPing const reply = {
    ERROR_NONE,
    PROGRAMMER_VERSION_MAJOR,
    PROGRAMMER_VERSION_MINOR,
    PROGRAMMER_ADDRESS_BITS,
    PROGRAMMER_MAX_LEN
  };

  protocol_write_packet(sizeof(reply), (uint8_t const*)&reply);
}


void command_identify(unsigned data_size, uint8_t const* cmd) {
  flash_enter_software_id();
  ReplyIdentify reply = {
    ERROR_NONE,
    flash_read_manufacturer_id(),
    flash_read_device_id()
  };
  flash_leave_software_id();

  protocol_write_packet(sizeof(reply), (uint8_t const*)&reply);
}


void command_read(unsigned data_size, uint8_t const* cmd) {
  if(data_size != sizeof(CommandRead)) {
    command_generate_error_reply(ERROR_COMMAND);
    return;
  }

  CommandRead const *read_command = (CommandRead const*)cmd;

  if(read_command->read_len > PROGRAMMER_MAX_LEN) {
    command_generate_error_reply(ERROR_LENGTH);
    return;
  }

  if(read_command->read_addr + read_command->read_len > PROGRAMMER_ROM_SIZE) {
    command_generate_error_reply(ERROR_ADDR);
    return;
  }

  uint8_t  reply[read_command->read_len+1];
  reply[0] = ERROR_NONE;
  for(uint8_t idx = 0; idx < read_command->read_len; ++idx) {
    reply[1+idx] = flash_read(read_command->read_addr + idx);
  }

  protocol_write_packet(read_command->read_len+1, reply);
}

void command_crc(unsigned data_size, uint8_t const* cmd) {
  if(data_size != sizeof(CommandCRC)) {
    command_generate_error_reply(ERROR_COMMAND);
    return;
  }

  CommandCRC const *crc_command = (CommandCRC const*)cmd;

  if(crc_command->start_addr >= PROGRAMMER_ROM_SIZE
      || crc_command->end_addr > PROGRAMMER_ROM_SIZE
      || crc_command->end_addr <= crc_command->start_addr) {

    command_generate_error_reply(ERROR_ADDR);
    return;
  }

  ReplyCRC reply = { ERROR_NONE };
  reply.crc = 0;
  for(uint32_t addr = crc_command->start_addr; addr < crc_command->end_addr; ++addr) {
    reply.crc = _crc_ccitt_update(reply.crc, flash_read(addr));
  }

  protocol_write_packet(sizeof(reply), (uint8_t const*)&reply);
}


void command_write(unsigned data_size, uint8_t const* cmd) {

  if(data_size < sizeof(CommandWrite)+1) {
    command_generate_error_reply(ERROR_LENGTH);
    return;
  }

  uint8_t write_len = data_size - sizeof(CommandWrite);

  CommandWrite const *write_command = (CommandWrite const*)cmd;


  if(write_command->write_addr + write_len > PROGRAMMER_ROM_SIZE) {
    command_generate_error_reply(ERROR_ADDR);
    return;
  }

  for(uint8_t idx = 0; idx < write_len; ++idx) {
    flash_write_byte(write_command->write_addr + idx, write_command->data[idx]);
  }

  command_generate_error_reply(ERROR_NONE);
}


void command_erase_sector(unsigned data_size, uint8_t const* cmd) {
  if(data_size != sizeof(CommandEraseSector)) {
    command_generate_error_reply(ERROR_COMMAND);
    return;
  }

  CommandEraseSector const *erase_command = (CommandEraseSector const*)cmd;

  if(erase_command->start_addr >= PROGRAMMER_ROM_SIZE
      || erase_command->end_addr > PROGRAMMER_ROM_SIZE
      || erase_command->end_addr <= erase_command->start_addr) {

    command_generate_error_reply(ERROR_ADDR);
    return;
  }

  if((erase_command->start_addr & (PROGRAMMER_SECTOR_SIZE-1)) != 0
      || (erase_command->end_addr & (PROGRAMMER_SECTOR_SIZE-1)) != 0) {
    command_generate_error_reply(ERROR_ADDR);
    return;
  }

  for(uint32_t addr = erase_command->start_addr; addr < erase_command->end_addr; addr += PROGRAMMER_SECTOR_SIZE) {
    flash_erase_sector(addr);
  }

  command_generate_error_reply(ERROR_NONE);
}


void command_erase_chip(unsigned data_size, uint8_t const* cmd) {
  flash_erase_chip();

  command_generate_error_reply(ERROR_NONE);
}


typedef void (*CommandFunc)(unsigned data_len, uint8_t const*);
#define ENTRY(x) command_ ## x,
static CommandFunc const known_commands[] = {
  COMMAND_LIST
};
#undef ENTRY


void command_execute(uint8_t const *buffer) {
  CommandHeader const *command = (CommandHeader const*)buffer;
  if(command->command_id >= CMD_COUNT) {
    command_generate_error_reply(ERROR_COMMAND);
  }

  known_commands[command->command_id](command->packet_length - sizeof(CommandHeader), command->data);
}
