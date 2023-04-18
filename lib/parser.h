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

#ifndef WASMTOOLBOX_PARSER_H
#define WASMTOOLBOX_PARSER_H

#include <iostream>

#include "ast.h"

namespace wasmtoolbox {

// The structure of the parser closely follows the structure of the WebAssembly spec + a few extensions:
//
// - WebAssembly Core Spec 2.0 (Draft 2023-04-08):
//     https://webassembly.github.io/spec/core/
// - Threads extension (1.1 Draft Nov 20, 2020):
//     https://webassembly.github.io/threads/core/
// - Exception handling spec: (2.0 Draft 2023-04-12 + exception handling Draft, Apr 12, 2023)
//     https://webassembly.github.io/exception-handling/core/
// - Extended name section (1.0, Draft Jul 08, 2019):
//     https://www.scheidecker.net/2019-07-08-extended-name-section-spec/appendix/custom.html

// 5.5.2 Sections
enum Section_id : uint8_t {
  k_section_custom     =  0,
  k_section_type       =  1,
  k_section_import     =  2,
  k_section_function   =  3,
  k_section_table      =  4,
  k_section_memory     =  5,
  k_section_global     =  6,
  k_section_export     =  7,
  k_section_start      =  8,
  k_section_element    =  9,
  k_section_code       = 10,
  k_section_data       = 11,
  k_section_data_count = 12,

  // From Exception Handling spec
  k_section_tag        = 13
};

// 7.4.1 Name section
enum Name_subsection_id : uint8_t {
  k_name_subsection_module        = 0,
  k_name_subsection_functions     = 1,
  k_name_subsection_locals        = 2,
  k_name_subsection_globals       = 7,
  k_name_subsection_data_segments = 9
};

// 5.4 Instructions
enum Instr_opcode : uint8_t {
  k_instr_ext_prefix          = 0xfc,  // used by some Table Instructions, Memory Instructions and Numeric Instructions
  
  // 5.4.1 Control Instructions
  k_instr_unreachable         = 0x00,
  k_instr_nop                 = 0x01,
  k_instr_block               = 0x02,
  k_instr_loop                = 0x03,
  k_instr_if                  = 0x04,
  k_instr_else                = 0x05,
  k_instr_try                 = 0x06,
  k_instr_catch               = 0x07,
  k_instr_throw               = 0x08,
  k_instr_rethrow             = 0x09,
  k_instr_end                 = 0x0b,
  k_instr_br                  = 0x0c,
  k_instr_br_if               = 0x0d,
  k_instr_br_table            = 0x0e,
  k_instr_return              = 0x0f,
  k_instr_call                = 0x10,
  k_instr_call_indirect       = 0x11,
  k_instr_delegate            = 0x18,
  k_instr_catch_all           = 0x19,

  // 5.4.3 Parametric Instructions
  k_instr_drop                = 0x1a,
  k_instr_select              = 0x1b,
  
  // 5.4.4 Variable instructions
  k_instr_local_get           = 0x20,
  k_instr_local_set           = 0x21,
  k_instr_local_tee           = 0x22,
  k_instr_global_get          = 0x23,
  k_instr_global_set          = 0x24,

  // 5.4.6 Memory Instructions
  k_instr_i32_load            = 0x28,
  k_instr_i64_load            = 0x29,
  k_instr_f32_load            = 0x2a,
  k_instr_f64_load            = 0x2b,
  k_instr_i32_load8_s         = 0x2c,
  k_instr_i32_load8_u         = 0x2d,
  k_instr_i32_load16_s        = 0x2e,
  k_instr_i32_load16_u        = 0x2f,
  k_instr_i64_load8_s         = 0x30,
  k_instr_i64_load8_u         = 0x31,
  k_instr_i64_load16_s        = 0x32,
  k_instr_i64_load16_u        = 0x33,
  k_instr_i64_load32_s        = 0x34,
  k_instr_i64_load32_u        = 0x35,
  k_instr_i32_store           = 0x36,
  k_instr_i64_store           = 0x37,
  k_instr_f32_store           = 0x38,
  k_instr_f64_store           = 0x39,
  k_instr_i32_store8          = 0x3a,
  k_instr_i32_store16         = 0x3b,
  k_instr_i64_store8          = 0x3c,
  k_instr_i64_store16         = 0x3d,
  k_instr_i64_store32         = 0x3e,
  k_instr_memory_size         = 0x3f,

  // 5.4.6bis Atomic Memory Instructions (5.4.4 in Threads spec)
  k_instr_atomic_prefix       = 0xfe,

