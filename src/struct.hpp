#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "context.hpp"
#include "converter.hpp"
#include "node.hpp"

// Makes a static_assert message stand out in the compiler's output. Takes a string literal.
#define CORD_COMPILE_ERROR(message)                                                         \
    "\n\n"                                                                                  \
    "===================================== CORD =====================================\n"    \
    message "\n"                                                                            \
    "================================================================================\n"

namespace cord {

/**
 * @brief One key of a struct, with its member's type hidden so all keys fit in one vector.
 */
template <class T>
struct MemberBase {
    virtual ~MemberBase() = default;

    /** @brief Converts the node into this member of out. Same contract as Converter::read. */
    virtual bool read(const Node& node, T& out, Context& ctx) const = 0;

    std::string key;
};

/**
 * @brief A key bound to a member of type M.
 */
template <class T, Convertible M>
struct Member : MemberBase<T> {
    bool read(const Node& node, T& out, Context& ctx) const override {
        return Converter<M>::read(node, out.*field, ctx);
    }

    M T::* field = nullptr;
};

/**
 * @brief The keys of a struct, filled by the user's `describe(cord::Struct<T>&)`.
 *
 * Only describes the shape: it holds nothing about a read in progress.
 */
template <class T>
class Struct {
public:
    /**
     * @brief Binds a key to a member. An unsupported member type is a compile error.
     */
    template <class M>
    void add(std::string_view key, M T::* field) {
        // A plain bool, or compilers follow the message with a long explanation of the concept.
        constexpr bool convertible = Convertible<M>;
        static_assert(convertible, CORD_COMPILE_ERROR(
            "This member's type can't be read.\n"
            "For a struct, declare its describe(cord::Struct<T>&) before using it.\n"
            "Other supported types: bool, integers, floats, std::string, std::vector, std::optional."));

        // Keeps a bad type from adding more errors after the one above.
        if constexpr (convertible) {
            auto member = std::make_unique<Member<T, M>>();
            member->key = key;
            member->field = field;
            _members.push_back(std::move(member));
        }
    }

    /**
     * @brief Fills out from an object's entries, reporting problems to the context.
     *
     * An absent key warns and keeps the member's initializer. An unknown key warns and is skipped.
     * For a duplicate key the last one wins, and each repeat warns.
     *
     * @return false if a value could not be converted. The others are still read and reported.
     */
    bool read(const Object& object, T& out, Context& ctx) const {
        _warnAboutEntries(object, ctx);

        bool ok = true;
        for (const auto& member : _members) {
            const Entry* last = _findLast(object, member->key);

            ctx.pushKey(member->key);
            if (last == nullptr) ctx.warning("Absent key, keeping its default");
            else if (!member->read(last->value, out, ctx)) ok = false;
            ctx.popPath();
        }
        return ok;
    }

private:
    std::vector<std::unique_ptr<MemberBase<T>>> _members;

    void _warnAboutEntries(const Object& object, Context& ctx) const {
        std::unordered_set<std::string_view> seen;
        for (const Entry& entry : object) {
            ctx.pushKey(entry.key);
            if (!_hasMember(entry.key)) ctx.warning(entry.value, "Unknown key, skipped");
            else if (!seen.insert(entry.key).second) ctx.warning(entry.value, "Duplicate key, the last one wins");
            ctx.popPath();
        }
    }

    bool _hasMember(std::string_view key) const {
        for (const auto& member : _members) {
            if (member->key == key) return true;
        }
        return false;
    }

    static const Entry* _findLast(const Object& object, std::string_view key) {
        const Entry* last = nullptr;
        for (const Entry& entry : object) {
            if (entry.key == key) last = &entry;
        }
        return last;
    }
};

} // namespace cord
