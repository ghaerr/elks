#pragma once

#include <stdint.h>

void command_generate_error_reply(uint8_t err);
void command_execute(uint8_t const* buffer);
