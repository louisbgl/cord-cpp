#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>
#include "cord.hpp"

struct Simple {
    int x = 0;
    std::string name = "";
};
CORD_REGISTER_STRUCT(Simple);

struct AllTypes {
    bool flag = false;
    int count = 0;
    float ratio = 0.0f;
    double precision = 0.0;
    std::string text = "";
};
CORD_REGISTER_STRUCT(AllTypes);

struct WithVectors {
    std::vector<int> ids;
    std::vector<std::string> tags;
};
CORD_REGISTER_STRUCT(WithVectors);

// ============================================================================
// Basic Parsing
// ============================================================================

TEST_CASE("Custom struct: basic int and string fields", "[custom_struct][basic]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = { x = 42, name = \"test\" }");
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("obj").as<Simple>();
    CHECK(obj.x == 42);
    CHECK(obj.name == "test");
}

TEST_CASE("Custom struct: all primitive types", "[custom_struct][basic]") {
    cord::CustomStruct<AllTypes> all_def("AllTypes");
    all_def.add("flag", &AllTypes::flag);
    all_def.add("count", &AllTypes::count);
    all_def.add("ratio", &AllTypes::ratio);
    all_def.add("precision", &AllTypes::precision);
    all_def.add("text", &AllTypes::text);

    cord::Schema schema;
    schema.add<AllTypes>("config", all_def);

    auto result = schema.parse(R"(config = { flag = true, count = 100, ratio = 1.5, precision = 3.14159, text = "hello" })");
    REQUIRE_FALSE(result.hasErrors());

    AllTypes obj = result.get("config").as<AllTypes>();
    CHECK(obj.flag == true);
    CHECK(obj.count == 100);
    CHECK_THAT(obj.ratio, Catch::Matchers::WithinRel(1.5f, 1e-5f));
    CHECK_THAT(obj.precision, Catch::Matchers::WithinRel(3.14159, 1e-5));
    CHECK(obj.text == "hello");
}

TEST_CASE("Custom struct: vector fields", "[custom_struct][basic]") {
    cord::CustomStruct<WithVectors> vec_def("WithVectors");
    vec_def.add("ids", &WithVectors::ids);
    vec_def.add("tags", &WithVectors::tags);

    cord::Schema schema;
    schema.add<WithVectors>("data", vec_def);

    auto result = schema.parse(R"(data = { ids = [1, 2, 3], tags = ["a", "b"] })");
    if (result.hasErrors()) result.printErrors();
    REQUIRE_FALSE(result.hasErrors());

    WithVectors obj = result.get("data").as<WithVectors>();
    REQUIRE(obj.ids.size() == 3);
    CHECK(obj.ids[0] == 1);
    CHECK(obj.ids[1] == 2);
    CHECK(obj.ids[2] == 3);
    REQUIRE(obj.tags.size() == 2);
    CHECK(obj.tags[0] == "a");
    CHECK(obj.tags[1] == "b");
}

TEST_CASE("Custom struct: empty vectors in fields", "[custom_struct][basic]") {
    cord::CustomStruct<WithVectors> vec_def("WithVectors");
    vec_def.add("ids", &WithVectors::ids);
    vec_def.add("tags", &WithVectors::tags);

    cord::Schema schema;
    schema.add<WithVectors>("data", vec_def);

    auto result = schema.parse(R"(data = { ids = [], tags = [] })");
    REQUIRE_FALSE(result.hasErrors());

    WithVectors obj = result.get("data").as<WithVectors>();
    CHECK(obj.ids.empty());
    CHECK(obj.tags.empty());
}

TEST_CASE("Custom struct: mixed with primitives in schema", "[custom_struct][basic]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<int>("port");
    schema.add<Simple>("config", simple_def);
    schema.add<std::string>("host");

    auto result = schema.parse(R"(
port = 8080
config = { x = 10, name = "test" }
host = "localhost"
)");
    REQUIRE_FALSE(result.hasErrors());

    CHECK(result.get("port").as<int>() == 8080);
    CHECK(result.get("host").as<std::string>() == "localhost");

    Simple obj = result.get("config").as<Simple>();
    CHECK(obj.x == 10);
    CHECK(obj.name == "test");
}

