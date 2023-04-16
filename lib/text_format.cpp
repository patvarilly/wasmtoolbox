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

#include "text_format.h"

#include "absl/strings/str_format.h"

namespace wasmtoolbox {

// 6.2 Lexical Format
// ==================

// 6.2.2 Tokens
// ------------

auto Text_format_writer::tok_keyword(std::string_view keyword) -> void {
  lex_maybe_ws();
  *os_ << keyword;
  need_ws = true;
  just_closed_sexp = false;
}

auto Text_format_writer::tok_left_paren() -> void {
  lex_maybe_ws();
  *os_ << "(";
  indent_level += 2;
  need_ws = false;
  just_closed_sexp = false;
}

auto Text_format_writer::tok_right_paren() -> void {
  *os_ << ")";
  indent_level -= 2;
  need_ws = false;
  just_closed_sexp = true;
}

// 6.2.3 White Space
// -----------------

auto Text_format_writer::lex_maybe_ws() -> void {
  if (need_ws || just_closed_sexp) {
    *os_ << " ";
    need_ws = false;
    just_closed_sexp = false;
  }
}

auto Text_format_writer::lex_nl() -> void {
  *os_ << "\n";
  for (auto i = 0; i != indent_level; ++i) {
    *os_ << ' ';
  }
  need_ws = false;
  just_closed_sexp = false;
}

// 6.3 Values
// ==========

// 6.3.5 Identifiers
// -----------------

auto Text_format_writer::tok_id(std::string_view id) -> void {
  if (id.empty()) {
    throw std::logic_error("Invalid empty identifier");
  }
  
  auto valid_puncts = std::string_view{"!#$%&\'*+-./:<=>?@\\^_`|~"};
  for (auto c : id) {
    auto is_digit = c >= '0' && c <= '9';
    auto is_upper_alpha = c >= 'A' && c <= 'Z';
    auto is_lower_alpha = c >= 'a' && c <= 'z';
    auto is_valid_punct = valid_puncts.find(c) != std::string_view::npos;
    if (not (is_digit || is_upper_alpha || is_lower_alpha || is_valid_punct)) {
      throw std::logic_error(absl::StrFormat(
          "Invalid idchar in id \"%s\": '%c'", id, c));
    }
  }
  lex_maybe_ws();
  *os_ << '$' << id;
  need_ws = true;
  just_closed_sexp = false;
}


// 6.4 Types
// =========

// 6.4.5 Function Types
// --------------------

auto Text_format_writer::write_functype(const Ast_functype& functype) -> void {
  tok_left_paren();
  tok_keyword("func");
  for (auto param_idx = Ast_localidx{0}; param_idx != std::ssize(functype.params); ++param_idx) {
    tok_left_paren();
    tok_keyword("param");
    tok_id(absl::StrFormat("param%d", param_idx));
    tok_right_paren();
  }
  for (const auto& result : functype.results) {
    tok_left_paren();
    tok_keyword("result");
    tok_right_paren();
  }
  tok_right_paren();
}


// 6.6 Modules
// ===========

// 6.6.2 Types
// -----------

auto Text_format_writer::write_type(Ast_typeidx type_idx, const Ast_functype& functype) -> void {
  lex_nl();
  tok_left_paren();
  tok_keyword("type");
  tok_id(absl::StrFormat("type%d", type_idx));
  write_functype(functype);
  tok_right_paren();
}

// 6.6.13 Modules
// --------------

auto Text_format_writer::write_module(const Ast_module& module) -> void {
  tok_left_paren();
  tok_keyword("module");
  if (module.name.has_value()) {
    tok_id(module.name.value());
  }
  for (auto type_idx = Ast_typeidx{0}; type_idx != std::ssize(module.types); ++type_idx) {
    write_type(type_idx, module.types[type_idx]);
  }
  tok_right_paren();
}

}  // namespace wasmtoolbox
