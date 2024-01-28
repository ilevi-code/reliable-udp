#include <cstring>

#include "test/utils.h"

const std::span<uint8_t> to_span(const char* s)
{
    return std::span<uint8_t>(reinterpret_cast<uint8_t*>(const_cast<char*>(s)), strlen(s));
}

std::string to_string(const std::vector<uint8_t>& v)
{
    return std::string(reinterpret_cast<const char*>(v.data()), v.size());
}
