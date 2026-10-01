#include "cord.hpp"

struct UnregisteredStruct {
    int x;
    std::string name;
};
// Missing: CORD_REGISTER_STRUCT(UnregisteredStruct)

int main() {
    // This should fail to compile: CustomStruct requires CORD_REGISTER_STRUCT
    cord::CustomStruct<UnregisteredStruct> def("UnregisteredStruct");
    return 0;
}
