#include "koto/focus.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace koto
{
    // =========================================================================
    // IdScope Implementation (RAII Guard)
    // =========================================================================

    IdScope::IdScope(FocusManager& fm, Id scope_id, ScopeMode mode)
        : m_fm(&fm)
    {
        m_fm->push_scope(scope_id, mode);
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

    void FocusManager::register_target(FocusTarget target)
    {
        Id current_modal = this->active_modal_scope();

        if (m_targets.empty())
        {
            if (target.set_focus) target.set_focus(true);
            if (target.on_focus) target.on_focus();
            m_targets.push_back(std::move(target));
            m_active_index = 0;
            return;
        }

        // If inside a modal scope, check if this is the first target in the modal
        if (!current_modal.is_none())
        {
            bool first_in_modal = true;
            for (const auto& t : m_targets)
            {
                if (t.modal_scope == current_modal)
                {
                    first_in_modal = false;
                    break;
                }
            }
            if (first_in_modal)
            {
                // Deactivate old active target
                if (m_active_index < m_targets.size())
                {
                    auto& old_t = m_targets[m_active_index];
                    if (old_t.set_focus) old_t.set_focus(false);
                    if (old_t.on_blur) old_t.on_blur();
                }
                m_active_index = m_targets.size();
                if (target.set_focus) target.set_focus(true);
                if (target.on_focus) target.on_focus();
                m_targets.push_back(std::move(target));
                return;
            }
        }

        if (target.set_focus) target.set_focus(false);
        m_targets.push_back(std::move(target));
    }

    FocusManager& FocusManager::add(Id id, std::function<bool(const KeyEvent&)> on_key)
    {
        Id scoped = this->scoped_id(id);
        Id current_modal = this->active_modal_scope();

        FocusTarget target{
            .id = scoped,
            .bounds = Area{0, 0, 0, 0},
            .modal_scope = current_modal,
            .set_focus = nullptr,
            .handle_key = std::move(on_key),
            .on_focus = nullptr,
            .on_blur = nullptr
        };

        this->register_target(std::move(target));
        return *this;
    }

    void FocusManager::next()
    {
        if (m_targets.empty())
        {
            return;
        }

        Id current_modal = this->active_modal_scope();

        size_t next_idx = m_active_index;
        for (size_t step = 1; step <= m_targets.size(); ++step)
        {
            size_t cand_idx = (m_active_index + step) % m_targets.size();
            if (m_targets[cand_idx].modal_scope == current_modal)
            {
                next_idx = cand_idx;
                break;
            }
        }

        if (next_idx != m_active_index)
        {
            this->focus(next_idx);
        }
    }

    void FocusManager::prev()
    {
        if (m_targets.empty())
        {
            return;
        }

        Id current_modal = this->active_modal_scope();

        size_t prev_idx = m_active_index;
        for (size_t step = 1; step <= m_targets.size(); ++step)
        {
            size_t cand_idx = (m_active_index + m_targets.size() - step) % m_targets.size();
            if (m_targets[cand_idx].modal_scope == current_modal)
            {
                prev_idx = cand_idx;
                break;
            }
        }

        if (prev_idx != m_active_index)
        {
            this->focus(prev_idx);
        }
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

        Id scoped = this->scoped_id(id);

        for (size_t i = 0; i < m_targets.size(); ++i)
        {
            if (m_targets[i].id == id || m_targets[i].id == scoped)
            {
                return this->focus(i);
            }
        }
        return false;
    }

    void FocusManager::sync_geometry(const Frame& frame)
    {
        for (auto& target : m_targets)
        {
            auto bounds = frame.bounds_of(target.id);
            if (bounds.has_value())
            {
                target.bounds = *bounds;
            }
            else
            {
                target.bounds = Area{0, 0, 0, 0};
            }
        }
    }

    bool FocusManager::move(Direction dir)
    {
        if (m_targets.empty() || m_active_index >= m_targets.size())
        {
            return false;
        }

        const auto& current = m_targets[m_active_index];
        if (!current.bounds.is_valid())
        {
            return false;
        }

        Id current_modal = this->active_modal_scope();

        int best_index = -1;
        int best_score = std::numeric_limits<int>::max();

        int cur_mid_r = current.bounds.center_row();
        int cur_mid_c = current.bounds.center_col();

        for (size_t i = 0; i < m_targets.size(); ++i)
        {
            if (i == m_active_index) continue;
            const auto& cand = m_targets[i];
            if (!cand.bounds.is_valid()) continue;

            // Modal isolation: candidate must belong to the active modal scope!
            if (cand.modal_scope != current_modal) continue;

            int cand_mid_r = cand.bounds.center_row();
            int cand_mid_c = cand.bounds.center_col();

            int primary_dist = 0;
            int secondary_dist = 0;
            bool in_direction = false;

            switch (dir)
            {
                case Direction::Right:
                    if (cand_mid_c > cur_mid_c)
                    {
                        in_direction = true;
                        primary_dist = std::max(0, cand.bounds.col - current.bounds.right());
                        secondary_dist = std::abs(cand_mid_r - cur_mid_r);
                    }
                    break;
                case Direction::Left:
                    if (cand_mid_c < cur_mid_c)
                    {
                        in_direction = true;
                        primary_dist = std::max(0, current.bounds.col - cand.bounds.right());
                        secondary_dist = std::abs(cand_mid_r - cur_mid_r);
                    }
                    break;
                case Direction::Down:
                    if (cand_mid_r > cur_mid_r)
                    {
                        in_direction = true;
                        primary_dist = std::max(0, cand.bounds.row - current.bounds.bottom());
                        secondary_dist = std::abs(cand_mid_c - cur_mid_c);
                    }
                    break;
                case Direction::Up:
                    if (cand_mid_r < cur_mid_r)
                    {
                        in_direction = true;
                        primary_dist = std::max(0, current.bounds.row - cand.bounds.bottom());
                        secondary_dist = std::abs(cand_mid_c - cur_mid_c);
                    }
                    break;
            }

            if (in_direction)
            {
                int score = primary_dist + 2 * secondary_dist;
                if (score < best_score)
                {
                    best_score = score;
                    best_index = static_cast<int>(i);
                }
            }
        }

        if (best_index != -1)
        {
            return this->focus(static_cast<size_t>(best_index));
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
        Id scoped = this->scoped_id(id);
        return m_targets[m_active_index].id == id || m_targets[m_active_index].id == scoped;
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
        m_scope_stack.clear();
        m_saved_focus_stack.clear();
    }

    bool FocusManager::handle_key(const KeyEvent& key)
    {
        if (m_targets.empty())
        {
            return false;
        }

        // 1. Tab / Shift+Tab -> 1D ring navigation
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

        // 2. Focused target key handler (e.g. typing into TextInput, custom callbacks)
        if (m_active_index < m_targets.size() && m_targets[m_active_index].handle_key)
        {
            if (m_targets[m_active_index].handle_key(key))
            {
                return true;
            }
        }

        // 3. Arrow keys -> Spatial 2D navigation (if unconsumed by target)
        if (key.is(Key::Up))
        {
            return this->move(Direction::Up);
        }
        if (key.is(Key::Down))
        {
            return this->move(Direction::Down);
        }
        if (key.is(Key::Left))
        {
            return this->move(Direction::Left);
        }
        if (key.is(Key::Right))
        {
            return this->move(Direction::Right);
        }

        return false;
    }

    bool FocusManager::handle_mouse_click(int row, int col, const Frame& frame)
    {
        Id hit = frame.hit_at(row, col);
        if (hit.is_none())
        {
            return false;
        }

        Id current_modal = this->active_modal_scope();

        for (size_t i = 0; i < m_targets.size(); ++i)
        {
            if (m_targets[i].id == hit)
            {
                // If a modal is active, clicks outside the modal are blocked!
                if (!current_modal.is_none() && m_targets[i].modal_scope != current_modal)
                {
                    return true; // Intercepted & shielded!
                }
                return this->focus(i);
            }
        }

        // Click hit an element not in targets (e.g. backdrop or non-focusable box)
        if (!current_modal.is_none())
        {
            return true; // Absorb click inside modal backdrop
        }

        return false;
    }

    IdScope FocusManager::scope(Id scope_id)
    {
        return IdScope(*this, scope_id, ScopeMode::Normal);
    }

    IdScope FocusManager::modal_scope(Id scope_id)
    {
        return IdScope(*this, scope_id, ScopeMode::Modal);
    }

    void FocusManager::push_scope(Id scope_id, ScopeMode mode)
    {
        if (mode == ScopeMode::Modal)
        {
            m_saved_focus_stack.push_back(m_active_index);
        }
        m_scope_stack.push_back(ScopeEntry{scope_id.value(), mode});
    }

    void FocusManager::pop_scope()
    {
        if (m_scope_stack.empty())
        {
            return;
        }

        ScopeMode mode = m_scope_stack.back().mode;
        m_scope_stack.pop_back();

        if (mode == ScopeMode::Modal && !m_saved_focus_stack.empty())
        {
            size_t restored_index = m_saved_focus_stack.back();
            m_saved_focus_stack.pop_back();
            this->focus(restored_index);
        }
    }

    Id FocusManager::scoped_id(Id base_id) const
    {
        if (m_scope_stack.empty())
        {
            return base_id;
        }
        Id cur = base_id;
        for (const auto& s : m_scope_stack)
        {
            cur = Id(s.id).combine(cur);
        }
        return cur;
    }

    Id FocusManager::active_modal_scope() const
    {
        for (auto it = m_scope_stack.rbegin(); it != m_scope_stack.rend(); ++it)
        {
            if (it->mode == ScopeMode::Modal)
            {
                return Id(it->id);
            }
        }
        return Id::none();
    }
}
