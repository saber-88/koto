#include "koto/app.hpp"
#include "koto/focus.hpp"
#include "koto/frame.hpp"
#include <koto/koto.hpp>
#include <array>

using namespace koto;

int main()
{
    App app;
    ListState hosts({"alpha", "beta", "gamma"});
    hosts.set_selected(1);
    std::vector<uint64_t> load = {2, 4, 3, 6, 8, 7, 5, 9, 6, 4, 7, 10};
    
    FocusManager fm;
    fm.navigation_policy(FocusNavPolicy::SpatialArrows);
    fm.add(hosts);
    app.use_focus(fm);

    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.ch == 'Q' || (key.ctrl && (key.ch == 'c' || key.ch == 'C' || key.ch == 'q' || key.ch == 'Q')))
        {
            app.quit();
        }
    });

    app.on_render([&](Frame& frame){

        auto [left, right] = Layout::horizontal(frame.area(), {Constraint::percent(30), Constraint::fill()});
        List(hosts).title("Hosts").highlight_symbol("> ").border_style(BorderStyle::Rounded).draw(frame, left);
    
        auto [top, bottom] = Layout::vertical(right, {Constraint::fixed(3), Constraint::fill()});
        Box().title("CPU").border_style(BorderStyle::Rounded).draw(frame, top);
        Gauge().percent(64).label("64%").draw(frame, top.inset(1, 1, 1, 1));
        Box().title("Load").border_style(BorderStyle::Rounded).draw(frame, bottom);
        Sparkline().data(load).draw(frame, bottom.inset(1, 1, 1, 1));
    });

    auto res = app.run();
    if (!res)
    {
        std::fprintf(stderr, "Error: %.*s (errno: %d)\n",
                     static_cast<int>(res.error().message().size()),
                     res.error().message().data(),
                     res.error().system_errno);
        return 1;
    }
    return 0;
   
}
