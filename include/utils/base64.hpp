#pragma once

#include <string>
#include <algorithm>

#include "hex.hpp"
#include "binary.hpp"

namespace base64
{

static const auto base64_alphabet = std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/");

////////////////////////////////////////////////////////////
std::string encode(const std::string &input)
{
  std::size_t len = input.size();

  std::string output;
  output.reserve(len * (4/3)); // Base64 encoding turns 3 bytes into 4, so the resulting data is 1 1/3 times the size of the input

  // Get a uint8_t pointer to the data
  auto data = reinterpret_cast<const uint8_t *>(input.data());

  // Loop through the input 3 bytes at a time
  for (std::size_t i = 0; i < len; i += 3)
  {
    // Each 3 bytes is treated as one 24 bit number
    // Use the first available byte, then check to make sure there is a second and third byte left in the input data
    uint32_t n = (*data++) << 16;

    // Is there a second byte available?
    if (i + 1 < len)
    {
      n += (*data++) << 8;
    }
    
    // Is there a third?
    if (i + 2 < len)
    {
      n += *data++;
    }

    // Then the 24 bit number is split into 4 x 8 bit numbers
    uint8_t a = ((n >> 18) & 63);
    uint8_t b = static_cast<uint8_t>((n >> 12) & 63);
    uint8_t c = static_cast<uint8_t>((n >> 6) & 63);
    uint8_t d = static_cast<uint8_t>(n & 63);    

    // Finally, use these 4 numbers to look up the corresponding base64 character
    output.push_back(base64_alphabet[a]);
    output.push_back(base64_alphabet[b]);

    if (i + 1 < len)
    {
      output.push_back(base64_alphabet[c]);
    }

    if (i + 2 < len)
    {
      output.push_back(base64_alphabet[d]);
    }
  }

  // Input length should be a multiple of 3 - if not, pad with 1 or 2 '=' chars
  int pad = len % 3;
  switch (pad)
  {
    case 1:
      output += std::string(2, '=');
      break;
    case 2:
      output += std::string(1, '=');
      break;
  }

  return output;
}


////////////////////////////////////////////////////////////
std::string decode(const std::string& input)
{
  std::size_t len = input.size();

  // Fail point - must contain at least two chars, as valid base64 encoding always results in at least two chars
  if (len < 2)
  {
    LOG_ERROR("Input is too short for valid base64! Must have at least 2 chars!");
    return std::string{};
  }

  // Fail point - must contain valid base64 chars
  auto e = input.find_first_not_of(base64_alphabet + "=");
  if (e != std::string::npos)
  {
    LOG_ERROR("Invalid base64 char '" << input[e] << "' at index " << e << "!");
    return std::string{};
  }

  // We want to ignore any padding, so check if it's present. If it is, we'll stop our base64 decoding loop at that point, otherwise we'll go until the end
  auto end = std::find(std::begin(input), std::end(input), '=');
  
  std::string output;
  output.reserve(len * (3/4)); // Base64 decoding turns 4 bytes into 3, so the resulting data is 3/4 times the size of the input

  // Lambda to perform conversion from a given base64 character to the index within the base64 alphabet for that character
  // TODO: This approach will need changing if custom base64 alphabets are to be supported
  auto b64_to_uint8_t = [](const uint8_t n) -> uint8_t
  {
    // Is it an uppercase char?
    if (n >= 0x41 && n <= 0x5a)
    {
      return n - 0x41;
    }
    // ...or is it a lowercase char?
    else if (n >= 0x61 && n <= 0x7a)
    {
      return n - 0x61 + 26;
    }
    // ...or is it a numeric char?
    else if (n >= 0x30 && n <= 0x39)
    {
      return n - 0x30 + 52;
    }
    // ...or is it a '+' char?
    else if (n == 0x2b)
    {
      return 0x3e;
    }
    // ...or is it a '/' char?
    else if (n == 0x2f)
    {
      return 0x3f;
    }
    // ...or is it an invalid base64 char? Let's use 0xFF to signal this, although earlier checks mean this shouldn't happen
    else
    {
      return 0xFF;
    }
    
  };

  // We'll iterate through the input grabbing 4 bytes at a time, unless only 3 or 2 bytes remain at the end in which case we handle those slightly differently
  auto iter = input.begin();

  while (iter < end)
  {
    // We should always have at least 2 bytes to play with, otherwise this can't be valid base64 encoded data
    if (end - iter == 1)
    {
      LOG_ERROR("There's only one byte left!");
      return std::string{};
    }

    // How many bytes are left? 2, 3, or 4+
    auto remaining = (end - iter > 4) ? 4 : end - iter;
    switch (remaining)
    {
      case 4:
      {
        // Take the current block of 4 x base64 chars and look up their positions in the base64 alphabet
        uint8_t a = b64_to_uint8_t(*iter++);
        uint8_t b = b64_to_uint8_t(*iter++);
        uint8_t c = b64_to_uint8_t(*iter++);
        uint8_t d = b64_to_uint8_t(*iter++);

        /*
        LOG_INFO("a: " << base64_alphabet[a] << " is at base64 alphabet index: " << int(a) << " == " << hex::encode(a));
        LOG_INFO("b: " << base64_alphabet[b] << " is at base64 alphabet index: " << int(b) << " == " << hex::encode(b));
        LOG_INFO("c: " << base64_alphabet[c] << " is at base64 alphabet index: " << int(c) << " == " << hex::encode(c));
        LOG_INFO("d: " << base64_alphabet[d] << " is at base64 alphabet index: " << int(d) << " == " << hex::encode(d));

        LOG_INFO("a == " << hex::encode(a) << " == " << binary::encode(a) << " -->> static_cast<uint32_t>(a) << 18 == " << hex::encode(static_cast<uint32_t>(a) << 18) << " == " << binary::encode(static_cast<uint32_t>(a) << 18));
        LOG_INFO("b == " << hex::encode(b) << " == " << binary::encode(b) << " -->> static_cast<uint32_t>(b) << 12 == " << hex::encode(static_cast<uint32_t>(b) << 12) << " == " << binary::encode(static_cast<uint32_t>(b) << 12));
        LOG_INFO("c == " << hex::encode(c) << " == " << binary::encode(c) << " -->> static_cast<uint32_t>(c) << 6  == " << hex::encode(static_cast<uint32_t>(c) << 6) << " == " << binary::encode(static_cast<uint32_t>(c) << 6));
        LOG_INFO("d == " << hex::encode(d) << " == " << binary::encode(d) << " -->> static_cast<uint32_t>(d)       == " << hex::encode(static_cast<uint32_t>(d)) << " == " << binary::encode(static_cast<uint32_t>(d)));
        */

        // Take the 4 x 8 bit numbers from the base64 alphabet index and turn back into 1 x 24 bit number
        uint32_t n = static_cast<uint32_t>(a) << 18 | static_cast<uint32_t>(b) << 12 | static_cast<uint32_t>(c) << 6 | static_cast<uint32_t>(d);

        //LOG_INFO("n == " << hex::encode(n) << " == " << binary::encode(n));

        // Then split the 24 bit number back into 3 bytes
        auto d1 = static_cast<uint8_t>(n >> 16);
        auto d2 = static_cast<uint8_t>((n >> 8) & 0xFF);
        auto d3 = static_cast<uint8_t>(n & 0xFF);

        /*
        LOG_INFO("d1: " << hex::encode(d1));
        LOG_INFO("d2: " << hex::encode(d2));
        LOG_INFO("d3: " << hex::encode(d3));
        */

        // Finally, add these 3 bytes of decoded data to the output
        output.push_back(d1);
        output.push_back(d2);
        output.push_back(d3);

        break;
      }
      case 3:
      {
        // Take the current block of 3 x base64 chars and look up their positions in the base64 alphabet
        uint8_t a = b64_to_uint8_t(*iter++);
        uint8_t b = b64_to_uint8_t(*iter++);
        uint8_t c = b64_to_uint8_t(*iter++);

        /*
        LOG_INFO("a: " << base64_alphabet[a] << " is at base64 alphabet index: " << int(a) << " == " << hex::encode(a));
        LOG_INFO("b: " << base64_alphabet[b] << " is at base64 alphabet index: " << int(b) << " == " << hex::encode(b));
        LOG_INFO("c: " << base64_alphabet[c] << " is at base64 alphabet index: " << int(c) << " == " << hex::encode(c));

        LOG_INFO("a == " << hex::encode(a) << " == " << binary::encode(a) << " -->> static_cast<uint32_t>(a) << 18 == " << hex::encode(static_cast<uint32_t>(a) << 16) << " == " << binary::encode(static_cast<uint32_t>(a) << 16));
        LOG_INFO("b == " << hex::encode(b) << " == " << binary::encode(b) << " -->> static_cast<uint32_t>(b) << 12 == " << hex::encode(static_cast<uint32_t>(b) << 10) << " == " << binary::encode(static_cast<uint32_t>(b) << 10));
        LOG_INFO("c == " << hex::encode(c) << " == " << binary::encode(c) << " -->> static_cast<uint32_t>(c) << 6  == " << hex::encode(static_cast<uint32_t>(c) << 2) << " == " << binary::encode(static_cast<uint32_t>(c) << 2));
        */

        // Take the 3 x 8 bit numbers from the base64 alphabet index and turn back into 1 x 24 bit number
        uint32_t n = static_cast<uint32_t>(a) << 18 | static_cast<uint32_t>(b) << 12 | static_cast<uint32_t>(c) << 6;

        //LOG_INFO("n == " << hex::encode(n) << " == " << binary::encode(n));

        // Then split the 24 bit number back into 2 bytes
        auto d1 = static_cast<uint8_t>(n >> 16);
        auto d2 = static_cast<uint8_t>((n >> 8) & 0xFF);

        /*
        LOG_INFO("d1: " << hex::encode(d1));
        LOG_INFO("d2: " << hex::encode(d2));
        */

        // Finally, add these 2 bytes of decoded data to the output
        output.push_back(d1);
        output.push_back(d2);

        break;
      }
      case 2:
      {
        // Take the current block of 2 x base64 chars and look up their positions in the base64 alphabet
        uint8_t a = b64_to_uint8_t(*iter++);
        uint8_t b = b64_to_uint8_t(*iter++);

        /*
        LOG_INFO("a: " << base64_alphabet[a] << " is at base64 alphabet index: " << int(a) << " == " << hex::encode(a));
        LOG_INFO("b: " << base64_alphabet[b] << " is at base64 alphabet index: " << int(b) << " == " << hex::encode(b));

        LOG_INFO("a == " << hex::encode(a) << " == " << binary::encode(a) << " -->> static_cast<uint32_t>(a) << 18 == " << hex::encode(static_cast<uint32_t>(a) << 16) << " == " << binary::encode(static_cast<uint32_t>(a) << 16));
        LOG_INFO("b == " << hex::encode(b) << " == " << binary::encode(b) << " -->> static_cast<uint32_t>(b) << 12 == " << hex::encode(static_cast<uint32_t>(b) << 10) << " == " << binary::encode(static_cast<uint32_t>(b) << 10));
        */

        // Take the 3 x 8 bit numbers from the base64 alphabet index and turn back into 1 x 24 bit number
        uint32_t n = static_cast<uint32_t>(a) << 18 | static_cast<uint32_t>(b) << 12;

        //LOG_INFO("n == " << hex::encode(n) << " == " << binary::encode(n));

        // Then split the 24 bit number back into 2 bytes
        auto d1 = static_cast<uint8_t>(n >> 16);

        //LOG_INFO("d1: " << hex::encode(d1));

        // Finally, add this 1 byte of decoded data to the output
        output.push_back(d1);

        break;
      }
      default:
        LOG_ERROR("Not enough data left!: " << remaining);
        return std::string{};
    }
  }

  return output;
}

} // namespace base64