TEST_CASE("Custom struct: multiple instances of same type", "[custom_struct][basic]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("first", simple_def);
    schema.add<Simple>("second", simple_def);

    auto result = schema.parse(R"(
first = { x = 1, name = "one" }
second = { x = 2, name = "two" }
)");
    REQUIRE_FALSE(result.hasErrors());

    Simple first = result.get("first").as<Simple>();
    CHECK(first.x == 1);
    CHECK(first.name == "one");

    Simple second = result.get("second").as<Simple>();
    CHECK(second.x == 2);
    CHECK(second.name == "two");
}

TEST_CASE("Custom struct: whitespace around braces and fields", "[custom_struct][basic]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = {   x   =   42  ,  name  =  \"test\"   }");
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("obj").as<Simple>();
    CHECK(obj.x == 42);
    CHECK(obj.name == "test");
}

TEST_CASE("Custom struct: string field with comma inside quotes", "[custom_struct][basic]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse(R"(obj = { x = 1, name = "hello, world" })");
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("obj").as<Simple>();
    CHECK(obj.x == 1);
    CHECK(obj.name == "hello, world");
}

// ============================================================================
// Required Fields & Defaults
// ============================================================================

TEST_CASE("Custom struct: required field missing produces error", "[custom_struct][required]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).required();
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = { name = \"test\" }");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    REQUIRE_FALSE(errors.empty());
    CHECK(errors[0].message.find("x") != std::string::npos);
}

TEST_CASE("Custom struct: required field present, no error", "[custom_struct][required]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).required();
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = { x = 42, name = \"test\" }");
    result.printErrors();
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("obj").as<Simple>();
    CHECK(obj.x == 42);
    CHECK(obj.name == "test");
}

TEST_CASE("Custom struct: multiple required fields, all present", "[custom_struct][required]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).required();
    simple_def.add("name", &Simple::name).required();

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = { x = 10, name = \"required\" }");
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("obj").as<Simple>();
    CHECK(obj.x == 10);
    CHECK(obj.name == "required");
}

TEST_CASE("Custom struct: multiple required fields, one missing", "[custom_struct][required]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).required();
    simple_def.add("name", &Simple::name).required();

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = { x = 10 }");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    REQUIRE_FALSE(errors.empty());
    CHECK(errors[0].message.find("name") != std::string::npos);
}

TEST_CASE("Custom struct: optional field missing, uses struct default", "[custom_struct][defaults]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    // Only provide x, name should use struct default ("")
    auto result = schema.parse("obj = { x = 42 }");
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("obj").as<Simple>();
    CHECK(obj.x == 42);
    CHECK(obj.name == "");  // struct default
}

TEST_CASE("Custom struct: optional field present, overrides struct default", "[custom_struct][defaults]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = { x = 42, name = \"override\" }");
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("obj").as<Simple>();
    CHECK(obj.x == 42);
    CHECK(obj.name == "override");
}

TEST_CASE("Custom struct: Field::default_() overrides struct member default", "[custom_struct][defaults]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).default_(999);  // Field default
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    // x absent, should use Field::default_(999) not struct default (0)
    auto result = schema.parse("obj = { name = \"test\" }");
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("obj").as<Simple>();
    CHECK(obj.x == 999);  // Field default wins
    CHECK(obj.name == "test");
}

TEST_CASE("Custom struct: unknown key produces error (always strict)", "[custom_struct][errors]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = { x = 1, name = \"test\", unknown = 42 }");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    REQUIRE_FALSE(errors.empty());
    CHECK(errors[0].message.find("unknown") != std::string::npos);
}
