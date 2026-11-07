#include "koto/focus.hpp"
#include <koto/koto.hpp>
#include <array>

using namespace koto;

int main()
{
    App app;
    FocusManager fm;
    app.use_focus(fm); // 1. Connect FocusManager (enables mouse tracking & event routing)

    // 2. State: 5x5 grid of booleans (true = active '#', false = empty ' ')
    std::array<std::array<bool, 5>, 5> active = {};

    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.ch == 'Q') {
            app.quit();
        }
    });
    fm.navigation_policy(FocusNavPolicy::SpatialArrows);

    app.on_render([&](Frame& frame) {
        // Center a 60-col x 15-row box on the screen
        auto center_area = Layout::center(app.area().inset(1, 1, 1, 1), 60, 15);
        auto grid = Layout::grid_uniform(center_area, 5, 5);

        grid.for_each([&](size_t r, size_t c, const Area& square) {
            // 3. Show "#" when clicked, otherwise blank " "
            std::string label = active[r][c] ? "#" : " ";

            Button(label)
                .id(Id("cell", r, c))       // 4. Unique ID per cell
                .focus(fm)                  // Connect to FocusManager
                .border_style(BorderStyle::Rounded)
                .fg(Color::DarkGray)
                .bg(active[r][c] ? Color::Rgb(60, 20, 20) : Color::Rgb(20, 20, 20))
                .label_fg(Color::Yellow)
                .focused_border_fg(Color::Emerald)
                .on_click([&, r, c] {       // 5. Toggle cell on click!
                    active[r][c] = !active[r][c];
                })
                .draw(frame, square);
        });
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
