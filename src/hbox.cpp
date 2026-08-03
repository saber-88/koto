#include "koto/hbox.hpp"
#include "koto/frame.hpp"
#include "koto/layout.hpp"
#include <memory>
#include <vector>

namespace koto
{   

    auto Hbox::draw(Frame& frame, Area area) -> void
    {
        if (!area.is_valid()) return;

        std::vector<Constraint> constraints;
        for (auto& e : m_children)
        {
            constraints.push_back(e.constraint);
        }

        auto rect = Layout::horizontal(area, constraints);

        for (int i {0}; i < m_children.size(); i++)
        {
            m_children[i].widget->draw(frame, rect[i]);
        }
    }
}