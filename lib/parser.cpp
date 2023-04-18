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

#include "parser.h"

#include <optional>

#include "absl/log/check.h"
#include "absl/strings/str_format.h"

namespace wasmtoolbox {

auto Wasm_parser::prime() -> void {
  cur_byte = is_->get();
  cur_offset = 0;
}

auto Wasm_parser::skip_bytes(std::streamsize count) -> void {
  if (count <= 0) { return; }
  
  auto offset = cur_offset;
  is_->ignore(count - 1);
  if (is_->eof()) {
    throw std::logic_error(absl::StrFormat(
        "Unexpected end of file when skipping %d bytes from offset %d", count, offset));
  }
  cur_byte = is_->get();
  cur_offset += count;
}

// 5.1 Conventions
// ===============

// 5.1.3 Vectors
// -------------

auto Wasm_parser::parse_vec(std::invocable<uint32_t /*i*/> auto element_parser)
    -> std::vector<decltype(element_parser(0))> {
  using Elem_type = decltype(element_parser(0));
  auto result = std::vector<Elem_type>{};
  
  auto n = parse_u32();
  result.reserve(n);
  CHECK_NE(n, std::numeric_limits<uint32_t>::max()) << "would overflow loop!";
  
  for (auto i = uint32_t{0}; i != n; ++i) {
    result.push_back(std::invoke(element_parser, i));
  }

  return result;
}


// 5.2 Values
// ==========

// 5.2.1 Bytes
// -----------

auto Wasm_parser::parse_byte() -> uint8_t {
  if (is_->eof()) {
    throw std::logic_error(absl::StrFormat(
        "Unexpected end of file at offset %d", cur_offset));
  }
  auto result = cur_byte;
  cur_byte = static_cast<uint8_t>(is_->get());
  ++cur_offset;
  return result;
}

auto Wasm_parser::match_byte(uint8_t expected) -> void {
  auto offset = cur_offset;
  auto actual = parse_byte();
  if (actual != expected) {
    throw std::logic_error(absl::StrFormat(
        "Expected byte 0x%02x at offset %d, found 0x%02x instead", expected, offset, actual));
  }
}

auto Wasm_parser::maybe_match_byte(uint8_t probe) -> bool {
  if (cur_byte == probe) {
    (void) parse_byte();
    return true;
  } else {
    return false;
  }
}

// 5.2.2 Integers
// --------------

auto Wasm_parser::parse_u8() -> uint8_t {
  return static_cast<uint8_t>(internal_parse_uN(8));
}

auto Wasm_parser::parse_u16() -> uint16_t {
  return static_cast<uint16_t>(internal_parse_uN(16));
}

auto Wasm_parser::parse_u32() -> uint32_t {
  return static_cast<uint32_t>(internal_parse_uN(32));
}

auto Wasm_parser::internal_parse_uN(int N) -> uint64_t {
  DCHECK_GE(N, 0);
  DCHECK_LE(N, 64);
  auto offset = cur_offset;
  auto result = uint64_t{0};
  auto N_now = N;
  auto shift = 0;
  while (true) {
    auto n = parse_byte();
    result |= (n & 0x7f) << shift;
    if ((n & 0x80) == 0) {
      // High bit unset => end of number
      if (N_now < 8 && n >= (1 << N_now)) {
        throw std::logic_error(absl::StrFormat(
            "Invalid encoding of u%d at offset %d: more than %d bits in encoded by trailing byte",
            N, offset, N));
      }
      break;
    } else {
      // High bit set => rest of number follows
      if (N_now <= 7) {
        throw std::logic_error(absl::StrFormat(
            "Invalid enconding of u%d at offset %d: more than %d bits in encoded by middle byte",
            N, offset, N));
      }
      shift += 7;
      N_now -= 7;
    }
  }
  return result;
}

auto Wasm_parser::parse_s8() -> int8_t {
  return static_cast<int8_t>(internal_parse_sN(8));
}

auto Wasm_parser::parse_s16() -> int16_t {
  return static_cast<int16_t>(internal_parse_sN(16));
}

auto Wasm_parser::parse_s33() -> int64_t {
  return static_cast<int64_t>(internal_parse_sN(33));
}

auto Wasm_parser::internal_parse_sN(int N) -> int64_t {
  DCHECK_GE(N, 0);
  DCHECK_LE(N, 64);
  auto offset = cur_offset;
  auto result = int64_t{0};
  auto N_now = N;
  auto shift = 0;
  while (true) {
    auto n = parse_byte();
    if ((n & 0x80) == 0) {
      // High bit unset => end of number
      if ((n & 0x40) == 0) {  // it's a positive number
        if (N_now < 8 && n >= (1 << (N_now - 1))) {
          throw std::logic_error(absl::StrFormat(
              "Invalid encoding of s%d at offset %d: more than %d bits in encoded by trailing byte",
              N, offset, N));
        }
        result |= (n & 0x3f) << shift;
      } else {  // it's a negative number
        if (N_now < 8 && n < ((1 << 7) - (1 << (N_now - 1)))) {
          throw std::logic_error(absl::StrFormat(
              "Invalid encoding of s%d at offset %d: more than %d bits in encoded by trailing byte",
              N, offset, N));
        }
        result |= (int64_t{n} - int64_t{0x80}) << shift;
      }
      break;
    } else {
      // High bit set => rest of number follows
      if (N_now <= 7) {
        throw std::logic_error(absl::StrFormat(
            "Invalid enconding of s%d at offset %d: more than %d bits in encoded by middle byte",
            N, offset, N));
      }
      result |= (n & 0x7f) << shift;
      shift += 7;
      N_now -= 7;
    }
  }
  return result;
}

auto Wasm_parser::parse_i32() -> int32_t {
  return static_cast<int32_t>(internal_parse_sN(32));
}

auto Wasm_parser::parse_i64() -> int64_t {
  return static_cast<int64_t>(internal_parse_sN(64));
}

// 5.2.3 Floating-Point
// --------------------

auto Wasm_parser::parse_f32() -> float {
  auto bits = uint32_t{0};
  bits |= parse_byte() <<  0;
  bits |= parse_byte() <<  8;
  bits |= parse_byte() << 16;
  bits |= parse_byte() << 24;
  return std::bit_cast<float>(bits);
}

auto Wasm_parser::parse_f64() -> double {
  auto bits = uint64_t{0};
  bits |= uint64_t{parse_byte()} <<  0;
  bits |= uint64_t{parse_byte()} <<  8;
  bits |= uint64_t{parse_byte()} << 16;
  bits |= uint64_t{parse_byte()} << 24;
  bits |= uint64_t{parse_byte()} << 32;
  bits |= uint64_t{parse_byte()} << 40;
  bits |= uint64_t{parse_byte()} << 48;
  bits |= uint64_t{parse_byte()} << 56;
  return std::bit_cast<double>(bits);
}

// 5.2.4 Names
// -----------

auto Wasm_parser::parse_name() -> std::string {
  // Don't use parse_vec to avoid creating a std::vector<char> followed by a copy
  auto n = parse_u32();
  auto result = std::string{};
  result.reserve(n);
  CHECK_NE(n, std::numeric_limits<uint32_t>::max()) << "would overflow loop!";
  for (auto i = uint32_t{0}; i != n; ++i) {
    result.push_back(parse_byte());
  }

  return result;
}

// 5.3 Types
// =========

// 5.3.1 Number Types
// ------------------

auto Wasm_parser::parse_numtype() -> Ast_numtype {
  auto b_offset = cur_offset;
  auto b = parse_byte();
  switch (b) {
    case 0x7F: return k_numtype_i32;
    case 0x7E: return k_numtype_i64;
    case 0x7D: return k_numtype_f32;
    case 0x7C: return k_numtype_f64;
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized numtype 0x%02x at offset %d", b, b_offset));
  }
}

// 5.3.2 Vector Types
// ------------------

auto Wasm_parser::parse_vectype() -> Ast_vectype {
  auto b_offset = cur_offset;
  auto b = parse_byte();
  switch (b) {
    case 0x7B: return k_vectype_v128;
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized vectype 0x%02x at offset %d", b, b_offset));
  }
}

// 5.3.3 Reference Types
// ---------------------

auto Wasm_parser::parse_reftype() -> Ast_reftype {
  auto b_offset = cur_offset;
  auto b = parse_byte();
  switch (b) {
    case 0x70: return k_reftype_funcref;
    case 0x6F: return k_reftype_externref;
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized reftype 0x%02x at offset %d", b, b_offset));
  }
}

// 5.3.4 Value Types
// -----------------

auto Wasm_parser::can_parse_valtype() -> bool {
  switch (cur_byte) {
    case 0x7F:
    case 0x7E:
    case 0x7D:
    case 0x7C:
    case 0x7B:
    case 0x70:
    case 0x6F:
      return true;
    default:
      return false;
  }
}

auto Wasm_parser::parse_valtype() -> Ast_valtype {
  switch (cur_byte) {
    case 0x7F:
    case 0x7E:
    case 0x7D:
    case 0x7C:
      return parse_numtype();
    case 0x7B:
      return parse_vectype();
    case 0x70:
    case 0x6F:
      return parse_reftype();
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized valtype 0x%02x at offset %d", cur_byte, cur_offset));
  }
}

// 5.3.5 Result Types
// ------------------

auto Wasm_parser::parse_resulttype() -> Ast_resulttype {
  return parse_vec([&](auto /*i*/) {
    return parse_valtype();
  });
}

// 5.3.6 Function Types
// --------------------

auto Wasm_parser::parse_functype() -> Ast_functype {
  match_byte(0x60);
  auto params = parse_resulttype();
  auto results = parse_resulttype();
  return Ast_functype{
    .params = std::move(params),
    .results = std::move(results)
  };
}

// 5.3.7 Limits
// ------------
auto Wasm_parser::parse_limits() -> void {
  // Including thread extensions
  auto b_offset = cur_offset;
  auto b = parse_byte();
  switch (b) {
    case 0x00:     // unshared, min-only
      parse_u32();  // n
      break;
    case 0x01:     // unshared, min-max
      parse_u32();  // n
      parse_u32();  // m
      break;
    case 0x02:     // shared, min-only
      parse_u32();  // n
      break;
    case 0x03:     // shared, min-max
      parse_u32();  // n
      parse_u32();  // m
      break;
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized limits flags 0x%02x at offset %d", b, b_offset));
  }
}

// 5.3.8 Memory Types
// ------------------
auto Wasm_parser::parse_memtype() -> void {
  parse_limits(); // lim
}

// 5.3.9 Table Types
// -----------------

auto Wasm_parser::parse_tabletype() -> void {
  parse_reftype(); // et
  parse_limits();  // lim
}

// 5.3.10 Global Types
// -------------------

auto Wasm_parser::parse_globaltype() -> void {
  parse_valtype(); // t
  parse_mut();
}

auto Wasm_parser::parse_mut() -> void {
  auto b_offset = cur_offset;
  auto b = parse_byte();
  switch (b) {
    case 0x00: return;  // const
    case 0x01: return;  // var
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized mut type 0x%02x at offset %d", b, b_offset));
  }
}

// [EXTRA] Tag Types (5.3.11 in the Exception Handling Spec)

auto Wasm_parser::parse_tagtype() -> void {
  match_byte(0x00);
  parse_functype();  // f
}


// 5.4 Instructions
// ================

// <instr> is defined over several subsections...
auto Wasm_parser::parse_instr() -> void {
  // Instructions added here as needed for parsing
  auto opcode_offset = cur_offset;
  auto opcode = parse_byte();
  switch (opcode) {

    // 5.4.1 Control Instructions
    case k_instr_unreachable: break;
    case k_instr_nop: break;
    case k_instr_block: {
      parse_blocktype();
      while (cur_byte != k_instr_end) {
        parse_instr();
      }
      match_byte(k_instr_end);
      break;
    }
    case k_instr_loop: {
      parse_blocktype();
      while (cur_byte != k_instr_end) {
        parse_instr();
      }
      match_byte(k_instr_end);
      break;
    }
    case k_instr_if: {
      parse_blocktype();
      while (cur_byte != k_instr_else && cur_byte != k_instr_end) {
        parse_instr();
      }
      if (cur_byte == k_instr_else) {
        match_byte(k_instr_else);
        while (cur_byte != k_instr_end) {
          parse_instr();
        }
      }
      match_byte(k_instr_end);
      break;
    }
    case k_instr_try: {
      parse_blocktype();
      while (cur_byte != k_instr_catch &&
             cur_byte != k_instr_catch_all &&
             cur_byte != k_instr_delegate &&
             cur_byte != k_instr_end) {
        parse_instr();
      }
      if (cur_byte == k_instr_delegate) {
        // try-delegate
        match_byte(k_instr_delegate);
        parse_labelidx();
      } else {
        // try-catch
        while (cur_byte == k_instr_catch) {
          match_byte(k_instr_catch);
          parse_tagidx();
          while (cur_byte != k_instr_catch && cur_byte != k_instr_catch_all && cur_byte != k_instr_end) {
            parse_instr();
          }
        }
        while (cur_byte == k_instr_catch_all) {
          match_byte(k_instr_catch_all);
          while (cur_byte != k_instr_catch_all && cur_byte != k_instr_end) {
            parse_instr();
          }
        }
        match_byte(k_instr_end);
      }
      break;
    }
    case k_instr_throw: parse_tagidx(); break;
    case k_instr_rethrow: parse_labelidx(); break;
    case k_instr_br: parse_labelidx(); break;
    case k_instr_br_if: parse_labelidx(); break;
    case k_instr_br_table: {
      parse_vec([&](auto /*i*/) {
        parse_labelidx();
        return Ast_TODO{};
      });
      parse_labelidx();
      break;
    }
    case k_instr_return: break;
    case k_instr_call: parse_funcidx(); break;
    case k_instr_call_indirect: {
      parse_typeidx();  // y
      parse_tableidx(); // x
      break;
    }

      // 5.4.3 Parametric Instructions
    case k_instr_drop: break;
    case k_instr_select: break;
      
      // 5.4.4 Variable Instructions
    case k_instr_local_get:    parse_localidx(); break;
    case k_instr_local_set:    parse_localidx(); break;
    case k_instr_local_tee:    parse_localidx(); break;
    case k_instr_global_get:   parse_globalidx(); break;
    case k_instr_global_set:   parse_globalidx(); break;

      // 5.4.6 Memory Instructions
    case k_instr_i32_load:     parse_memarg(); break;
    case k_instr_i64_load:     parse_memarg(); break;
    case k_instr_f32_load:     parse_memarg(); break;
    case k_instr_f64_load:     parse_memarg(); break;
    case k_instr_i32_load8_s:  parse_memarg(); break;
    case k_instr_i32_load8_u:  parse_memarg(); break;
    case k_instr_i32_load16_s: parse_memarg(); break;
    case k_instr_i32_load16_u: parse_memarg(); break;
    case k_instr_i64_load8_s:  parse_memarg(); break;
    case k_instr_i64_load8_u:  parse_memarg(); break;
    case k_instr_i64_load16_s: parse_memarg(); break;
    case k_instr_i64_load16_u: parse_memarg(); break;
    case k_instr_i64_load32_s: parse_memarg(); break;
    case k_instr_i64_load32_u: parse_memarg(); break;
    case k_instr_i32_store:    parse_memarg(); break;
    case k_instr_i64_store:    parse_memarg(); break;
    case k_instr_f32_store:    parse_memarg(); break;
    case k_instr_f64_store:    parse_memarg(); break;
    case k_instr_i32_store8:   parse_memarg(); break;
    case k_instr_i32_store16:  parse_memarg(); break;
    case k_instr_i64_store8:   parse_memarg(); break;
    case k_instr_i64_store16:  parse_memarg(); break;
    case k_instr_i64_store32:  parse_memarg(); break;
    case k_instr_memory_size:  match_byte(0x00); break;
      
      // 5.4.6bis Atomic Memory Instructions (5.4.5 in Threads Spec)
    case k_instr_atomic_prefix: {
      auto opcode2_offset = cur_offset;
      auto opcode2 = parse_u32();
      switch (opcode2) {
        case k_atomic_instr_memory_atomic_notify:      parse_memarg(); break;
        case k_atomic_instr_memory_atomic_wait32:      parse_memarg(); break;
          
        case k_atomic_instr_i32_atomic_load:           parse_memarg(); break;
        case k_atomic_instr_i64_atomic_load:           parse_memarg(); break;
        case k_atomic_instr_i32_atomic_load8:          parse_memarg(); break;
        case k_atomic_instr_i32_atomic_store:          parse_memarg(); break;
        case k_atomic_instr_i64_atomic_store:          parse_memarg(); break;
        case k_atomic_instr_i32_atomic_store8:         parse_memarg(); break;
        
        case k_atomic_instr_i32_atomic_rmw_add:        parse_memarg(); break;
          
        case k_atomic_instr_i32_atomic_rmw_sub:        parse_memarg(); break;

        case k_atomic_instr_i32_atomic_rmw_or:         parse_memarg(); break;

        case k_atomic_instr_i32_atomic_rmw_xchg:       parse_memarg(); break;
        case k_atomic_instr_i32_atomic_rmw8_xchg_u:    parse_memarg(); break;
          
        case k_atomic_instr_i32_atomic_rmw_cmpxchg:    parse_memarg(); break;
        case k_atomic_instr_i32_atomic_rmw8_cmpxchg_u: parse_memarg(); break;
          
        default:
          throw std::logic_error(absl::StrFormat(
              "Unrecognized atomic memory instruction secondary opcode 0x%02x at offset %d", opcode2, opcode2_offset));
      }
      break;
    }
      
      // 5.4.7 Numeric instructions
    case k_instr_i32_const:  parse_i32();  break;
    case k_instr_i64_const:  parse_i64();  break;
    case k_instr_f32_const:  parse_f32();  break;
    case k_instr_f64_const:  parse_f64();  break;
      
    case k_instr_i32_eqz: break;
    case k_instr_i32_eq: break;
    case k_instr_i32_ne: break;
    case k_instr_i32_lt_s: break;
    case k_instr_i32_lt_u: break;
    case k_instr_i32_gt_s: break;
    case k_instr_i32_gt_u: break;
    case k_instr_i32_le_s: break;
    case k_instr_i32_le_u: break;
    case k_instr_i32_ge_s: break;
    case k_instr_i32_ge_u: break;

    case k_instr_i64_eqz: break;
    case k_instr_i64_eq: break;
    case k_instr_i64_ne: break;
    case k_instr_i64_lt_s: break;
    case k_instr_i64_lt_u: break;
    case k_instr_i64_gt_s: break;
    case k_instr_i64_gt_u: break;
    case k_instr_i64_le_s: break;
    case k_instr_i64_le_u: break;
    case k_instr_i64_ge_s: break;
    case k_instr_i64_ge_u: break;

    case k_instr_f64_eq: break;
    case k_instr_f64_ne: break;
    case k_instr_f64_lt: break;
    case k_instr_f64_gt: break;
    case k_instr_f64_le: break;
    case k_instr_f64_ge: break;
      
    case k_instr_i32_clz: break;
    case k_instr_i32_ctz: break;
    case k_instr_i32_add: break;
    case k_instr_i32_sub: break;
    case k_instr_i32_mul: break;
    case k_instr_i32_div_s: break;
    case k_instr_i32_div_u: break;
    case k_instr_i32_rem_s: break;
    case k_instr_i32_rem_u: break;
    case k_instr_i32_and: break;
    case k_instr_i32_or: break;
    case k_instr_i32_xor: break;
    case k_instr_i32_shl: break;
    case k_instr_i32_shr_s: break;
    case k_instr_i32_shr_u: break;
    case k_instr_i32_rotl: break;
      
    case k_instr_i64_clz: break;
    case k_instr_i64_ctz: break;
    case k_instr_i64_add: break;
    case k_instr_i64_sub: break;
    case k_instr_i64_mul: break;
    case k_instr_i64_div_s: break;
    case k_instr_i64_div_u: break;
    case k_instr_i64_rem_s: break;
    case k_instr_i64_rem_u: break;
    case k_instr_i64_and: break;
    case k_instr_i64_or: break;
    case k_instr_i64_xor: break;
    case k_instr_i64_shl: break;
    case k_instr_i64_shr_s: break;
    case k_instr_i64_shr_u: break;

    case k_instr_f32_mul: break;

    case k_instr_f64_abs: break;
    case k_instr_f64_neg: break;
    case k_instr_f64_ceil: break;
    case k_instr_f64_floor: break;
    case k_instr_f64_sqrt: break;
    case k_instr_f64_add: break;
    case k_instr_f64_sub: break;
    case k_instr_f64_mul: break;
    case k_instr_f64_div: break;
      
    case k_instr_i32_wrap_i64: break;
    case k_instr_i32_trunc_f64_s: break;
    case k_instr_i32_trunc_f64_u: break;
    case k_instr_i64_extend_i32_s: break;
    case k_instr_i64_extend_i32_u: break;
    case k_instr_i64_trunc_f64_s: break;
    case k_instr_i64_trunc_f64_u: break;
    case k_instr_f32_convert_i32_s: break;
    case k_instr_f32_demote_f64: break;
    case k_instr_f64_convert_i32_s: break;
    case k_instr_f64_convert_i32_u: break;
    case k_instr_f64_convert_i64_s: break;
    case k_instr_f64_convert_i64_u: break;
    case k_instr_f64_promote_f32: break;
    case k_instr_i32_reinterpret_f32: break;
    case k_instr_i64_reinterpret_f64: break;
    case k_instr_f32_reinterpret_i32: break;
    case k_instr_f64_reinterpret_i64: break;

    case k_instr_i32_extend8_s: break;
    case k_instr_i32_extend16_s: break;
    case k_instr_i64_extend8_s: break;
    case k_instr_i64_extend16_s: break;
    
      // Extended instructions
    case k_instr_ext_prefix: {
      auto opcode2_offset = cur_offset;
      auto opcode2 = parse_u32();
      switch (opcode2) {
        
        // 5.4.6 Memory Instructions
        case k_ext_instr_memory_init:
          parse_dataidx();
          match_byte(0x00);
          break;
        case k_ext_instr_data_drop: parse_dataidx(); break;
        case k_ext_instr_memory_copy:
          match_byte(0x00);
          match_byte(0x00);
          break;
        case k_ext_instr_memory_fill:
          match_byte(0x00);
          break;
          
        default:
          throw std::logic_error(absl::StrFormat(
              "Unrecognized extended instruction secondary opcode %d at offset %d", opcode2, opcode2_offset));
      }
      break;
    }

      
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized instruction opcode 0x%02x at offset %d", opcode, opcode_offset));
  }
}

// 5.4.1 Control Instructions
// --------------------------

auto Wasm_parser::parse_blocktype() -> void {
  if (maybe_match_byte(0x40)) {
    // epsilon
  } else {
    if (can_parse_valtype()) {
      parse_valtype();  // t
    } else {
      parse_s33();
    }
  }
}

// 5.4.6 Memory Instructions
// -------------------------

auto Wasm_parser::parse_memarg() -> void {
  parse_u32(); // a
  parse_u32(); // o
}

// 5.4.9 Expressions
// -----------------

auto Wasm_parser::parse_expr() -> void {
  while (cur_byte != k_instr_end) {  // opcode for "end"
    parse_instr();
  }
  match_byte(k_instr_end);
}


// 5.5 Modules
// ===========

// 5.5.1 Indices
// -------------

auto Wasm_parser::parse_typeidx() -> void {
  parse_u32();
}

auto Wasm_parser::parse_funcidx() -> void {
  parse_u32();
}

auto Wasm_parser::parse_tableidx() -> void {
  parse_u32();
}

auto Wasm_parser::parse_memidx() -> void {
  parse_u32();
}

auto Wasm_parser::parse_tagidx() -> void {
  parse_u32();
}

auto Wasm_parser::parse_globalidx() -> void {
  parse_u32();
}

auto Wasm_parser::parse_dataidx() -> void {
  parse_u32();
}

auto Wasm_parser::parse_localidx() -> void {
  parse_u32();
}

auto Wasm_parser::parse_labelidx() -> void {
  parse_u32();
}

// 5.5.2 Sections
// --------------

auto Wasm_parser::parse_section(Section_id section_id, std::invocable<uint32_t /*size*/> auto section_parser)
    -> decltype(section_parser(0)) {
  match_byte(section_id);  // section id
  auto declared_size = parse_u32();
  auto start_offset = cur_offset;

  // TODO: Really need to bound reading within section_parser to declared_size
  // Otherwise, the beginning of a following section may be confused with more bytes from this section
  // (e.g., empty name custom section followed by another custom section, vs
  //  a name custom section with a module name subsection)
  auto result = std::invoke(section_parser, declared_size);
  
  auto end_offset = cur_offset;
  auto actual_size = end_offset - start_offset;
  
  if (actual_size != declared_size) {
    throw std::logic_error(absl::StrFormat(
        "Invalid section id %d in byte range [%d,%d): declared size %d doesn't match actual size %d",
        section_id, start_offset, end_offset, declared_size, actual_size)); 
  }

  return result;
}

// 5.5.3 Custom Section
// --------------------


auto Wasm_parser::parse_namesubsection(
    Name_subsection_id N,
    std::invocable<uint32_t /*size*/> auto subsection_parser)
    -> decltype(subsection_parser(0)) {
  match_byte(N);
  auto size = parse_u32();
  return std::invoke(subsection_parser, size);
}

auto Wasm_parser::parse_customsec(Ast_module& module) -> void {
  parse_section(k_section_custom, [&](auto size) {
    auto start_offset = cur_offset;
    auto end_offset = start_offset + size;
    [[maybe_unused]] auto name = parse_name();
    //std::cerr << "Custom section '" << name << "'\n";
    if (name == "name") {
      // Including additions from extended name section spec
      while (cur_offset < end_offset) {
        auto N_offset = cur_offset;
        auto N = Name_subsection_id{cur_byte};
        switch (N) {
          case k_name_subsection_module:
            module.name = parse_modulenamesubsec();
            break;
          case k_name_subsection_functions:        parse_funcnamesubsec(); break;
          case k_name_subsection_locals:           parse_localnamesubsec(); break;
          case k_name_subsection_globals:          parse_globalnamesubsec(); break;
          case k_name_subsection_data_segments:    parse_datasegmentnamesubsec(); break;
          default:
            parse_namesubsection(N, [&](auto subsection_size) {
              std::cerr << absl::StreamFormat(
                  "Unrecognized namesubsection id %d at offset %d, skipping %d bytes\n", N, N_offset, subsection_size);
              skip_bytes(subsection_size);
            });
            break;
        }
      }
    } else if (name == "sourceMappingURL") {
      auto url = parse_name();
      std::cerr << "Source mapping url: " << url << "\n";
      if (cur_offset != end_offset) {
        std::cerr << "??\n";
        skip_bytes(end_offset - cur_offset);
      }
    } else {
      skip_bytes(size - (cur_offset - start_offset));
    }
    return Ast_TODO{};
  });
}

auto Wasm_parser::parse_modulenamesubsec() -> std::string {
  return parse_namesubsection(k_name_subsection_module, [&](auto /*size*/){
    return parse_name();
  });
}

auto Wasm_parser::parse_funcnamesubsec() -> void {
  parse_namesubsection(k_name_subsection_functions, [&](auto /*size*/){
    //std::cerr << "Function names:\n";
    parse_namemap(false);
  });
}

auto Wasm_parser::parse_localnamesubsec() -> void {
  parse_namesubsection(k_name_subsection_locals, [&](auto /*size*/) {
    //std::cerr << "Local names:\n";
    parse_indirectnamemap(false);
  });
}

auto Wasm_parser::parse_globalnamesubsec() -> void {
  parse_namesubsection(k_name_subsection_globals, [&](auto /*size*/){
    //std::cerr << "Global names:\n";
    parse_namemap(false);
  });
}

auto Wasm_parser::parse_datasegmentnamesubsec() -> void {
  parse_namesubsection(k_name_subsection_data_segments, [&](auto /*size*/){
    //std::cerr << "Data segment names:\n";
    parse_namemap(false);
  });
}

auto Wasm_parser::parse_namemap(bool dump) -> std::vector<Ast_TODO> {
  return parse_vec([&](auto /*i*/) {
    parse_nameassoc(dump);
    return Ast_TODO{};
  });
}

auto Wasm_parser::parse_nameassoc(bool dump) -> void {
  auto idx = parse_u32();
  auto name = parse_name();
  if (dump) {
    std::cerr << absl::StreamFormat("- %d -> %s\n", idx, name);
  }
}

auto Wasm_parser::parse_indirectnamemap(bool dump) -> std::vector<Ast_TODO> {
  return parse_vec([&](auto /*i*/) {
    parse_indirectnameassoc(dump);
    return Ast_TODO{};
  });
}

auto Wasm_parser::parse_indirectnameassoc(bool dump) -> void {
  auto idx = parse_u32();
  if (dump) {
    std::cerr << absl::StreamFormat("[%d]:\n", idx);
  }
  parse_namemap(dump);
}

// 5.5.4 Type Section
// ------------------

auto Wasm_parser::parse_typesec() -> std::vector<Ast_functype> {
  return parse_section(k_section_type, [&](auto /*size*/) {
    return parse_vec([&](auto /*i*/) {
      return parse_functype();
    });
  });
}

// 5.5.5 Import Section
// --------------------

auto Wasm_parser::parse_importsec() -> std::vector<Ast_import> {
  return parse_section(k_section_import, [&](auto /*size*/) {
    return parse_vec([&](auto /*i*/) {
      return parse_import();
    });
  });
}

auto Wasm_parser::parse_import() -> Ast_import {
  auto module = parse_name();
  auto name = parse_name();
  parse_importdesc();
  return Ast_import{
    .module = std::move(module),
    .name = std::move(name)
  };
}

auto Wasm_parser::parse_importdesc() -> void {
  auto b_offset = cur_offset;
  auto b = parse_byte();
  switch (b) {
    case 0x00: return parse_typeidx();     // func
    case 0x01: return parse_tabletype();   // table
    case 0x02: return parse_memtype();     // mem
    case 0x03: return parse_globaltype();  // global
    case 0x04: return parse_tag();         // tag
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized importdesc type 0x%02x at offset %d", b, b_offset));
  }
}

// 5.5.6 Function Section
// ----------------------

auto Wasm_parser::parse_funcsec() -> std::vector<Ast_TODO> {
  return parse_section(k_section_function, [&](auto /*size*/) {
    return parse_vec([&](auto /*i*/) {
      parse_typeidx();
      return Ast_TODO{};
    });
  });
}

// 5.5.7 Table Section
// -------------------

auto Wasm_parser::parse_tablesec() -> std::vector<Ast_TODO> {
  return parse_section(k_section_table, [&](auto /*size*/) {
    return parse_vec([&](auto /*i*/) {
      parse_table();
      return Ast_TODO{};
    });
  });
}

auto Wasm_parser::parse_table() -> void {
  parse_tabletype();
}

// 5.5.8 Memory Section
// --------------------

auto Wasm_parser::parse_memsec() -> std::vector<Ast_TODO> {
  return parse_section(k_section_memory, [&](auto /*size*/) {
    return parse_vec([&](auto /*i*/) {
      parse_mem();
      return Ast_TODO{};
    });
  });
}

auto Wasm_parser::parse_mem() -> void {
  parse_memtype();
}

// 5.5.9 Global Section
// --------------------

auto Wasm_parser::parse_globalsec() -> std::vector<Ast_TODO> {
  return parse_section(k_section_global, [&](auto /*size*/) {
    return parse_vec([&](auto /*i*/) {
      parse_global();
      return Ast_TODO{};
    });
  });
}

auto Wasm_parser::parse_global() -> void {
  parse_globaltype();  // gt
  parse_expr();  // e
}

// 5.5.10 Export Section
// ---------------------

auto Wasm_parser::parse_exportsec() -> std::vector<Ast_TODO> {
  return parse_section(k_section_export, [&](auto /*size*/) {
    return parse_vec([&](auto /*i*/) {
      parse_export();
      return Ast_TODO{};
    });
  });
}

auto Wasm_parser::parse_export() -> void {
  parse_name(); // nm
  parse_exportdesc(); // d
}

auto Wasm_parser::parse_exportdesc() -> void {
  // Including additions from the exception handling spec
  auto b_offset = cur_offset;
  auto b = parse_byte();
  switch (b) {
    case 0x00: return parse_funcidx();     // func
    case 0x01: return parse_tableidx();    // table
    case 0x02: return parse_memidx();      // mem
    case 0x03: return parse_globalidx();   // global
    case 0x04: return parse_tagidx();      // tag
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized exportdesc type 0x%02x at offset %d", b, b_offset));
  }
}

// 5.5.11 Start Section
// --------------------

auto Wasm_parser::parse_startsec() -> void {
  parse_section(k_section_start, [&](auto /*size*/) {
    parse_start();
    return Ast_TODO{};
  });
}

auto Wasm_parser::parse_start() -> void {
  parse_funcidx();
}

// 5.5.12 Element Section
// ----------------------

auto Wasm_parser::parse_elemsec() -> std::vector<Ast_TODO> {
  return parse_section(k_section_element, [&](auto /*size*/) {
    return parse_vec([&](auto /*i*/) {
      parse_elem();
      return Ast_TODO{};
    });
  });
}

auto Wasm_parser::parse_elem() -> void {
  auto discriminant_offset = cur_offset;
  auto discriminant = parse_u32();
  switch (discriminant) {
    case 0:
      parse_expr();
      parse_vec([&](auto /*i*/) {
        parse_funcidx();
        return Ast_TODO{};
      });
      break;

      // TODO: Add parsing for other discriminants
      
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized elem discriminant %d at offset %d", discriminant, discriminant_offset));
  }
}

// 5.5.13 Code Section
// -------------------

auto Wasm_parser::parse_codesec() -> std::vector<Ast_TODO> {
  return parse_section(k_section_code, [&](auto /*size*/) {
    return parse_vec([&](auto /*i*/) {
      parse_code();
      return Ast_TODO{};
    });
  });
}

auto Wasm_parser::parse_code() -> void {
  parse_u32();  // size
  parse_func();  // Assume ||func|| == size
}

auto Wasm_parser::parse_func() -> void {
  parse_vec([&](auto /*i*/) {
    parse_locals();
    return Ast_TODO{};
  });
  parse_expr();
}

auto Wasm_parser::parse_locals() -> void {
  parse_u32();  // n
  parse_valtype();  // t
}

// 5.5.14 Data Section
// -------------------

auto Wasm_parser::parse_datasec() -> std::vector<Ast_TODO> {
  return parse_section(k_section_data, [&](auto /*size*/) {
    return parse_vec([&](auto /*i*/) {
      parse_data();
      return Ast_TODO{};
    });
  });
}

auto Wasm_parser::parse_data() -> void {
  auto discriminant_offset = cur_offset;
  auto discriminant = parse_u32();
  switch (discriminant) {
    case 0: // active, implicit memory index 0
      parse_expr();  // e
      parse_vec([&](auto /*i*/) {
        return parse_byte();
      });
      break;
    case 1: // passive
      parse_vec([&](auto /*i*/) {
        return parse_byte();
      });
      break;
    case 2: // active, explicit memory
      parse_memidx();  // x
      parse_expr();  // e
      parse_vec([&](auto /*i*/) {
        return parse_byte();
      });
      break;
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized data discriminant %d at offset %d", discriminant, discriminant_offset));
  }
}

// 5.5.15 Data Count Section
// -------------------------

auto Wasm_parser::parse_datacountsec() -> uint32_t {
  return parse_section(k_section_data_count, [&](auto /*size*/) {
    return parse_u32();  // n
  });
}

// [EXTRA] Tag Section (5.5.16 in Exception Handling Spec)
// -------------------------------------------------------

auto Wasm_parser::parse_tagsec() -> std::vector<Ast_TODO> {
  return parse_section(k_section_tag, [&](auto /*size*/) {
    return parse_vec([&](auto /*i*/) {
      parse_tag();
      return Ast_TODO{};
    });
  });
}

auto Wasm_parser::parse_tag() -> void {
  match_byte(0x00);
  parse_typeidx();  // x
}

// 5.5.16 Modules
// --------------

auto Wasm_parser::parse_magic() -> void {
  match_byte(0x00); match_byte(0x61); match_byte(0x73); match_byte(0x6D);
}

auto Wasm_parser::parse_version() -> void {
  match_byte(0x01); match_byte(0x00); match_byte(0x00); match_byte(0x00);
}

auto Wasm_parser::parse_module() -> Ast_module {
  auto module = Ast_module{};
  auto parse_opt_customsecs = [&]{
    while (not is_->eof() && cur_byte == k_section_custom) { parse_customsec(module); }
  };
  
  parse_magic();
  parse_version();
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_type) {
    module.types = parse_typesec();
  }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_import) {
    module.imports = parse_importsec();
  }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_function) { parse_funcsec(); }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_table) { parse_tablesec(); }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_memory) { parse_memsec(); }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_tag) { parse_tagsec(); }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_global) { parse_globalsec(); }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_export) { parse_exportsec(); }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_start) { parse_startsec(); }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_element) { parse_elemsec(); }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_data_count) { parse_datacountsec(); }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_code) { parse_codesec(); }
  parse_opt_customsecs();
  if (not is_->eof() && cur_byte == k_section_data) { parse_datasec(); }
  parse_opt_customsecs();
  
  if (not is_->eof()) {
    throw std::logic_error(absl::StrFormat(
        "Expected end of file at offset %d, but the data continues: 0x%02x...", cur_offset, cur_byte));
  }

  return module;
}

}  // namespace wasmtoolbox
