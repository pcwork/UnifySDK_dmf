#ifndef ZIGPC_COMMAND_MAPPER_HEX_UTILS_HPP
#define ZIGPC_COMMAND_MAPPER_HEX_UTILS_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <sl_status.h>

sl_status_t zigpc_command_mapper_hex_string_to_bytes(
  const char *hex_string,
  std::vector<uint8_t> &bytes);

std::string zigpc_command_mapper_bytes_to_hex_string(const uint8_t *bytes,
                                                     size_t length);

#endif  // ZIGPC_COMMAND_MAPPER_HEX_UTILS_HPP