  // 5.4.7 Numeric instructions
  k_instr_i32_const           = 0x41,
  k_instr_i64_const           = 0x42,
  k_instr_f32_const           = 0x43,
  k_instr_f64_const           = 0x44,
  
  k_instr_i32_eqz             = 0x45,
  k_instr_i32_eq              = 0x46,
  k_instr_i32_ne              = 0x47,
  k_instr_i32_lt_s            = 0x48,
  k_instr_i32_lt_u            = 0x49,
  k_instr_i32_gt_s            = 0x4a,
  k_instr_i32_gt_u            = 0x4b,
  k_instr_i32_le_s            = 0x4c,
  k_instr_i32_le_u            = 0x4d,
  k_instr_i32_ge_s            = 0x4e,
  k_instr_i32_ge_u            = 0x4f,

  k_instr_i64_eqz             = 0x50,
  k_instr_i64_eq              = 0x51,
  k_instr_i64_ne              = 0x52,
  k_instr_i64_lt_s            = 0x53,
  k_instr_i64_lt_u            = 0x54,
  k_instr_i64_gt_s            = 0x55,
  k_instr_i64_gt_u            = 0x56,
  k_instr_i64_le_s            = 0x57,
  k_instr_i64_le_u            = 0x58,
  k_instr_i64_ge_s            = 0x59,
  k_instr_i64_ge_u            = 0x5a,
  
  k_instr_f64_eq              = 0x61,
  k_instr_f64_ne              = 0x62,
  k_instr_f64_lt              = 0x63,
  k_instr_f64_gt              = 0x64,
  k_instr_f64_le              = 0x65,
  k_instr_f64_ge              = 0x66,
  
  k_instr_i32_clz             = 0x67,
  k_instr_i32_ctz             = 0x68,
  k_instr_i32_add             = 0x6a,
  k_instr_i32_sub             = 0x6b,
  k_instr_i32_mul             = 0x6c,
  k_instr_i32_div_s           = 0x6d,
  k_instr_i32_div_u           = 0x6e,
  k_instr_i32_rem_s           = 0x6f,
  k_instr_i32_rem_u           = 0x70,
  k_instr_i32_and             = 0x71,
  k_instr_i32_or              = 0x72,
  k_instr_i32_xor             = 0x73,
  k_instr_i32_shl             = 0x74,
  k_instr_i32_shr_s           = 0x75,
  k_instr_i32_shr_u           = 0x76,
  k_instr_i32_rotl            = 0x77,

  k_instr_i64_clz             = 0x79,
  k_instr_i64_ctz             = 0x7a,
  k_instr_i64_add             = 0x7c,
  k_instr_i64_sub             = 0x7d,
  k_instr_i64_mul             = 0x7e,
  k_instr_i64_div_s           = 0x7f,
  k_instr_i64_div_u           = 0x80,
  k_instr_i64_rem_s           = 0x81,
  k_instr_i64_rem_u           = 0x82,
  k_instr_i64_and             = 0x83,
  k_instr_i64_or              = 0x84,
  k_instr_i64_xor             = 0x85,
  k_instr_i64_shl             = 0x86,
  k_instr_i64_shr_s           = 0x87,
  k_instr_i64_shr_u           = 0x88,

  k_instr_f32_mul             = 0x94,

  k_instr_f64_abs             = 0x99,
  k_instr_f64_neg             = 0x9a,
  k_instr_f64_ceil            = 0x9b,
  k_instr_f64_floor           = 0x9c,
  k_instr_f64_sqrt            = 0x9f,
  k_instr_f64_add             = 0xa0,
  k_instr_f64_sub             = 0xa1,
  k_instr_f64_mul             = 0xa2,
  k_instr_f64_div             = 0xa3,

  k_instr_i32_wrap_i64        = 0xa7,
  k_instr_i32_trunc_f64_s     = 0xaa,
  k_instr_i32_trunc_f64_u     = 0xab,
  k_instr_i64_extend_i32_s    = 0xac,
  k_instr_i64_extend_i32_u    = 0xad,
  k_instr_i64_trunc_f64_s     = 0xb0,
  k_instr_i64_trunc_f64_u     = 0xb1,
  k_instr_f32_convert_i32_s   = 0xb2,
  k_instr_f32_demote_f64      = 0xb6,
  k_instr_f64_convert_i32_s   = 0xb7,
  k_instr_f64_convert_i32_u   = 0xb8,
  k_instr_f64_convert_i64_s   = 0xb9,
  k_instr_f64_convert_i64_u   = 0xba,
  k_instr_f64_promote_f32     = 0xbb,
  k_instr_i32_reinterpret_f32 = 0xbc,
  k_instr_i64_reinterpret_f64 = 0xbd,
  k_instr_f32_reinterpret_i32 = 0xbe,
  k_instr_f64_reinterpret_i64 = 0xbf,

