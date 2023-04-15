// Copyright (C) 2023 Patrick Varilly
// patvarilly@gmail.com
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License as published by the Free
// Software Foundation, either version 3 of the License, or (at your option)
// any later version.
//
// This program is distributed in the hope that it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of  MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program.  If not, see <http://www.gnu.org/licenses/>.

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "parser.h"

#include <fstream>
#include <stdexcept>
#include <span>

namespace wasmtoolbox {

// From https://tuttlem.github.io/2014/08/18/getting-istream-to-work-off-a-byte-array.html
class Membuf : public std::basic_streambuf<char> {
 public:
  Membuf(std::span<const uint8_t> bytes) {
    setg((char*)bytes.data(), (char*)bytes.data(), (char*)bytes.data() + bytes.size());
  }
};
class Memstream : public std::istream {
 public:
  Memstream(std::span<const uint8_t> bytes) : std::istream{&buffer_}, buffer_{bytes} {
    rdbuf(&buffer_);
  }

 private:
  Membuf buffer_;
};

TEST(parser, empty) {
  auto bytes = std::vector<uint8_t>{};
  auto is = Memstream{bytes};
  EXPECT_THROW(parse_wasm(is), std::logic_error);
}

TEST(parser, just_magic) {
  auto bytes = std::vector<uint8_t>{0x00, 0x61, 0x73, 0x6D};
  auto is = Memstream{bytes};
  EXPECT_THROW(parse_wasm(is), std::logic_error);
}

TEST(parser, just_magic_and_version) {
  // This is the smallest valid WASM module
  auto bytes = std::vector<uint8_t>{
    0x00, 0x61, 0x73, 0x6D,  // magic
    0x01, 0x00, 0x00, 0x00,  // version
    0x00,                    // Custom section (id = 0)
    0x0d,                    // Size (u32)
    0x04,                    // Custom section name length (4 bytes)
    'n', 'a', 'm', 'e',      // Custom section name "name"
    0x00,                    // Name subsection id (0 = "module name")
    0x06,                    // Name subsection 0 size (u32)
    0x05,                    // Module name length
    'h', 'e', 'l', 'l', 'o'  // Module name
  };
  auto is = Memstream{bytes};
  auto module = parse_wasm(is);  // Should work!

  EXPECT_THAT(module.name.value(), testing::StrEq("hello"));
}

TEST(parser, min_module_with_name) {
  auto bytes = std::vector<uint8_t>{
    0x00, 0x61, 0x73, 0x6D,  // magic
    0x01, 0x00, 0x00, 0x00   // version
  };
  auto is = Memstream{bytes};
  auto module = parse_wasm(is);  // Should work!

  EXPECT_THAT(module.name, testing::Eq(std::nullopt));
}

TEST(parser, u8) {
  auto do_it = [](const std::vector<uint8_t>& bytes) -> uint8_t {
    auto is = Memstream{bytes};
    auto parser = Wasm_parser{is};
    return parser.parse_u8();
  };

  EXPECT_THAT(do_it({0x00}), testing::Eq(0));
  EXPECT_THAT(do_it({0x42}), testing::Eq(0x42));
  EXPECT_THROW(do_it({0x80}), std::logic_error);  // EOF
  EXPECT_THAT(do_it({0x03}), testing::Eq(0x03));
  EXPECT_THAT(do_it({0x83, 0x00}), testing::Eq(0x03));
  EXPECT_THROW(do_it({0x83, 0x10}), std::logic_error);  // Exceeds u8 range in last byte
  EXPECT_THROW(do_it({0x80, 0x88, 0x00}), std::logic_error);  // Exceeds u8 range in middle byte
}

TEST(parser, u16) {
  auto do_it = [](const std::vector<uint8_t>& bytes) -> uint16_t {
    auto is = Memstream{bytes};
    auto parser = Wasm_parser{is};
    return parser.parse_u16();
  };

  EXPECT_THAT(do_it({0x00}), testing::Eq(0));
  EXPECT_THAT(do_it({0x42}), testing::Eq(0x42));
  EXPECT_THROW(do_it({0x80}), std::logic_error);  // EOF
  EXPECT_THAT(do_it({0x03}), testing::Eq(0x03));
  EXPECT_THAT(do_it({0x83, 0x00}), testing::Eq(0x03));
  EXPECT_THAT(do_it({0x83, 0x10}), testing::Eq(0x10 << 7 | 0x03));
  EXPECT_THAT(do_it({0x80, 0x88, 0x00}), testing::Eq(0x08 << 7 | 0x00));
  EXPECT_THROW(do_it({0x80, 0x88}), std::logic_error);  // EOF
  EXPECT_THROW(do_it({0x83, 0x80, 0x10}), std::logic_error);  // Exceeds u16 range in last byte
  EXPECT_THROW(do_it({0x80, 0x80, 0x88, 0x00}), std::logic_error);  // Exceeds u16 range in middle byte
}

TEST(parser, u32) {
  auto do_it = [](const std::vector<uint8_t>& bytes) -> uint32_t {
    auto is = Memstream{bytes};
    auto parser = Wasm_parser{is};
    return parser.parse_u32();
  };

  EXPECT_THAT(do_it({0x00}), testing::Eq(0));
  EXPECT_THAT(do_it({0x42}), testing::Eq(0x42));
  EXPECT_THROW(do_it({0x80}), std::logic_error);  // EOF
  EXPECT_THAT(do_it({0x03}), testing::Eq(0x03));
  EXPECT_THAT(do_it({0x83, 0x00}), testing::Eq(0x03));
  EXPECT_THAT(do_it({0x83, 0x10}), testing::Eq(0x10 << 7 | 0x03));
  EXPECT_THAT(do_it({0x80, 0x88, 0x00}), testing::Eq(0x08 << 7 | 0x00));
  EXPECT_THROW(do_it({0x80, 0x88}), std::logic_error);  // EOF
  EXPECT_THAT(do_it({0xFF, 0xFF, 0xFF, 0xFF, 0x0F}), testing::Eq(0xFFFFFFFF));
  EXPECT_THROW(do_it({0xFF, 0xFF, 0xFF, 0xFF, 0x1F}), std::logic_error);  // Exceeds u32 range in last byte
  EXPECT_THROW(do_it({0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00}), std::logic_error);  // Exceeds u32 range in middle byte
}

TEST(parser, s8) {
  auto do_it = [](const std::vector<uint8_t>& bytes) -> int8_t {
    auto is = Memstream{bytes};
    auto parser = Wasm_parser{is};
    return parser.parse_s8();
  };

  EXPECT_THAT(do_it({0x00}), testing::Eq(0));
  EXPECT_THAT(do_it({0x2e}), testing::Eq(0x2e));
  EXPECT_THAT(do_it({0x7f}), testing::Eq(-1));
  EXPECT_THAT(do_it({0x7e}), testing::Eq(-2));
  EXPECT_THAT(do_it({0xfe, 0x7f}), testing::Eq(-2));
  EXPECT_THROW(do_it({0x80}), std::logic_error);  // EOF
  EXPECT_THROW(do_it({0x80, 0x88}), std::logic_error);  // EOF
  EXPECT_THROW(do_it({0x83, 0x3e}), std::logic_error);  // Exceeds s8 range in last byte (positive)
  EXPECT_THROW(do_it({0xff, 0x7b}), std::logic_error);  // Exceeds s8 range in last byte (negative)
  EXPECT_THROW(do_it({0xff, 0xff, 0x3f}), std::logic_error);  // Exceeds s8 range in middle byte (positive)
  EXPECT_THROW(do_it({0xff, 0xff, 0x7f}), std::logic_error);  // Exceeds s8 range in middle byte (negative)
}

TEST(parser, s16) {
  auto do_it = [](const std::vector<uint8_t>& bytes) -> int16_t {
    auto is = Memstream{bytes};
    auto parser = Wasm_parser{is};
    return parser.parse_s16();
  };

  EXPECT_THAT(do_it({0x00}), testing::Eq(0));
  EXPECT_THAT(do_it({0x2e}), testing::Eq(0x2e));
  EXPECT_THAT(do_it({0x7f}), testing::Eq(-1));
  EXPECT_THAT(do_it({0x7e}), testing::Eq(-2));
  EXPECT_THAT(do_it({0xfe, 0x7f}), testing::Eq(-2));
  EXPECT_THAT(do_it({0xff, 0x3f}), testing::Eq(0x3f << 7 | 0x7f));
  EXPECT_THROW(do_it({0x80}), std::logic_error);  // EOF
  EXPECT_THROW(do_it({0x80, 0x88}), std::logic_error);  // EOF
  EXPECT_THROW(do_it({0xff, 0xff, 0x3f}), std::logic_error);  // Exceeds s16 range in last byte (positive)
  EXPECT_THROW(do_it({0xff, 0xff, 0x7b}), std::logic_error);  // Exceeds s16 range in last byte (negative)
  EXPECT_THROW(do_it({0xff, 0xff, 0xff, 0x3f}), std::logic_error);  // Exceeds s16 range in middle byte (positive)
  EXPECT_THROW(do_it({0xff, 0xff, 0xff, 0x7b}), std::logic_error);  // Exceeds s16 range in middle byte (negative)
}

TEST(parser, f32) {
  auto do_it = [](const std::vector<uint8_t>& bytes) -> float {
    auto is = Memstream{bytes};
    auto parser = Wasm_parser{is};
    return parser.parse_f32();
  };

  // Conversions done with https://gregstoll.com/~gregstoll/floattohex/
  EXPECT_THAT(do_it({0x00, 0x48, 0x2a, 0x44}), testing::Eq(681.125f));
  EXPECT_THAT(do_it({0x00, 0x00, 0x00, 0x00}), testing::Eq(+0.0f));
  EXPECT_THAT(do_it({0x00, 0x00, 0x00, 0x80}), testing::Eq(-0.0f));
  EXPECT_THAT(do_it({0x00, 0x00, 0x80, 0x7f}),
              testing::Eq(+std::numeric_limits<float>::infinity()));
  EXPECT_THAT(do_it({0x00, 0x00, 0x80, 0xff}),
              testing::Eq(-std::numeric_limits<float>::infinity()));
}

TEST(parser, f64) {
  auto do_it = [](const std::vector<uint8_t>& bytes) -> double {
    auto is = Memstream{bytes};
    auto parser = Wasm_parser{is};
    return parser.parse_f64();
  };

  // Conversions done with https://gregstoll.com/~gregstoll/floattohex/
  EXPECT_THAT(do_it({0x00, 0x00, 0x00, 0x00, 0x00, 0x49, 0x85, 0x40}), testing::Eq(681.125));
  EXPECT_THAT(do_it({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}), testing::Eq(+0.0));
  EXPECT_THAT(do_it({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80}), testing::Eq(-0.0));
  EXPECT_THAT(do_it({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0x7f}),
              testing::Eq(+std::numeric_limits<double>::infinity()));
  EXPECT_THAT(do_it({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf0, 0xff}),
              testing::Eq(-std::numeric_limits<double>::infinity()));

  // Examples from https://en.cppreference.com/w/cpp/numeric/bit_cast
  EXPECT_THAT(do_it({0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xe9, 0x3f}), testing::Eq(0.781250));
  EXPECT_THAT(do_it({0x00, 0x00, 0x00, 0xc0, 0x8b, 0xf5, 0x72, 0x41}), testing::Eq(19880124.0));
}

TEST(parser, skip_bytes) {
  auto bytes = std::vector<uint8_t>{0x01, 0x02, 0x03, 0x04};
  auto skip_N_and_parse_byte = [&](int N, bool read = true) -> uint8_t {
    auto is = Memstream{bytes};
    auto parser = Wasm_parser{is};
    parser.skip_bytes(N);
    return read ? parser.parse_byte() : 0;
  };
  EXPECT_THAT(skip_N_and_parse_byte(0), testing::Eq(0x01));
  EXPECT_THAT(skip_N_and_parse_byte(1), testing::Eq(0x02));
  EXPECT_THAT(skip_N_and_parse_byte(2), testing::Eq(0x03));
  EXPECT_THAT(skip_N_and_parse_byte(3), testing::Eq(0x04));
  EXPECT_THROW(skip_N_and_parse_byte(4), std::logic_error);  // EOF when reading "next byte"
  EXPECT_THAT(skip_N_and_parse_byte(4, false), testing::Eq(0x00));
  EXPECT_THROW(skip_N_and_parse_byte(7, false), std::logic_error);  // EOF when skipping bytes
}

TEST(parser, parse_customsec) {
  auto bytes = std::vector<uint8_t>{
    0x00,                    // Custom section id = 0
    0x04,                    // Size = 4
    0x03, 'h', 'i', '!',     // Name
    0xBA};
  auto is = Memstream{bytes};
  auto parser = Wasm_parser{is};
  auto module = Ast_module{};
  
  parser.parse_customsec(module);
  EXPECT_THAT(parser.parse_byte(), testing::Eq(0xBA));
}

// TODO: typesec
// TODO: importsec

}  // namespace wasmtoolbox
