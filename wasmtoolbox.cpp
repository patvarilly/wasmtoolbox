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

#include <iostream>
#include <fstream>

#include "absl/log/initialize.h"
#include "absl/strings/str_format.h"

#include "parser.h"

namespace wasmtoolbox {

auto usage() -> void {
  std::cerr <<
      "Usage: wasmtoolbox <tool> [<args>]\n"
      "Tools:\n"
      "- wasm2wat <file.wasm>\n"
      "    Converts binary representation in <file.wasm> to text representation\n";
  std::exit(EXIT_FAILURE);
}

}  // namespace wasmtoolbox

auto main(int argc, char** argv) -> int {
  using namespace wasmtoolbox;
  
  absl::InitializeLog();

  if (argc < 2) { usage(); }

  auto toolname = std::string{argv[1]};
  if (toolname == "wasm2wat") {
    if (argc < 3) { usage(); }
    auto filename = std::string{argv[2]};
    auto is = std::ifstream{filename, std::ios::binary};
    if (!is) {
      std::cerr << absl::StreamFormat("Error: could not open file %s\n", filename);
      return EXIT_FAILURE;
    }
    parse_wasm(is);
    // TODO: Generate AST and produce WAT
  } else {
    usage();
  }
  
  return 0;
}
