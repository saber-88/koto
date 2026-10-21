#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>

namespace koto
{
    inline constexpr int TAG_NONE = -1;

    // =========================================================================
    // FNV-1a 32-bit Compile-Time Hash Function
    //
    // CONCEPT: constexpr
    // Evaluated entirely by the compiler at compile-time when passed literals.
    // Zero runtime CPU cycles, zero heap allocation.
    // =========================================================================
    constexpr uint32_t fnv1a_32(std::string_view str, uint32_t seed = 0x811c9dc5u) noexcept
    {
        uint32_t hash = seed;
        for (char c : str)
        {
            hash ^= static_cast<uint8_t>(c);
            hash *= 0x01000193u;
        }
        return hash;
    }

    constexpr uint32_t fnv1a_32(uint64_t val, uint32_t seed = 0x811c9dc5u) noexcept
    {
        uint32_t hash = seed;
        for (size_t i = 0; i < 8; ++i)
        {
            uint8_t byte = static_cast<uint8_t>((val >> (i * 8)) & 0xFF);
            hash ^= byte;
            hash *= 0x01000193u;
        }
        return hash;
    }

    // =========================================================================
    // Universal Id Token
    //
    // Identifies any UI element:
    // - From a string: Id("sidebar")
    // - From a pointer: Id(&my_state) (unique by C++ memory model)
    // - From an enum: Id(Panel::Editor)
    // - From an integer: Id(42)
    // - From a string + index: Id("card", index)
    // =========================================================================
    class Id
    {
    public:
        // Default constructor: creates an empty/none ID (TAG_NONE = -1)
        constexpr Id() noexcept : m_value(static_cast<uint32_t>(TAG_NONE)) {}

        // Construct from raw uint32_t or int (non-explicit for seamless numeric tag passing)
        constexpr Id(uint32_t raw_val) noexcept : m_value(raw_val) {}
        constexpr Id(int raw_val) noexcept : m_value(static_cast<uint32_t>(raw_val)) {}

        // Construct from string literal or string_view (hashed at compile-time when literal)
        constexpr Id(std::string_view str) noexcept : m_value(fnv1a_32(str)) {}
        constexpr Id(const char* str) noexcept : m_value(fnv1a_32(std::string_view(str))) {}

        // Multi-part ID: combines a string name with an index (e.g. Id("item", i))
        constexpr Id(std::string_view str, size_t index) noexcept
            : m_value(fnv1a_32(static_cast<uint64_t>(index), fnv1a_32(str))) {}

        // 2D Matrix / Grid ID: combines a string name with row and column (e.g. Id("cell", r, c))
        constexpr Id(std::string_view str, size_t row, size_t col) noexcept
            : m_value(fnv1a_32(static_cast<uint64_t>(col), fnv1a_32(static_cast<uint64_t>(row), fnv1a_32(str)))) {}

        // Construct from any object pointer (hashes memory address, collision-proof)
        Id(const void* ptr) noexcept
            : m_value(fnv1a_32(reinterpret_cast<uintptr_t>(ptr))) {}

        // CONCEPT: template with requires constraint (C++20)
        // Enables Id to be constructed from ANY enum class without casting!
        template<typename Enum>
        requires std::is_enum_v<Enum>
        constexpr Id(Enum e) noexcept
            : m_value(static_cast<uint32_t>(e)) {}

        // Combine two IDs (used for scoping: scope_id.combine(base_id))
        constexpr Id combine(Id other) const noexcept
        {
            return Id(fnv1a_32(static_cast<uint64_t>(other.m_value), m_value));
        }

        // Accessors
        constexpr uint32_t value() const noexcept { return m_value; }
        constexpr bool is_none() const noexcept { return m_value == static_cast<uint32_t>(TAG_NONE); }
        constexpr explicit operator bool() const noexcept { return !is_none(); }
        constexpr explicit operator int() const noexcept { return static_cast<int>(m_value); }

        // CONCEPT: C++20 Defaulted Equality Operator
        // Automatically compares m_value == other.m_value
        constexpr bool operator==(const Id& other) const noexcept = default;
        constexpr bool operator==(int val) const noexcept { return static_cast<int>(m_value) == val; }
        constexpr bool operator==(uint32_t val) const noexcept { return m_value == val; }

        // Static factory for explicitly creating an empty ID
        static constexpr Id none() noexcept { return Id(); }

    private:
        uint32_t m_value = static_cast<uint32_t>(TAG_NONE);
    };
}
