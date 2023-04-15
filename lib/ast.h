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

struct Ast_module {
  std::optional<std::string> name;
};

}  // namespace wasmtoolbox

#endif /* WASMTOOLBOX_AST_H */
