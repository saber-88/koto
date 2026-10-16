#include <koto/koto.hpp>
#include <koto/hbox.hpp>
#include <koto/vbox.hpp>

using namespace koto;

int main()
{
    App app;

    app.on_render([&](Frame& frame) {
        Area area = frame.area();
        if (area.height < 5 || area.width < 10) return;

        Vbox vbox;

        // Top bar (Saffron)
        vbox.add<Box>(Constraint::percent(33))
            .bg("#d75c05")
            .border_style(BorderStyle::None)
            .filled(true);

        // Middle row (Hbox with White - Blue - White)
        auto& content = vbox.add<Hbox>(Constraint::fill());
        content.add<Box>(Constraint::percent(40))
            .bg("#ffffff")
            .border_style(BorderStyle::None)
            .filled(true);
        content.add<Box>(Constraint::fill())
            .bg("#0065fe")
            .border_style(BorderStyle::None)
            .filled(true);
        content.add<Box>(Constraint::percent(40))
            .bg("#ffffff")
            .border_style(BorderStyle::None)
            .filled(true);

        // Bottom bar (Green)
        vbox.add<Box>(Constraint::percent(33))
            .bg("#159f00")
            .border_style(BorderStyle::None)
            .filled(true);

        vbox.draw(frame, area);
    });

    app.run();
    return 0;
}