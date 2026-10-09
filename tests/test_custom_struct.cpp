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

// ============================================================================
// Constraints
// ============================================================================

TEST_CASE("Custom struct: min() enforced on int field", "[custom_struct][constraints]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).min(10);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto ok = schema.parse("obj = { x = 20, name = \"test\" }");
    REQUIRE_FALSE(ok.hasErrors());

    auto bad = schema.parse("obj = { x = 5, name = \"test\" }");
    REQUIRE(bad.hasErrors());
    CHECK(bad.getErrors()[0].message.find("below minimum") != std::string::npos);
}

TEST_CASE("Custom struct: max() enforced on int field", "[custom_struct][constraints]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).max(100);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto ok = schema.parse("obj = { x = 50, name = \"test\" }");
    REQUIRE_FALSE(ok.hasErrors());

    auto bad = schema.parse("obj = { x = 200, name = \"test\" }");
    REQUIRE(bad.hasErrors());
    CHECK(bad.getErrors()[0].message.find("exceeds maximum") != std::string::npos);
}

TEST_CASE("Custom struct: min() and max() combined on int field", "[custom_struct][constraints]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).min(10).max(100);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    CHECK_FALSE(schema.parse("obj = { x = 50, name = \"test\" }").hasErrors());
    CHECK(schema.parse("obj = { x = 5, name = \"test\" }").hasErrors());
    CHECK(schema.parse("obj = { x = 150, name = \"test\" }").hasErrors());
}

TEST_CASE("Custom struct: min() enforced on string length", "[custom_struct][constraints]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name).min(5);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto ok = schema.parse("obj = { x = 1, name = \"hello\" }");
    REQUIRE_FALSE(ok.hasErrors());

    auto bad = schema.parse("obj = { x = 1, name = \"hi\" }");
    REQUIRE(bad.hasErrors());
    CHECK(bad.getErrors()[0].message.find("shorter than minimum length") != std::string::npos);
}

TEST_CASE("Custom struct: max() enforced on string length", "[custom_struct][constraints]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name).max(10);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto ok = schema.parse("obj = { x = 1, name = \"short\" }");
    REQUIRE_FALSE(ok.hasErrors());

    auto bad = schema.parse("obj = { x = 1, name = \"very_long_name\" }");
    REQUIRE(bad.hasErrors());
    CHECK(bad.getErrors()[0].message.find("longer than maximum length") != std::string::npos);
}

TEST_CASE("Custom struct: min() enforced on vector count", "[custom_struct][constraints]") {
    cord::CustomStruct<WithVectors> vec_def("WithVectors");
    vec_def.add("ids", &WithVectors::ids).min(2);
    vec_def.add("tags", &WithVectors::tags);

    cord::Schema schema;
    schema.add<WithVectors>("obj", vec_def);

    auto ok = schema.parse("obj = { ids = [1, 2, 3], tags = [] }");
    REQUIRE_FALSE(ok.hasErrors());

    auto bad = schema.parse("obj = { ids = [1], tags = [] }");
    REQUIRE(bad.hasErrors());
    CHECK(bad.getErrors()[0].message.find("too few elements") != std::string::npos);
}

TEST_CASE("Custom struct: max() enforced on vector count", "[custom_struct][constraints]") {
    cord::CustomStruct<WithVectors> vec_def("WithVectors");
    vec_def.add("ids", &WithVectors::ids).max(3);
    vec_def.add("tags", &WithVectors::tags);

    cord::Schema schema;
    schema.add<WithVectors>("obj", vec_def);

    auto ok = schema.parse("obj = { ids = [1, 2], tags = [] }");
    REQUIRE_FALSE(ok.hasErrors());

    auto bad = schema.parse("obj = { ids = [1, 2, 3, 4], tags = [] }");
    REQUIRE(bad.hasErrors());
    CHECK(bad.getErrors()[0].message.find("too many elements") != std::string::npos);
}

TEST_CASE("Custom struct: oneOf() enforced on string field", "[custom_struct][constraints]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name).oneOf({"alice", "bob", "charlie"});

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto ok = schema.parse("obj = { x = 1, name = \"alice\" }");
    REQUIRE_FALSE(ok.hasErrors());

    auto bad = schema.parse("obj = { x = 1, name = \"dave\" }");
    REQUIRE(bad.hasErrors());
    CHECK(bad.getErrors()[0].message.find("not in allowed values") != std::string::npos);
}

