#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <algorithm>
#include <numeric>
#include <bitset>
#include <sstream>

#include "exceptions.hpp"

namespace hmr::binary
{

////////////////////////////////////////////////////////////
auto encode(std::string_view input, bool delimited = true) noexcept -> std::string;


////////////////////////////////////////////////////////////
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
auto encode(T input, bool delimited = true) noexcept -> std::string
{
  if (delimited)
  {
    // How many bytes is the integral value?
    std::size_t const s = sizeof(input);

    auto output = std::string{};
    output.reserve(s + (s - 1));

    // There's probably a smarter way to do this, but for now just manually code in options for 1, 2, 4 and 8 byte integrals
    switch (s)
    {
      case 1: {
        auto bits = std::bitset<8>(input);
        return bits.to_string();
      }
      case 2: {
        uint8_t a = (input & 0xFF00) >> 8;
        uint8_t b = (input & 0x00FF);

        auto bits = std::bitset<8>(a);
        output += bits.to_string();

        output.push_back(' ');

        bits = std::bitset<8>(b);
        output += bits.to_string();
        return output;
      }
      case 4: {
        uint8_t a = (input & 0xFF000000) >> 24;
        uint8_t b = (input & 0x00FF0000) >> 16;
        uint8_t c = (input & 0x0000FF00) >> 8;
        uint8_t d = (input & 0x000000FF);

        auto bits = std::bitset<8>(a);
        output += bits.to_string();

        output.push_back(' ');

        bits = std::bitset<8>(b);
        output += bits.to_string();

        output.push_back(' ');

        bits = std::bitset<8>(c);
        output += bits.to_string();

        output.push_back(' ');

        bits = std::bitset<8>(d);
        output += bits.to_string();

        return output;
      }
      case 8: {
        uint8_t a = (input & 0xFF00000000000000) >> 56;
        uint8_t b = (input & 0x00FF000000000000) >> 48;
        uint8_t c = (input & 0x0000FF0000000000) >> 40;
        uint8_t d = (input & 0x000000FF00000000) >> 32;
        uint8_t e = (input & 0x00000000FF000000) >> 24;
        uint8_t f = (input & 0x0000000000FF0000) >> 16;
        uint8_t g = (input & 0x000000000000FF00) >> 8;
        uint8_t h = (input & 0x00000000000000FF);

        auto bits = std::bitset<8>(a);
        output += bits.to_string();

        output.push_back(' ');

        bits = std::bitset<8>(b);
        output += bits.to_string();

        output.push_back(' ');

        bits = std::bitset<8>(c);
        output += bits.to_string();

        output.push_back(' ');

        bits = std::bitset<8>(d);
        output += bits.to_string();

        output.push_back(' ');

        bits = std::bitset<8>(e);
        output += bits.to_string();

        output.push_back(' ');

        bits = std::bitset<8>(f);
        output += bits.to_string();

        output.push_back(' ');

        bits = std::bitset<8>(g);
        output += bits.to_string();

        output.push_back(' ');

        bits = std::bitset<8>(h);
        output += bits.to_string();

        return output;
      }
      default: {
        return std::string{};
      }
    }

  } else
  {
    auto b = std::bitset<sizeof(T) * 8>(input);

    return b.to_string();
  }
}


////////////////////////////////////////////////////////////
auto decode(std::string const &input) -> std::string;


////////////////////////////////////////////////////////////
template<typename T, typename = std::enable_if_t<std::is_integral_v<T>>>
auto decode(std::string const &input) -> T
{
  // Strip any spaces
  std::string tmp = input;
  tmp.erase(std::remove(std::begin(tmp), std::end(tmp), ' '), std::end(tmp));

  // Input binary string must be divisible by 8
  if (tmp.size() % 8 != 0)
  {
    throw hmr::xcpt::binary::invalid_input("Input length not divisible by 8!");
  }

  // Input binary string must contain the correct number of bits to fit into the desired return type
  // TODO: For inputs that are shorter than the desired return type, maybe we could just convert to that type anyway, e.g. an input of "11111111" could be turned into a uint16_t output with the value 0x00FF ??
  if ((tmp.size() / 8) != sizeof(T))
  {
    auto ss = std::stringstream{};
    ss << "Input binary string does not contain the correct number of bits to fit into a " << sizeof(T) << " byte type!";
    throw hmr::xcpt::binary::invalid_input(ss.str()); // TODO: Create a better exception name for this scenario
  }

  // Input binary string must only contain 1s and 0s
  if (tmp.find_first_not_of("10") != std::string::npos)
  {
    throw hmr::xcpt::binary::invalid_input("Invalid binary char in input!");
  }

  auto const bits = std::bitset<sizeof(T) * 8>(tmp);

  auto const output = static_cast<T>(bits.to_ullong());

  return output;
}

} // namespace hmr::binary
