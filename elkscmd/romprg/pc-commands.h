#pragma once

#include <stdint.h>
#include "common/replies.h"

int command_ping(ReplyPing *ping);
int command_identify(ReplyIdentify *identify);
int command_read(uint32_t start_address, uint8_t len, uint8_t *buffer);
int command_crc(uint32_t start_address, uint32_t end_address, ReplyCRC *crc);
int command_write(uint32_t start_address, uint8_t len, uint8_t const* buffer);
int command_erase_sector(uint32_t start_address, uint32_t end_address);
int command_erase_chip();