TEST_CASE("Custom struct: oneOf() enforced on int field", "[custom_struct][constraints]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).oneOf({1, 2, 3});
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto ok = schema.parse("obj = { x = 2, name = \"test\" }");
    REQUIRE_FALSE(ok.hasErrors());

    auto bad = schema.parse("obj = { x = 99, name = \"test\" }");
    REQUIRE(bad.hasErrors());
    CHECK(bad.getErrors()[0].message.find("not in allowed values") != std::string::npos);
}

TEST_CASE("Custom struct: constraint error includes field context", "[custom_struct][constraints]") {
    cord::CustomStruct<AllTypes> all_def("AllTypes");
    all_def.add("count", &AllTypes::count).min(10);
    all_def.add("flag", &AllTypes::flag);
    all_def.add("ratio", &AllTypes::ratio);
    all_def.add("precision", &AllTypes::precision);
    all_def.add("text", &AllTypes::text);

    cord::Schema schema;
    schema.add<AllTypes>("config", all_def);

    auto result = schema.parse("config = { count = 5, flag = true, ratio = 1.0, precision = 1.0, text = \"test\" }");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    REQUIRE_FALSE(errors.empty());

    // Constraint errors propagate from field validation
    CHECK(errors[0].message.find("below minimum") != std::string::npos);
}

// ============================================================================
// Eager Validation (Schema Construction)
// ============================================================================

TEST_CASE("Custom struct: duplicate field name throws", "[custom_struct][eager]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);

    CHECK_THROWS_AS(
        simple_def.add("x", &Simple::name),  // duplicate field name
        cord::CordException
    );
}

TEST_CASE("Custom struct: empty field name throws", "[custom_struct][eager]") {
    cord::CustomStruct<Simple> simple_def("Simple");

    CHECK_THROWS_AS(
        simple_def.add("", &Simple::x),
        cord::CordException
    );
}

TEST_CASE("Custom struct: required() then default_() throws", "[custom_struct][eager]") {
    cord::CustomStruct<Simple> simple_def("Simple");

    CHECK_THROWS_AS(
        simple_def.add("x", &Simple::x).required().default_(42),
        cord::CordException
    );
}

TEST_CASE("Custom struct: default_() then required() throws", "[custom_struct][eager]") {
    cord::CustomStruct<Simple> simple_def("Simple");

    CHECK_THROWS_AS(
        simple_def.add("x", &Simple::x).default_(42).required(),
        cord::CordException
    );
}

TEST_CASE("Custom struct: min() > max() throws for int", "[custom_struct][eager]") {
    cord::CustomStruct<Simple> simple_def("Simple");

    CHECK_THROWS_AS(
        simple_def.add("x", &Simple::x).min(100).max(10),
        cord::CordException
    );
}

TEST_CASE("Custom struct: min() > max() throws for string length", "[custom_struct][eager]") {
    cord::CustomStruct<Simple> simple_def("Simple");

    CHECK_THROWS_AS(
        simple_def.add("name", &Simple::name).min(10).max(5),
        cord::CordException
    );
}

TEST_CASE("Custom struct: oneOf() with empty list throws", "[custom_struct][eager]") {
    cord::CustomStruct<Simple> simple_def("Simple");

    CHECK_THROWS_AS(
        simple_def.add("x", &Simple::x).oneOf({}),
        cord::CordException
    );
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_CASE("Custom struct: empty object with all optional fields", "[custom_struct][edge]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = {}");
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("obj").as<Simple>();
    CHECK(obj.x == 0);      // struct default
    CHECK(obj.name == "");  // struct default
}

TEST_CASE("Custom struct: empty object with Field defaults", "[custom_struct][edge]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).default_(999);
    simple_def.add("name", &Simple::name).default_("default");

    cord::Schema schema;
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = {}");
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("obj").as<Simple>();
    CHECK(obj.x == 999);
    CHECK(obj.name == "default");
}

// ============================================================================
// Integration with Schema
// ============================================================================

