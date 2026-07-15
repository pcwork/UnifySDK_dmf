#include "zigpc_command_mapper_hex_utils.hpp"

#include <cctype>
#include <iomanip>
#include <sstream>

namespace
{
uint8_t hex_nibble_to_uint(char nibble)
{
  if ((nibble >= '0') && (nibble <= '9')) {
    return static_cast<uint8_t>(nibble - '0');
  }

  nibble = static_cast<char>(std::toupper(static_cast<unsigned char>(nibble)));
  return static_cast<uint8_t>(nibble - 'A' + 10);
}
}  // namespace

sl_status_t zigpc_command_mapper_hex_string_to_bytes(
  const char *hex_string,
  std::vector<uint8_t> &bytes)
{
  bytes.clear();

  if (hex_string == nullptr) {
    return SL_STATUS_NULL_POINTER;
  }

  std::string input(hex_string);
  if ((input.size() % 2U) != 0U) {
    return SL_STATUS_INVALID_SIGNATURE;
  }

  for (char c: input) {
    if (!std::isxdigit(static_cast<unsigned char>(c))) {
      return SL_STATUS_INVALID_SIGNATURE;
    }
  }

  bytes.reserve(input.size() / 2U);
  for (size_t i = 0; i < input.size(); i += 2U) {
    uint8_t byte = static_cast<uint8_t>(hex_nibble_to_uint(input[i]) << 4U);
    byte |= hex_nibble_to_uint(input[i + 1U]);
    bytes.push_back(byte);
  }

  return SL_STATUS_OK;
}

std::string zigpc_command_mapper_bytes_to_hex_string(const uint8_t *bytes,
                                                     size_t length)
{
  if ((bytes == nullptr) && (length > 0U)) {
    return {};
  }

  std::ostringstream ss;
  ss << std::hex << std::uppercase << std::setfill('0');

  for (size_t i = 0; i < length; ++i) {
    ss << std::setw(2) << static_cast<unsigned int>(bytes[i]);
  }

  return ss.str();
}