  k_instr_i32_extend8_s       = 0xc0,
  k_instr_i32_extend16_s      = 0xc1,
  k_instr_i64_extend8_s       = 0xc2,
  k_instr_i64_extend16_s      = 0xc3
};

// Extended instruction Secondary Opcodes (follow a k_instr_ext_prefix opcode = 0xfc)
enum Ext_instr_ : uint32_t {
  // 5.4.6 Memory Instructions
  k_ext_instr_memory_init =  8,
  k_ext_instr_data_drop   =  9,
  k_ext_instr_memory_copy = 10,
  k_ext_instr_memory_fill = 11
};

// Thread Secondary Opcodes (follow a k_instr_atomic_prefix opcode = 0xfe)
enum Thread_instr_secondary_opcode : uint8_t {
  // 5.4.6bis Atomic Memory Instructions (5.4.4 in Threads spec)
  k_atomic_instr_memory_atomic_notify      = 0x00,
  k_atomic_instr_memory_atomic_wait32      = 0x01,

  k_atomic_instr_i32_atomic_load           = 0x10,
  k_atomic_instr_i64_atomic_load           = 0x11,
  k_atomic_instr_i32_atomic_load8          = 0x12,
  k_atomic_instr_i32_atomic_store          = 0x17,
  k_atomic_instr_i64_atomic_store          = 0x18,
  k_atomic_instr_i32_atomic_store8         = 0x19,
  k_atomic_instr_i32_atomic_rmw_add        = 0x1e,

  k_atomic_instr_i32_atomic_rmw_sub        = 0x25,
  
  k_atomic_instr_i32_atomic_rmw_or         = 0x33,
  
  k_atomic_instr_i32_atomic_rmw_xchg       = 0x41,
  k_atomic_instr_i32_atomic_rmw8_xchg_u    = 0x43,
  
  k_atomic_instr_i32_atomic_rmw_cmpxchg    = 0x48,
  k_atomic_instr_i32_atomic_rmw8_cmpxchg_u = 0x4a
};

struct Wasm_parser {
  std::istream* is_;
  uint8_t cur_byte;  // only valid if is_->eof() is false
  long cur_offset;

  explicit Wasm_parser(std::istream& is) : is_{&is} { prime(); }

  auto prime() -> void;
  auto skip_bytes(std::streamsize count) -> void;


  // 5.1 Conventions
  // ===============

  // 5.1.3 Vectors
  auto parse_vec(std::invocable<uint32_t /*i*/> auto element_parser)
      -> std::vector<decltype(element_parser(0))>;

  
  // 5.2 Values
  // ==========
  
  // 5.2.1 Bytes
  auto parse_byte() -> uint8_t;
  auto match_byte(uint8_t expected) -> void;
  auto maybe_match_byte(uint8_t probe) -> bool;

  // 5.2.2 Integers
  auto parse_u8() -> uint8_t;
  auto parse_u16() -> uint16_t;
  auto parse_u32() -> uint32_t;
  auto internal_parse_uN(int N) -> uint64_t;
  auto parse_s8() -> int8_t;
  auto parse_s16() -> int16_t;
  auto parse_s33() -> int64_t;
  auto internal_parse_sN(int N) -> int64_t;
  auto parse_i32() -> int32_t;
  auto parse_i64() -> int64_t;

  // 5.2.3 Floating-Point
  auto parse_f32() -> float;
  auto parse_f64() -> double;

  // 5.2.4 Names
  auto parse_name() -> std::string;

  
  // 5.3 Types
  // =========

  // 5.3.1 Number Types
  auto parse_numtype() -> Ast_numtype;

  // 5.3.2 Vector Types
  auto parse_vectype() -> Ast_vectype;

  // 5.3.3 Reference Types
  auto parse_reftype() -> Ast_reftype;

  // 5.3.4 Value Types
  auto can_parse_valtype() -> bool;
  auto parse_valtype() -> Ast_valtype;

  // 5.3.5 Result Types
  auto parse_resulttype() -> Ast_resulttype;
  
  // 5.3.6 Function Types
  auto parse_functype() -> Ast_functype;

  // 5.3.7 Limit Types
  auto parse_limits() -> void;
  
