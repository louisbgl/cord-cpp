#include <catch2/catch_test_macros.hpp>
#include "cord.hpp"

#include <sstream>

static std::string capture_describe(cord::Schema& schema) {
    std::ostringstream buf;
    auto* old = std::cout.rdbuf(buf.rdbuf());
    schema.describe();
    std::cout.rdbuf(old);
    return buf.str();
}

TEST_CASE("describe() under threshold: schema header and closing brace present", "[describe]") {
    cord::Schema schema;
    schema.add<int>("port").required().min(1024).max(65535);
    schema.add<std::string>("host").default_("localhost");
    schema.add<bool>("debug").default_(false);
    schema.add<float>("ratio").default_(0.5f).min(0.0f).max(1.0f);
    schema.add<std::string>("level").oneOf({"info", "warn", "error"});

    std::string out = capture_describe(schema);
    CHECK(out.find("Schema {") != std::string::npos);
    CHECK(out.find("}") != std::string::npos);
    // no blank-line grouping below threshold
    CHECK(out.find("\n\n") == std::string::npos);
}

TEST_CASE("describe() over threshold: groups separated by blank lines", "[describe]") {
    cord::Schema schema;
    // numeric
    schema.add<int>("port").required().min(1024).max(65535);
    schema.add<int>("workers").default_(4);
    schema.add<int>("timeout").default_(1000).oneOf({100, 500, 1000});
    schema.add<float>("ratio").default_(0.5f).min(0.0f).max(1.0f);
    schema.add<double>("threshold").default_(0.9);
    // bool
    schema.add<bool>("debug").default_(false);
    schema.add<bool>("verbose").default_(false);
    // string
    schema.add<std::string>("host").default_("localhost");
    schema.add<std::string>("level").oneOf({"info", "warn", "error"});
    // vectors with actual elements to exercise valueToString vector paths
    schema.add<std::vector<int>>("ports").default_({80, 443, 8080});
    schema.add<std::vector<float>>("weights").default_({0.1f, 0.5f, 0.9f});
    schema.add<std::vector<double>>("scores").default_({1.0, 2.5, 3.7});
    schema.add<std::vector<bool>>("flags").default_({true, false, true});
    schema.add<std::vector<std::string>>("tags").default_({"a", "b", "c"});

    std::string out = capture_describe(schema);
    CHECK(out.find("Schema {") != std::string::npos);
    CHECK(out.find("\n\n") != std::string::npos);
}
