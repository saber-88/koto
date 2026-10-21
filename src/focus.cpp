#include "koto/focus.hpp"
#include <algorithm>

namespace koto
{
    // =========================================================================
    // IdScope Implementation (RAII Guard)
    // =========================================================================

    IdScope::IdScope(FocusManager& fm, Id scope_id)
        : m_fm(&fm)
    {
        m_fm->push_scope(scope_id);
    }

    IdScope::~IdScope()
    {
        if (m_fm != nullptr)
        {
            m_fm->pop_scope();
        }
    }

    IdScope::IdScope(IdScope&& other) noexcept
        : m_fm(other.m_fm)
    {
        other.m_fm = nullptr;
    }

    IdScope& IdScope::operator=(IdScope&& other) noexcept
    {
        if (this != &other)
        {
            if (m_fm != nullptr)
            {
                m_fm->pop_scope();
            }
            m_fm = other.m_fm;
            other.m_fm = nullptr;
        }
        return *this;
    }

    // =========================================================================
    // FocusManager Implementation
    // =========================================================================

    FocusManager& FocusManager::add(Id id, std::function<bool(const KeyEvent&)> on_key)
    {
        Id scoped = this->scoped_id(id);

        FocusTarget target{
            .id = scoped,
            .set_focus = nullptr,
            .handle_key = std::move(on_key),
            .on_focus = nullptr,
            .on_blur = nullptr
        };

        m_targets.push_back(std::move(target));
        return *this;
    }

    void FocusManager::next()
    {
        if (m_targets.empty())
        {
            return;
        }

        if (m_active_index < m_targets.size())
        {
            auto& old_target = m_targets[m_active_index];
            if (old_target.set_focus) old_target.set_focus(false);
            if (old_target.on_blur) old_target.on_blur();
        }

        m_active_index = (m_active_index + 1) % m_targets.size();

        auto& new_target = m_targets[m_active_index];
        if (new_target.set_focus) new_target.set_focus(true);
        if (new_target.on_focus) new_target.on_focus();
    }

    void FocusManager::prev()
    {
        if (m_targets.empty())
        {
            return;
        }

        if (m_active_index < m_targets.size())
        {
            auto& old_target = m_targets[m_active_index];
            if (old_target.set_focus) old_target.set_focus(false);
            if (old_target.on_blur) old_target.on_blur();
        }

        m_active_index = (m_active_index + m_targets.size() - 1) % m_targets.size();

        auto& new_target = m_targets[m_active_index];
        if (new_target.set_focus) new_target.set_focus(true);
        if (new_target.on_focus) new_target.on_focus();
    }

    bool FocusManager::focus(size_t index)
    {
        if (index >= m_targets.size())
        {
            return false;
        }

        if (index == m_active_index)
        {
            return true;
        }

        if (m_active_index < m_targets.size())
        {
            auto& old_target = m_targets[m_active_index];
            if (old_target.set_focus) old_target.set_focus(false);
            if (old_target.on_blur) old_target.on_blur();
        }

        m_active_index = index;

        auto& new_target = m_targets[m_active_index];
        if (new_target.set_focus) new_target.set_focus(true);
        if (new_target.on_focus) new_target.on_focus();
        return true;
    }

    bool FocusManager::focus_id(Id id)
    {
        if (id.is_none())
        {
            return false;
        }

        for (size_t i = 0; i < m_targets.size(); ++i)
        {
            if (m_targets[i].id == id)
            {
                return this->focus(i);
            }
        }
        return false;
    }

    bool FocusManager::is_focused(Id id) const
    {
        if (m_targets.empty() || id.is_none())
        {
            return false;
        }
        if (m_active_index >= m_targets.size())
        {
            return false;
        }
        return m_targets[m_active_index].id == id;
    }

    Id FocusManager::current_id() const noexcept
    {
        if (m_targets.empty() || m_active_index >= m_targets.size())
        {
            return Id::none();
        }
        return m_targets[m_active_index].id;
    }

    void FocusManager::clear()
    {
        if (m_active_index < m_targets.size())
        {
            auto& old_target = m_targets[m_active_index];
            if (old_target.set_focus) old_target.set_focus(false);
            if (old_target.on_blur) old_target.on_blur();
        }
        m_targets.clear();
        m_active_index = 0;
    }

    bool FocusManager::handle_key(const KeyEvent& key)
    {
        // 1. Automatic Tab / BackTab focus navigation
        if (key.is(Key::BackTab))
        {
            this->prev();
            return true;
        }
        if (key.is(Key::Tab))
        {
            this->next();
            return true;
        }

        // 2. Route event to currently focused element
        if (m_active_index < m_targets.size() && m_targets[m_active_index].handle_key)
        {
            return m_targets[m_active_index].handle_key(key);
        }

        return false;
    }

    bool FocusManager::handle_mouse_click(int row, int col, const Frame& frame)
    {
        Id hit = frame.hit_at(row, col);
        if (!hit.is_none())
        {
            return this->focus_id(hit);
        }
        return false;
    }

    IdScope FocusManager::scope(Id scope_id)
    {
        return IdScope(*this, scope_id);
    }

    void FocusManager::push_scope(Id scope_id)
    {
        m_scope_stack.push_back(scope_id.value());
    }

    void FocusManager::pop_scope()
    {
        if (!m_scope_stack.empty())
        {
            m_scope_stack.pop_back();
        }
    }

    Id FocusManager::scoped_id(Id base_id) const
    {
        if (m_scope_stack.empty())
        {
            return base_id;
        }
        Id cur = base_id;
        for (uint32_t s : m_scope_stack)
        {
            cur = Id(s).combine(cur);
        }
        return cur;
    }
}
