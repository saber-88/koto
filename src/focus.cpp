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

        // 1. If target is already registered (by id), update it in place without duplicating
        for (size_t i = 0; i < m_targets.size(); ++i)
        {
            if (m_targets[i].id == target.id)
            {
                m_targets[i].bounds = target.bounds;
                m_targets[i].modal_scope = target.modal_scope;
                m_targets[i].set_focus = std::move(target.set_focus);
                m_targets[i].handle_key = std::move(target.handle_key);
                m_targets[i].handle_mouse = std::move(target.handle_mouse);
                m_targets[i].on_focus = std::move(target.on_focus);
                m_targets[i].on_blur = std::move(target.on_blur);

                // If inside a modal scope and currently active target is outside the modal,
                // activate this modal target!
                if (!current_modal.is_none() && target.modal_scope == current_modal)
                {
                    bool active_is_in_modal = (m_active_index < m_targets.size() &&
                                               m_targets[m_active_index].modal_scope == current_modal);
                    if (!active_is_in_modal)
                    {
                        this->focus(i);
                    }
                }
                return;
            }
        }

        // 2. New target: if inside a modal and currently active target is outside the modal,
        // activate this modal target immediately!
        if (!current_modal.is_none() && target.modal_scope == current_modal)
        {
            bool active_is_in_modal = (m_active_index < m_targets.size() &&
                                       m_targets[m_active_index].modal_scope == current_modal);
            if (!active_is_in_modal)
            {
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

        if (m_targets.empty())
        {
            if (target.set_focus) target.set_focus(true);
            if (target.on_focus) target.on_focus();
            m_targets.push_back(std::move(target));
            m_active_index = 0;
            return;
        }

        if (target.set_focus) target.set_focus(false);
        m_targets.push_back(std::move(target));
    }

    FocusManager& FocusManager::add(Id id, std::function<bool(const KeyEvent&)> on_key,
                                           std::function<bool(const MouseEvent&)> on_mouse)
    {
        Id scoped = this->scoped_id(id);
        Id current_modal = this->active_modal_scope();

        FocusTarget target{
            .id = scoped,
            .bounds = Area{0, 0, 0, 0},
            .modal_scope = current_modal,
            .set_focus = nullptr,
            .handle_key = std::move(on_key),
            .handle_mouse = std::move(on_mouse),
            .on_focus = nullptr,
            .on_blur = nullptr
        };

        this->register_target(std::move(target));
        return *this;
    }

    bool FocusManager::next()
    {
        if (m_targets.empty())
        {
            return false;
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
            return this->focus(next_idx);
        }
        return false;
    }

    bool FocusManager::prev()
    {
        if (m_targets.empty())
        {
            return false;
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
            return this->focus(prev_idx);
        }
        return false;
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

        Id old_id = (m_active_index < m_targets.size()) ? m_targets[m_active_index].id : Id::none();
        Id new_id = m_targets[index].id;

        // Run validator if registered (can veto focus change!)
        if (m_change_validator && !m_change_validator(*this, old_id, new_id))
        {
            return false;
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

        if (m_change_observer)
        {
            m_change_observer(old_id, new_id);
        }

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

        // 1. Run developer's custom FocusKeyInterceptor FIRST if registered!
        if (m_key_interceptor && m_key_interceptor(*this, key))
        {
            return true;
        }

        // 2. Tab / Shift+Tab -> 1D ring navigation (always consistent and predictable)
        if (key.is(Key::BackTab))
        {
            return this->prev();
        }
        if (key.is(Key::Tab))
        {
            return this->next();
        }

        // 3. Alt+Arrows spatial jumping (if policy is AltArrows)
        if (m_policy == FocusNavPolicy::AltArrows && key.alt)
        {
            if (key.is(Key::Up) || key.ch == static_cast<uint32_t>(Key::AltUp)) return this->move(Direction::Up);
            if (key.is(Key::Down) || key.ch == static_cast<uint32_t>(Key::AltDown)) return this->move(Direction::Down);
            if (key.is(Key::Left) || key.ch == static_cast<uint32_t>(Key::AltLeft)) return this->move(Direction::Left);
            if (key.is(Key::Right) || key.ch == static_cast<uint32_t>(Key::AltRight)) return this->move(Direction::Right);
        }

        // 4. Focused target key handler (typing in TextInput, editor in TextArea, items in List, tabs in Tabs)
        if (m_active_index < m_targets.size() && m_targets[m_active_index].handle_key)
        {
            if (m_targets[m_active_index].handle_key(key))
            {
                return true;
            }
        }

        // 5. Arrow keys -> Spatial 2D navigation (ONLY when policy is SpatialArrows!)
        // In default TabOnly mode, unconsumed arrow keys will NEVER hijack focus away from the widget!
        if (m_policy == FocusNavPolicy::SpatialArrows)
        {
            if (key.is(Key::Up))    return this->move(Direction::Up);
            if (key.is(Key::Down))  return this->move(Direction::Down);
            if (key.is(Key::Left))  return this->move(Direction::Left);
            if (key.is(Key::Right)) return this->move(Direction::Right);
        }

        return false;
    }

    FocusManager& FocusManager::navigation_policy(FocusNavPolicy policy) noexcept
    {
        m_policy = policy;
        return *this;
    }

    FocusManager& FocusManager::enable_spatial_arrows(bool enable) noexcept
    {
        m_policy = enable ? FocusNavPolicy::SpatialArrows : FocusNavPolicy::TabOnly;
        return *this;
    }

    FocusManager& FocusManager::interceptor(FocusKeyInterceptor fn)
    {
        m_key_interceptor = std::move(fn);
        return *this;
    }

    FocusManager& FocusManager::can_focus(FocusChangeValidator fn)
    {
        m_change_validator = std::move(fn);
        return *this;
    }

    FocusManager& FocusManager::on_focus_change(FocusChangeObserver fn)
    {
        m_change_observer = std::move(fn);
        return *this;
    }

    bool FocusManager::handle_mouse(const MouseEvent& mouse, const Frame& frame)
    {
        Id hit = frame.hit_at(mouse.row, mouse.col);
        Id current_modal = this->active_modal_scope();
        Id scoped_hit = this->scoped_id(hit);

        if (hit.is_none())
        {
            // If inside a modal, absorb clicks anywhere outside the modal
            if (!current_modal.is_none() && mouse.action == MouseAction::Press)
            {
                return true;
            }
            return false;
        }

        for (size_t i = 0; i < m_targets.size(); ++i)
        {
            if (m_targets[i].id == hit || m_targets[i].id == scoped_hit)
            {
                // If a modal is active, events on widgets outside the modal are shielded!
                if (!current_modal.is_none() && m_targets[i].modal_scope != current_modal)
                {
                    return true; // Intercepted & shielded!
                }

                // 1. Wheel events: route directly to target widget's mouse handler (e.g. list scroll, tab cycle)
                if (mouse.button == MouseButton::WheelUp || mouse.button == MouseButton::WheelDown)
                {
                    if (m_targets[i].handle_mouse)
                    {
                        return m_targets[i].handle_mouse(mouse);
                    }
                    return false;
                }

                // 2. Click events (Press + Left): shift focus, then forward click to sub-item or button action
                if (mouse.action == MouseAction::Press && mouse.button == MouseButton::Left)
                {
                    this->focus(i);
                    bool sub_handled = false;
                    if (m_targets[i].handle_mouse)
                    {
                        sub_handled = m_targets[i].handle_mouse(mouse);
                    }
                    else if (m_targets[i].handle_key)
                    {
                        // If element has no mouse handler but has a key handler (e.g. Button with on_key callback),
                        // simulate an Enter keypress so clicking the button triggers its action!
                        KeyEvent enter_key{static_cast<uint32_t>(Key::Enter)};
                        sub_handled = m_targets[i].handle_key(enter_key);
                    }
                    static_cast<void>(sub_handled);
                    return true;
                }

                // 3. Other mouse actions (Release, Right click, etc.) forwarded if handler exists
                if (m_targets[i].handle_mouse)
                {
                    return m_targets[i].handle_mouse(mouse);
                }

                return false;
            }
        }

        // Hit an element not in targets (e.g. modal backdrop)
        if (!current_modal.is_none())
        {
            return true; // Absorb inside modal backdrop
        }

        return false;
    }

    bool FocusManager::handle_mouse_click(int row, int col, const Frame& frame)
    {
        MouseEvent m{};
        m.action = MouseAction::Press;
        m.button = MouseButton::Left;
        m.row = row;
        m.col = col;
        return this->handle_mouse(m, frame);
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
            m_scope_stack.push_back(ScopeEntry{scope_id.value(), mode});

            // Automatically transfer focus to the first target inside this modal scope if already registered!
            for (size_t i = 0; i < m_targets.size(); ++i)
            {
                if (m_targets[i].modal_scope == scope_id)
                {
                    this->focus(i);
                    return;
                }
            }
            return;
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