  // 5.3.8 Memory Types
  auto parse_memtype() -> void;
  
  // 5.3.9 Table Types
  auto parse_tabletype() -> void;
  
  // 5.3.10 Global Types
  auto parse_globaltype() -> void;
  auto parse_mut() -> void;

  // [EXTRA] Tag Types (5.3.11 in Exception Handling Spec)
  auto parse_tagtype() -> void;
  

  // 5.4 Instructions
  // ================

  // <instr> is defined over several subsections...
  auto parse_instr() -> void;

  // 5.4.1 Control Instructions
  auto parse_blocktype() -> void;

  // 5.4.4 Memory Instructions
  auto parse_memarg() -> void;
  
  // 5.4.9 Expressions
  auto parse_expr() -> void;
  
  // 5.5 Modules
  // ===========

  // 5.5.1 Indices
  auto parse_typeidx() -> void;
  auto parse_funcidx() -> void;
  auto parse_tableidx() -> void;
  auto parse_memidx() -> void;
  auto parse_tagidx() -> void;
  auto parse_globalidx() -> void;
  auto parse_dataidx() -> void;
  auto parse_localidx() -> void;
  auto parse_labelidx() -> void;
  
  // 5.5.2 Sections
  auto parse_section(Section_id section_id, std::invocable<uint32_t /*size*/> auto section_parser)
      -> decltype(section_parser(0));

  // 5.5.3 Custom Section
  auto parse_customsec(Ast_module& module) -> void;
  // > 7.4.1 Name section
  auto parse_namesubsection(Name_subsection_id N, std::invocable<uint32_t /*size*/> auto subsection_parser)
      -> decltype(subsection_parser(0));
  auto parse_modulenamesubsec() -> std::string;
  auto parse_funcnamesubsec() -> void;
  auto parse_localnamesubsec() -> void;
  auto parse_globalnamesubsec() -> void;
  auto parse_datasegmentnamesubsec() -> void;
  auto parse_namemap(bool dump = false) -> std::vector<Ast_TODO>;
  auto parse_nameassoc(bool dump = false) -> void;
  auto parse_indirectnamemap(bool dump) -> std::vector<Ast_TODO>;
  auto parse_indirectnameassoc(bool dump) -> void;

  // 5.5.4 Type Section
  auto parse_typesec() -> std::vector<Ast_functype>;

  // 5.5.5 Import Section
  auto parse_importdesc() -> void;
  auto parse_importsec() -> std::vector<Ast_import>;
  auto parse_import() -> Ast_import;

  // 5.5.6 Function Section
  auto parse_funcsec() -> std::vector<Ast_TODO>;

  // 5.5.7 Table Section
  auto parse_tablesec() -> std::vector<Ast_TODO>;
  auto parse_table() -> void;

  // 5.5.8 Memory Section
  auto parse_memsec() -> std::vector<Ast_TODO>;
  auto parse_mem() -> void;

  // 5.5.9 Global Section
  auto parse_globalsec() -> std::vector<Ast_TODO>;
  auto parse_global() -> void;

  // 5.5.10 Export Section
  auto parse_exportsec() -> std::vector<Ast_TODO>;
  auto parse_export() -> void;
  auto parse_exportdesc() -> void;

  // 5.5.11 Start Section
  auto parse_startsec() -> void;
  auto parse_start() -> void;

  // 5.5.12 Element Section
  auto parse_elemsec() -> std::vector<Ast_TODO>;
  auto parse_elem() -> void;

  // 5.5.13 Code Section
  auto parse_codesec() -> std::vector<Ast_TODO>;
  auto parse_code() -> void;
  auto parse_func() -> void;
  auto parse_locals() -> void;

  // 5.5.14 Data Section
  auto parse_datasec() -> std::vector<Ast_TODO>;
  auto parse_data() -> void;

  // 5.5.15 Data Count Section
  auto parse_datacountsec() -> uint32_t;

  // [EXTRA] Tag Section  (5.5.16 in Exception Handling Spec)
  auto parse_tagsec() -> std::vector<Ast_TODO>;
  auto parse_tag() -> void;

  // 5.5.16 Modules
  auto parse_magic() -> void;
  auto parse_version() -> void;
  auto parse_module() -> Ast_module;
};

inline auto parse_wasm(std::istream& is) -> Ast_module {
  auto parser = Wasm_parser{is};
  return parser.parse_module();
}

}  // namespace wasmtoolbox

#endif /* WASMTOOLBOX_PARSER_H */
