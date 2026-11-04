#include <catch2/catch_test_macros.hpp>
#include "lexer.hpp"

TEST_CASE("Lexer can be constructed from a string", "[lexer]") {
    cord::Lexer lexer("port = 8080");
    SUCCEED();
}