TEST_CASE("Custom struct: case-insensitive mode on top-level schema", "[custom_struct][integration]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.setCaseInsensitive(true);
    schema.add<Simple>("config", simple_def);

    // Top-level key case-insensitive
    auto result = schema.parse("CONFIG = { x = 10, name = \"test\" }");
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("config").as<Simple>();
    CHECK(obj.x == 10);
    CHECK(obj.name == "test");
}

TEST_CASE("Custom struct: strict mode on top-level with valid custom struct", "[custom_struct][integration]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.setStrict(true);
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = { x = 1, name = \"test\" }");
    REQUIRE_FALSE(result.hasErrors());

    Simple obj = result.get("obj").as<Simple>();
    CHECK(obj.x == 1);
    CHECK(obj.name == "test");
}

TEST_CASE("Custom struct: strict mode rejects unknown top-level key", "[custom_struct][integration]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.setStrict(true);
    schema.add<Simple>("obj", simple_def);

    auto result = schema.parse("obj = { x = 1, name = \"test\" }\nunknown = 42");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    CHECK(errors[0].message.find("unknown") != std::string::npos);
}

TEST_CASE("Custom struct: multiple different custom types in same schema", "[custom_struct][integration]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::CustomStruct<AllTypes> all_def("AllTypes");
    all_def.add("flag", &AllTypes::flag);
    all_def.add("count", &AllTypes::count);
    all_def.add("ratio", &AllTypes::ratio);
    all_def.add("precision", &AllTypes::precision);
    all_def.add("text", &AllTypes::text);

    cord::Schema schema;
    schema.add<Simple>("simple", simple_def);
    schema.add<AllTypes>("all", all_def);

    auto result = schema.parse(R"(
simple = { x = 42, name = "test" }
all = { flag = true, count = 10, ratio = 1.5, precision = 3.14, text = "hello" }
)");
    REQUIRE_FALSE(result.hasErrors());

    Simple s = result.get("simple").as<Simple>();
    CHECK(s.x == 42);
    CHECK(s.name == "test");

    AllTypes a = result.get("all").as<AllTypes>();
    CHECK(a.flag == true);
    CHECK(a.count == 10);
}

// ============================================================================
// Vector of Custom Structs
// ============================================================================

TEST_CASE("Vector custom struct: empty vector", "[vector_custom][basic]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("items = []");
    REQUIRE_FALSE(result.hasErrors());

    auto items = result.get("items").as<std::vector<Simple>>();
    CHECK(items.empty());
}

TEST_CASE("Vector custom struct: single element", "[vector_custom][basic]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("items = [{x = 10, name = \"first\"}]");
    REQUIRE_FALSE(result.hasErrors());

    auto items = result.get("items").as<std::vector<Simple>>();
    REQUIRE(items.size() == 1);
    CHECK(items[0].x == 10);
    CHECK(items[0].name == "first");
}

TEST_CASE("Vector custom struct: multiple elements", "[vector_custom][basic]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("items = [{x = 1, name = \"a\"}, {x = 2, name = \"b\"}, {x = 3, name = \"c\"}]");
    REQUIRE_FALSE(result.hasErrors());

    auto items = result.get("items").as<std::vector<Simple>>();
    REQUIRE(items.size() == 3);
    CHECK(items[0].x == 1);
    CHECK(items[0].name == "a");
    CHECK(items[1].x == 2);
    CHECK(items[1].name == "b");
    CHECK(items[2].x == 3);
    CHECK(items[2].name == "c");
}

TEST_CASE("Vector custom struct: different struct types in schema", "[vector_custom][basic]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::CustomStruct<AllTypes> all_def("AllTypes");
    all_def.add("flag", &AllTypes::flag);
    all_def.add("count", &AllTypes::count);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("simples", simple_def);
    schema.add<std::vector<AllTypes>>("configs", all_def);

    auto result = schema.parse(R"(
simples = [{x = 1, name = "a"}, {x = 2, name = "b"}]
configs = [{flag = true, count = 10}]
)");
    REQUIRE_FALSE(result.hasErrors());

    auto simples = result.get("simples").as<std::vector<Simple>>();
    REQUIRE(simples.size() == 2);
    CHECK(simples[0].x == 1);

    auto configs = result.get("configs").as<std::vector<AllTypes>>();
    REQUIRE(configs.size() == 1);
    CHECK(configs[0].flag == true);
}

TEST_CASE("Vector custom struct: element missing required field", "[vector_custom][validation]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).required();
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("items = [{name = \"missing x\"}]");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].message.find("required") != std::string::npos);
    CHECK(errors[0].message.find("x") != std::string::npos);
}

