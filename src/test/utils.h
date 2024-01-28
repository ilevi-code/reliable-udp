#include <cstdint>
#include <span>
#include <string>
#include <vector>

template<size_t N>
const std::span<uint8_t> to_span(const char (&s)[N])
{
    return std::span<uint8_t>(reinterpret_cast<uint8_t*>(const_cast<char*>(s)), N - 1);
}

const std::span<uint8_t> to_span(const char* s);

std::string to_string(const std::vector<uint8_t>& v);
