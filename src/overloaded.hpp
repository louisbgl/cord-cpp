#pragma once

namespace cord {

/**
 * @brief Merges several lambdas into one overload set, mainly for std::visit.
 */
template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

} // namespace cord