TEST_CASE("Vector custom struct: element has default field", "[vector_custom][validation]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).default_(99);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("items = [{name = \"only name\"}]");
    REQUIRE_FALSE(result.hasErrors());

    auto items = result.get("items").as<std::vector<Simple>>();
    REQUIRE(items.size() == 1);
    CHECK(items[0].x == 99);
    CHECK(items[0].name == "only name");
}

TEST_CASE("Vector custom struct: element field type mismatch", "[vector_custom][validation]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("items = [{x = \"not an int\", name = \"test\"}]");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    CHECK(errors.size() > 0);
}

TEST_CASE("Vector custom struct: element field constraint violation", "[vector_custom][validation]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).min(10);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("items = [{x = 5, name = \"too small\"}, {x = 15, name = \"ok\"}]");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].message.find("index") != std::string::npos);
    CHECK(errors[0].message.find("0") != std::string::npos);
}

TEST_CASE("Vector custom struct: vector field required", "[vector_custom][metadata]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def).required();

    auto result = schema.parse("");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].message.find("required") != std::string::npos);
    CHECK(errors[0].message.find("items") != std::string::npos);
}

TEST_CASE("Vector custom struct: vector field optional", "[vector_custom][metadata]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("");
    REQUIRE_FALSE(result.hasErrors());
    CHECK_FALSE(result.contains("items"));
}

TEST_CASE("Vector custom struct: field type is VECTOR_CUSTOM", "[vector_custom][type_system]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    auto& field = schema.add<std::vector<Simple>>("items", simple_def);

    CHECK(field.getType() == cord::FieldType::VECTOR_CUSTOM);
}

TEST_CASE("Vector custom struct: error includes element index", "[vector_custom][errors]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x).max(100);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("items = [{x = 50, name = \"ok\"}, {x = 150, name = \"too big\"}]");
    REQUIRE(result.hasErrors());

    auto errors = result.getErrors();
    REQUIRE(errors.size() == 1);
    CHECK(errors[0].message.find("element at index 1") != std::string::npos);
}

TEST_CASE("Vector custom struct: parse malformed element", "[vector_custom][errors]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("items = [{x = 1, name = }]");
    REQUIRE(result.hasErrors());
}

TEST_CASE("Vector custom struct: missing closing bracket", "[vector_custom][errors]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("items = [{x = 1, name = \"a\"}");
    REQUIRE(result.hasErrors());
}

TEST_CASE("Vector custom struct: all elements identical", "[vector_custom][edge]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<Simple>>("items", simple_def);

    auto result = schema.parse("items = [{x = 42, name = \"same\"}, {x = 42, name = \"same\"}, {x = 42, name = \"same\"}]");
    REQUIRE_FALSE(result.hasErrors());

    auto items = result.get("items").as<std::vector<Simple>>();
    REQUIRE(items.size() == 3);
    for (const auto& item : items) {
        CHECK(item.x == 42);
        CHECK(item.name == "same");
    }
}

TEST_CASE("Vector custom struct: mixed with primitive vectors", "[vector_custom][interaction]") {
    cord::CustomStruct<Simple> simple_def("Simple");
    simple_def.add("x", &Simple::x);
    simple_def.add("name", &Simple::name);

    cord::Schema schema;
    schema.add<std::vector<int>>("numbers");
    schema.add<std::vector<Simple>>("objects", simple_def);

    auto result = schema.parse(R"(
numbers = [1, 2, 3]
objects = [{x = 10, name = "test"}]
)");
    REQUIRE_FALSE(result.hasErrors());

    auto numbers = result.get("numbers").as<std::vector<int>>();
    CHECK(numbers.size() == 3);

    auto objects = result.get("objects").as<std::vector<Simple>>();
    CHECK(objects.size() == 1);
}

