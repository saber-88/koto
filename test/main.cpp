#include "koto/app.hpp"
#include "koto/box.hpp"
#include "koto/event.hpp"
#include "koto/frame.hpp"
#include "koto/layout.hpp"
#include "koto/list.hpp"
#include <koto/koto.hpp>
#include <string>
#include <termios.h>
#include <vector>

using namespace koto;

int main()
{
    App app;
    ListState list;
    list.items = {"one","two","abracadabrahahahah","four"};
    list.navigation = ListNavigation::Wrap;
    list.set_selected(0);

    
    app.on_key([&](const KeyEvent& key){
        if (key.ch == 'q' || key.ch == 'Q') {
            app.quit();
        }
        if (list.handle_event(key)) {
            
        }

    });
    app.on_render([&](Frame& frame){

        auto sections = Layout::vertical(frame.area(), {
            Constraint::fixed(4), // topbar
            Constraint::fill(),   // middle area
            Constraint::fixed(3)  // bottom bar
        });

        Box()
            .border_style(BorderStyle::Rounded)
            .title("Top Bar")
            .title_align(TitleAlign::Left)
            .draw(frame, sections[0])
        ;

        auto horizontal = Layout::horizontal(sections[1], {
            Constraint::fixed(10), // list
            Constraint::fill() // text area
        });

        List(list)
            .border_style(BorderStyle::Thick)
            .highlight_fg("#FF0033")
            .draw(frame, horizontal[0])
            
        ;

        Box()
            .border_style(BorderStyle::Rounded)
            .title("Text area")
            .title_align(TitleAlign::Center)
            .draw(frame, horizontal[1])
        ;

        Box()
            .border_style(BorderStyle::Rounded)
            .title("Bottom Bar")
            .title_align(TitleAlign::Left)
            .draw(frame, sections[2])
        ;

    });

    app.run();
}