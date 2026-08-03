#pragma once

#include "layout.hpp"
#include "widget.hpp"
#include <memory>
#include <vector>


namespace koto
{
    class Hbox : public Widget
    {
        public:

            template<typename T,typename... Args>
            T& add(Constraint c, Args&&... args)
            {
                auto widget = std::make_unique<T>(std::forward<Args>(args)...);

                T& ref = *widget;

                m_children.push_back({std::move(widget) , c});

                return ref;
            }

            auto draw(Frame &frame, Area area) -> void override;


        
        private:
            struct Entry
            {
                std::unique_ptr<Widget> widget;
                Constraint constraint;
            };
            std::vector<Entry> m_children;
    };
}