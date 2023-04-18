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

// 6.2.4 Comments
// --------------

auto Text_format_writer::lex_blockcomment(std::string_view comment) -> void {
  lex_maybe_ws();
  *os_ << "(;";
  // Assume doesn't contain an improperly nested closing ";)"
  // TODO: Check the above!
  *os_ << comment;
  *os_ << ";)";
  need_ws = true;
  just_closed_sexp = true;
}


// 6.3 Values
// ==========

// 6.3.3 Strings
// -------------

auto Text_format_writer::tok_string(std::string_view str) -> void {
  lex_maybe_ws();
  *os_ << '\"';
  for (const auto& c : str) {
    // Not yet handling UTF-8 parsing: instead, dump raw bytes outside 7-bit ASCII
    switch (c) {
      case '\t': *os_ << "\\t"; break;
      case '\n': *os_ << "\\n"; break;
      case '\r': *os_ << "\\r"; break;
      case '\"': *os_ << "\\\""; break;
      case '\'': *os_ << "\\'"; break;
      case '\\': *os_ << "\\\\"; break;
      default:
        if (c >= 0x20 && c < 0x7F) {
          *os_ << static_cast<char>(c);
        } else {
          *os_ << absl::StreamFormat("\\%02x", c);
        }
    }
  }
  *os_ << '\"';
  need_ws = true;
  just_closed_sexp = false;
}

// 6.3.4 Names
// -----------

auto Text_format_writer::tok_name(std::string_view name) -> void {
  tok_string(name);
}

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

// 6.4.1 Number Types
// ------------------

auto Text_format_writer::write_numtype(Ast_numtype numtype) -> void {
  // See ast.h for why numtype and valtype are actually the same
  write_valtype(numtype);
}


// 6.4.2 Vector Types
// ------------------

auto Text_format_writer::write_vectype(Ast_vectype vectype) -> void {
  // See ast.h for why vectype and valtype are actually the same
  write_valtype(vectype);
}

// 6.4.3 Reference Types
// ---------------------

auto Text_format_writer::write_reftype(Ast_reftype reftype) -> void {
  // See ast.h for why reftype and valtype are actually the same
  write_valtype(reftype);
}

// 6.4.4 Value Types
// -----------------

auto Text_format_writer::write_valtype(Ast_valtype valtype) -> void {
  switch (valtype) {
    case k_numtype_i32: return tok_keyword("i32");
    case k_numtype_i64: return tok_keyword("i64");
    case k_numtype_f32: return tok_keyword("f32");
    case k_numtype_f64: return tok_keyword("f64");
    case k_vectype_v128: return tok_keyword("v128");
    case k_reftype_funcref: return tok_keyword("funcref");
    case k_reftype_externref: return tok_keyword("externref");
    default:
      throw std::logic_error(absl::StrFormat(
          "Unrecognized Ast_valtype %d", valtype));
  }
}

// 6.4.5 Function Types
// --------------------

auto Text_format_writer::write_functype(const Ast_functype& functype) -> void {
  tok_left_paren();
  tok_keyword("func");
  if (not functype.params.empty()) {
    tok_left_paren();
    tok_keyword("param");
    for (const auto& param : functype.params) {
      write_valtype(param);
    }
    tok_right_paren();
  }
  if (not functype.results.empty()) {
    tok_left_paren();
    tok_keyword("result");
    for (const auto& result : functype.results) {
      write_valtype(result);
    }
    tok_right_paren();
  }
  tok_right_paren();
}


// 6.6 Modules
// ===========

// 6.6.2 Types
// -----------

auto Text_format_writer::write_type(Ast_typeidx typeidx, const Ast_functype& functype) -> void {
  lex_nl();
  tok_left_paren();
  tok_keyword("type");
  lex_blockcomment(absl::StrFormat("%d", typeidx));
  write_functype(functype);
  tok_right_paren();
}

// 6.6.4 Imports
// -------------

auto Text_format_writer::write_import(const Ast_import& import) -> void {
  lex_nl();
  tok_left_paren();
  tok_keyword("import");
  tok_name(import.module);
  tok_name(import.name);
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
  for (auto typeidx = Ast_typeidx{0}; typeidx != std::ssize(module.types); ++typeidx) {
    write_type(typeidx, module.types[typeidx]);
  }
  for (const auto& import : module.imports) {
    write_import(import);
  }
  tok_right_paren();
}

}  // namespace wasmtoolbox
