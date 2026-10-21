#pragma once

#include "koto/event.hpp"
#include "koto/frame.hpp"
#include "koto/id.hpp"
#include <concepts>
#include <cstddef>
#include <functional>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace koto
{
    // =========================================================================
    // CONCEPT: FocusableState (C++20 Concept Constraint)
    //
    // A type 'T' satisfies FocusableState if:
    // 1. It has a member variable or property '.focused' convertible to bool.
    // 2. It has a method '.handle_event(const KeyEvent&)' returning a bool
    //    (where true = event consumed, false = unconsumed).
    //
    // Built-in matching states: TextInputState, TextAreaState, ListState, TabsState.
    // User custom matching states: ANY user struct satisfying the concept!
    // =========================================================================
    template<typename T>
    concept FocusableState = requires(T& s, const KeyEvent& k) {
        { s.focused } -> std::convertible_to<bool>;
        { s.handle_event(k) } -> std::convertible_to<bool>;
    };

    // Forward declaration of FocusManager for IdScope
    class FocusManager;

    // =========================================================================
    // RAII Scope Guard: IdScope
    //
    // CONCEPT: RAII (Resource Acquisition Is Initialization)
    // Automatically pops the scope when exiting a block { ... }.
    // Prevents ID collision when rendering nested/reusable components.
    // =========================================================================
    class IdScope
    {
    public:
        IdScope(FocusManager& fm, Id scope_id);
        ~IdScope();

        // Non-copyable, movable
        IdScope(const IdScope&) = delete;
        IdScope& operator=(const IdScope&) = delete;
        IdScope(IdScope&& other) noexcept;
        IdScope& operator=(IdScope&& other) noexcept;

    private:
        FocusManager* m_fm = nullptr;
    };

    // =========================================================================
    // FocusTarget: Type-Erased Focus Element
    //
    // CONCEPT: Type Erasure
    // Uses std::function closures to store different types in one std::vector
    // without requiring common base class inheritance or heap-allocated pointers.
    // =========================================================================
    struct FocusTarget
    {
        Id id;
        std::function<void(bool)> set_focus;              // updates .focused on the underlying state
        std::function<bool(const KeyEvent&)> handle_key;  // routes key event to the underlying state
        std::function<void()> on_focus;                   // optional user hook when focus gained
        std::function<void()> on_blur;                    // optional user hook when focus lost
    };

    // =========================================================================
    // FocusManager: Centralized Keyboard Navigation & Mouse Click Focus Engine
    // =========================================================================
    class FocusManager
    {
    public:
        FocusManager() = default;

        // 1. Register a rich state (TextInputState, TextAreaState, ListState, TabsState, or custom struct)
        // CONCEPT: Constrained Template Method
        template<FocusableState T>
        FocusManager& add(T& state, Id custom_id = Id::none())
        {
            Id id = custom_id.is_none() ? Id(&state) : custom_id;
            Id scoped = this->scoped_id(id);

            FocusTarget target{
                .id = scoped,
                .set_focus = [&state](bool f) { state.focused = f; },
                .handle_key = [&state](const KeyEvent& k) { return state.handle_event(k); },
                .on_focus = nullptr,
                .on_blur = nullptr
            };

            // Initial focus to first registered element
            if (m_targets.empty())
            {
                target.set_focus(true);
            }
            else
            {
                target.set_focus(false);
            }

            m_targets.push_back(std::move(target));
            return *this;
        }

        // 2. Register a stateless Box or Panel with an ID and optional key callback
        FocusManager& add(Id id, std::function<bool(const KeyEvent&)> on_key = nullptr);

        // Navigation (Tab / Shift+Tab / Index / Id / State)
        void next();                               // Tab forward
        void prev();                               // Shift+Tab / BackTab backward
        bool focus(size_t index);                  // Focus by 0-based ring index
        bool focus_id(Id id);                      // Focus by Id token (from click or code)

        template<typename T>
        requires FocusableState<std::remove_cvref_t<T>>
        bool focus(const T& state)
        {
            return focus_id(Id(&state));
        }

        // Query
        bool is_focused(Id id) const;              // Check if Id is active

        template<typename T>
        requires FocusableState<std::remove_cvref_t<T>>
        bool is_focused(const T& state) const
        {
            return is_focused(Id(&state));
        }

        size_t current_index() const noexcept { return m_active_index; }
        Id current_id() const noexcept;
        size_t size() const noexcept { return m_targets.size(); }
        bool empty() const noexcept { return m_targets.empty(); }
        void clear();

        // Event Handling
        // Automatically handles Tab / BackTab to cycle focus.
        // Forwards other keys to the active focus target.
        // Returns true if consumed, false if unhandled (passes to global hotkeys).
        bool handle_key(const KeyEvent& key);

        // Mouse click integration: queries Frame hit-buffer at (row, col)
        // If a valid tagged widget was clicked, shifts focus to it and returns true.
        bool handle_mouse_click(int row, int col, const Frame& frame);

        // RAII Scoping
        IdScope scope(Id scope_id);
        void push_scope(Id scope_id);
        void pop_scope();
        Id scoped_id(Id base_id) const;

    private:
        std::vector<FocusTarget> m_targets;
        size_t m_active_index = 0;
        std::vector<uint32_t> m_scope_stack;
    };
}
