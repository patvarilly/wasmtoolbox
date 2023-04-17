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

#ifndef WASMTOOLBOX_AST_H
#define WASMTOOLBOX_AST_H

#include <iostream>
#include <string>
#include <optional>
#include <vector>

namespace wasmtoolbox {

// The structure of the AST closely follows the structure of the WebAssembly spec + a few extensions:
//
// - WebAssembly Core Spec 2.0 (Draft 2023-04-08):
//     https://webassembly.github.io/spec/core/
// - Threads extension (1.1 Draft Nov 20, 2020):
//     https://webassembly.github.io/threads/core/
// - Exception handling spec: (2.0 Draft 2023-04-12 + exception handling Draft, Apr 12, 2023)
//     https://webassembly.github.io/exception-handling/core/
// - Extended name section (1.0, Draft Jul 08, 2019):
//     https://www.scheidecker.net/2019-07-08-extended-name-section-spec/appendix/custom.html

// TODO: Get rid of this when no longer needed
struct Ast_TODO {};  // A node on the AST that we don't deal with yet

// 2.3 Types
// =========

// 2.3.1 Number Types
// 2.3.2 Vector Types
// 2.3.3 Reference Types
// 2.3.4 Value Types
//
// These are all tags with no extra information, so we pack them into a single enum.
// The specific types are merely aliases to the general valtype, purely for information.
// It would be cleaner to define separate enum classes and use a std::variant to join them
// up in valtype, but for now, that feels like overkill

enum Ast_valtype {
  // 2.3.1 Number Types
  k_numtype_i32,
  k_numtype_i64,
  k_numtype_f32,
  k_numtype_f64,

  // 2.3.2 Vector Types
  k_vectype_v128,

  // 2.3.3 Reference Types
  k_reftype_funcref,
  k_reftype_externref
};
using Ast_numtype = Ast_valtype;
using Ast_vectype = Ast_valtype;
using Ast_reftype = Ast_valtype;

/// 2.3.5 Result Types
using Ast_resulttype = std::vector<Ast_valtype>;

// 2.3.6 Function Types
struct Ast_functype {
  Ast_resulttype params{};
  Ast_resulttype results{};
};

// 2.5 Modules
// ===========

struct Ast_module {
  std::optional<std::string> name{};
  std::vector<Ast_functype> types{};
};

// 2.5.1 Indices
using Ast_typeidx = uint32_t;
using Ast_localidx = uint32_t;

}  // namespace wasmtoolbox

#endif /* WASMTOOLBOX_AST_H */
