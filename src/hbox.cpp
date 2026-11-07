#include "koto/hbox.hpp"
#include "koto/frame.hpp"
#include "koto/layout.hpp"
#include <array>
#include <cstddef>
#include <memory>
#include <span>
#include <vector>

namespace koto
{   
    auto Hbox::draw(Frame& frame, Area area) -> void
    {
        if (!area.is_valid() || m_children.empty()) return;

        if (m_children.size() <= LayoutResult::InlineCapacity)
        {
            std::array<Constraint, LayoutResult::InlineCapacity> stack_c;
            for (size_t i = 0; i < m_children.size(); ++i)
            {
                stack_c[i] = m_children[i].constraint;
            }
            auto rect = Layout::horizontal(area, std::span<const Constraint>(stack_c.data(), m_children.size()));
            for (size_t i = 0; i < m_children.size(); ++i)
            {
                m_children[i].widget->draw(frame, rect[i]);
            }
        }
        else
        {
            std::vector<Constraint> constraints;
            constraints.reserve(m_children.size());
            for (const auto& e : m_children)
            {
                constraints.push_back(e.constraint);
            }
            auto rect = Layout::horizontal(area, constraints);
            for (size_t i = 0; i < m_children.size(); ++i)
            {
                m_children[i].widget->draw(frame, rect[i]);
            }
        }
    }
}