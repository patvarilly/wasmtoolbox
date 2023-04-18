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

#ifndef WASMTOOLBOX_TEXT_FORMAT_H
#define WASMTOOLBOX_TEXT_FORMAT_H

#include <iostream>

#include "ast.h"

namespace wasmtoolbox {

// The structure of the text format closely follows the structure of the WebAssembly spec + a few extensions:
//
// - WebAssembly Core Spec 2.0 (Draft 2023-04-08):
//     https://webassembly.github.io/spec/core/
// - Threads extension (1.1 Draft Nov 20, 2020):
//     https://webassembly.github.io/threads/core/
// - Exception handling spec: (2.0 Draft 2023-04-12 + exception handling Draft, Apr 12, 2023)
//     https://webassembly.github.io/exception-handling/core/
// - Extended name section (1.0, Draft Jul 08, 2019):
//     https://www.scheidecker.net/2019-07-08-extended-name-section-spec/appendix/custom.html

struct Text_format_writer {
  std::ostream* os_;
  
  explicit Text_format_writer(std::ostream& os) : os_{&os} {}

  // 6.2 Lexical Format
  // ==================
  
  // 6.2.2 Tokens
  auto tok_keyword(std::string_view keyword) -> void;
  auto tok_left_paren() -> void;
  auto tok_right_paren() -> void;

  // 6.2.3 White Space
  bool need_ws = false;
  int indent_level = 0;
  bool just_closed_sexp = false;
  auto lex_maybe_ws() -> void;
  auto lex_nl() -> void;

  // 6.2.4 Comments
  auto lex_blockcomment(std::string_view comment) -> void;

  // 6.3 Values
  // ==========

  // 6.3.5 Identifiers
  auto tok_id(std::string_view id) -> void;

  // 6.4 Types
  // =========

  // 6.4.1 Number Types
  auto write_numtype(Ast_numtype numtype) -> void;

  // 6.4.2 Vector Types
  auto write_vectype(Ast_vectype vectype) -> void;

  // 6.4.3 Reference Types
  auto write_reftype(Ast_reftype reftype) -> void;

  // 6.4.4 Value Types
  auto write_valtype(Ast_valtype valtype) -> void;
  
  // 6.4.5 Function Types
  auto write_functype(const Ast_functype& functype) -> void;
  
  // 6.6 Modules
  // ===========

  // 6.6.2 Types
  auto write_type(Ast_typeidx type_idx, const Ast_functype& functype) -> void;
  
  // 6.6.13 Modules
  auto write_module(const Ast_module& module) -> void;
};

}  // namespace wasmtoolbox

#endif /* WASMTOOLBOX_TEXT_FORMAT_H */
