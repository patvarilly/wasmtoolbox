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

#include "text_format.h"

#include <sstream>

namespace wasmtoolbox {

TEST(text_format_writer, min_module) {
  auto module = Ast_module{};
  auto os = std::stringstream{};
  auto w = Text_format_writer{os};

  w.write_module(module);

  EXPECT_THAT(os.str(), testing::StrEq("(module)"));
}

TEST(text_format_writer, module_with_name) {
  auto module = Ast_module{.name="hello"};
  auto os = std::stringstream{};
  auto w = Text_format_writer{os};

  w.write_module(module);

  EXPECT_THAT(os.str(), testing::StrEq("(module $hello)"));
}

TEST(text_format_writer, id) {
  auto do_it = [&](auto id) -> std::string {
    auto os = std::stringstream{};
    auto w = Text_format_writer{os};
    w.tok_id(id);
    return os.str();
  };

  EXPECT_THROW(do_it(""), std::logic_error);
  EXPECT_THAT(do_it("hello"), testing::Eq("$hello"));
  EXPECT_THAT(do_it("weird012!#$%&'*+-./:<=>?@\\^_`|~weird"), testing::Eq("$weird012!#$%&'*+-./:<=>?@\\^_`|~weird"));
  EXPECT_THROW(do_it("bad bad"), std::logic_error);
  EXPECT_THROW(do_it("bad\"bad"), std::logic_error);
  EXPECT_THROW(do_it("bad,bad"), std::logic_error);
  EXPECT_THROW(do_it("bad;bad"), std::logic_error);
  EXPECT_THROW(do_it("bad[bad"), std::logic_error);
  EXPECT_THROW(do_it("bad]bad"), std::logic_error);
  EXPECT_THROW(do_it("bad(bad"), std::logic_error);
  EXPECT_THROW(do_it("bad)bad"), std::logic_error);
  EXPECT_THROW(do_it("bad{bad"), std::logic_error);
  EXPECT_THROW(do_it("bad}bad"), std::logic_error);
  EXPECT_THAT(do_it("$"), testing::Eq("$$"));
}

TEST(text_format_writer, module_with_two_types) {
  auto module = Ast_module{
    .types = {
      Ast_functype{
        .params = {k_numtype_i32, k_numtype_i64, k_vectype_v128},
        .results = {k_numtype_f32, k_numtype_f64}
      },
      Ast_functype{
        .params = {},
        .results = {k_reftype_funcref, k_reftype_externref}
      }
    }
  };
  auto os = std::stringstream{};
  auto w = Text_format_writer{os};

  w.write_module(module);

  EXPECT_THAT(os.str(), testing::StrEq(
      "(module\n"
      "  (type (;0;) (func (param i32 i64 v128) (result f32 f64)))\n"
      "  (type (;1;) (func (result funcref externref))))"));
}

}  // namespace wasmtoolbox
