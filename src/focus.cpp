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
                // Collision detection logic (N1 & N2 fix):
                // If neither target has an explicit custom ID (both auto-derived from pointer Id(&state)),
                // and both have non-null state pointers pointing to DIFFERENT addresses,
                // that indicates a true 32-bit hash collision between distinct state objects!
                if (!m_targets[i].has_custom_id && !target.has_custom_id &&
                    m_targets[i].state_ptr != nullptr && target.state_ptr != nullptr)
                {
                    assert(m_targets[i].state_ptr == target.state_ptr &&
                           "FocusManager: 32-bit Id hash collision detected between two distinct state objects!");
                }

                m_targets[i].bounds = target.bounds;
                m_targets[i].modal_scope = target.modal_scope;
                m_targets[i].set_focus = std::move(target.set_focus);
                m_targets[i].handle_key = std::move(target.handle_key);
                m_targets[i].handle_mouse = std::move(target.handle_mouse);
                m_targets[i].handle_paste = std::move(target.handle_paste);
                m_targets[i].on_focus = std::move(target.on_focus);
                m_targets[i].on_blur = std::move(target.on_blur);
                m_targets[i].state_ptr = target.state_ptr;
                m_targets[i].has_custom_id = target.has_custom_id;

                // Re-sync focus state (C-35)
                if (m_targets[i].set_focus)
                {
                    m_targets[i].set_focus(i == m_active_index);
                }

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
        // activate this modal target immediately (respecting validator/observer) (C-36)!
        if (!current_modal.is_none() && target.modal_scope == current_modal)
        {
            bool active_is_in_modal = (m_active_index < m_targets.size() &&
                                       m_targets[m_active_index].modal_scope == current_modal);
            if (!active_is_in_modal)
            {
                size_t new_idx = m_targets.size();
                m_targets.push_back(std::move(target));
                this->focus(new_idx);
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

        if (!on_mouse && on_key)
        {
            on_mouse = [key_fn = on_key](const MouseEvent& m) {
                if (m.is_left_press())
                {
                    KeyEvent enter_key{static_cast<uint32_t>(Key::Enter)};
                    return key_fn(enter_key);
                }
                return false;
            };
        }

        FocusTarget target{
            .id = scoped,
            .bounds = Area{0, 0, 0, 0},
            .modal_scope = current_modal,
            .enabled = true,
            .visible = true,
            .set_focus = nullptr,
            .handle_key = std::move(on_key),
            .handle_mouse = std::move(on_mouse),
            .handle_paste = nullptr,
            .on_focus = nullptr,
            .on_blur = nullptr
        };

        this->register_target(std::move(target));
        return *this;
    }

    bool FocusManager::has_geometry() const noexcept
    {
        for (const auto& t : m_targets)
        {
            if (t.bounds.is_valid() && t.bounds.width > 0 && t.bounds.height > 0)
            {
                return true;
            }
        }
        return false;
    }

    bool FocusManager::remove(Id id)
    {
        Id scoped = this->scoped_id(id);
        for (auto it = m_targets.begin(); it != m_targets.end(); ++it)
        {
            if (it->id == scoped || it->id == id)
            {
                size_t idx = static_cast<size_t>(std::distance(m_targets.begin(), it));
                if (idx == m_active_index)
                {
                    if (it->set_focus) it->set_focus(false);
                    if (it->on_blur) it->on_blur();
                }
                m_targets.erase(it);
                if (m_active_index >= m_targets.size() && !m_targets.empty())
                {
                    m_active_index = m_targets.size() - 1;
                    if (m_targets[m_active_index].set_focus) m_targets[m_active_index].set_focus(true);
                }
                return true;
            }
        }
        return false;
    }

    FocusManager& FocusManager::set_visible(Id id, bool visible)
    {
        Id scoped = this->scoped_id(id);
        for (auto& t : m_targets)
        {
            if (t.id == scoped || t.id == id)
            {
                t.visible = visible;
                break;
            }
        }
        return *this;
    }

    FocusManager& FocusManager::set_enabled(Id id, bool enabled)
    {
        Id scoped = this->scoped_id(id);
        for (auto& t : m_targets)
        {
            if (t.id == scoped || t.id == id)
            {
                t.enabled = enabled;
                break;
            }
        }
        return *this;
    }

    bool FocusManager::is_visible(Id id) const
    {
        Id scoped = this->scoped_id(id);
        for (const auto& t : m_targets)
        {
            if (t.id == scoped || t.id == id)
            {
                return t.visible;
            }
        }
        return false;
    }

    bool FocusManager::is_enabled(Id id) const
    {
        Id scoped = this->scoped_id(id);
        for (const auto& t : m_targets)
        {
            if (t.id == scoped || t.id == id)
            {
                return t.enabled;
            }
        }
        return false;
    }

    bool FocusManager::next()
    {
        if (m_targets.empty())
        {
            return false;
        }

        Id current_modal = this->active_modal_scope();
        bool req_geom = this->has_geometry();

        size_t next_idx = m_active_index;
        for (size_t step = 1; step <= m_targets.size(); ++step)
        {
            size_t cand_idx = (m_active_index + step) % m_targets.size();
            const auto& cand = m_targets[cand_idx];
            if (cand.modal_scope == current_modal && cand.is_focusable(req_geom))
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
        bool req_geom = this->has_geometry();

        size_t prev_idx = m_active_index;
        for (size_t step = 1; step <= m_targets.size(); ++step)
        {
            size_t cand_idx = (m_active_index + m_targets.size() - step) % m_targets.size();
            const auto& cand = m_targets[cand_idx];
            if (cand.modal_scope == current_modal && cand.is_focusable(req_geom))
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

        Id current_modal = this->active_modal_scope();
        if (!current_modal.is_none() && m_targets[index].modal_scope != current_modal)
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

        // If the currently focused widget was hidden or collapsed off-screen,
        // automatically advance focus to a visible target
        if (m_active_index < m_targets.size() && this->has_geometry())
        {
            if (!m_targets[m_active_index].is_focusable(true))
            {
                this->next();
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
        // 1. Run developer's custom FocusKeyInterceptor FIRST if registered!
        if (m_key_interceptor && m_key_interceptor(*this, key))
        {
            return true;
        }

        if (m_targets.empty())
        {
            return false;
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

    bool FocusManager::handle_paste(const std::string& text)
    {
        if (m_targets.empty() || m_active_index >= m_targets.size())
        {
            return false;
        }

        auto& target = m_targets[m_active_index];
        if (target.handle_paste)
        {
            return target.handle_paste(text);
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
                    if (m_targets[i].handle_mouse)
                    {
                        m_targets[i].handle_mouse(mouse);
                    }
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
            Id saved_id = (m_active_index < m_targets.size()) ? m_targets[m_active_index].id : Id::none();
            m_saved_focus_stack.push_back(saved_id);
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

        ScopeEntry popped = m_scope_stack.back();
        m_scope_stack.pop_back();

        if (popped.mode == ScopeMode::Modal)
        {
            Id popped_id(popped.id);
            // Prune all targets that belonged specifically to this popped modal scope
            std::erase_if(m_targets, [popped_id](const FocusTarget& t) {
                return t.modal_scope == popped_id;
            });

            if (m_active_index >= m_targets.size() && !m_targets.empty())
            {
                m_active_index = m_targets.size() - 1;
            }

            if (!m_saved_focus_stack.empty())
            {
                Id restored_id = m_saved_focus_stack.back();
                m_saved_focus_stack.pop_back();
                if (!restored_id.is_none())
                {
                    this->focus_id(restored_id);
                }
            }
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
