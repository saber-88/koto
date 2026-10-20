#include "koto/app.hpp"
#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/event.hpp"
#include "koto/frame.hpp"
#include "koto/gauge.hpp"
#include "koto/layout.hpp"
#include "koto/list.hpp"
#include "koto/text_area.hpp"
#include "koto/text_input.hpp"
#include <koto/koto.hpp>
#include <string>
#include <termios.h>
#include <vector>

using namespace koto;

int main()
{
    App app;
    ListState list;
    TextAreaState text_area_state;
    bool text_area_focus = false;
    list.items = {"one","two","abracadabrahahahah","four"};
    list.navigation = ListNavigation::Wrap;
    list.set_selected(0);

    
    app.on_key([&](const KeyEvent& key){
        if (key.ctrl && (key.ch == 'q' || key.ch == 'Q')) {
            app.quit();
        }
        else if (!text_area_focus && (key.ch == 'q' || key.ch == 'Q')) {
            app.quit();
        }

        if (text_area_focus) {
            text_area_state.handle_event(key);
        }
        else {
            list.handle_event(key);
        }

        if (key.is(Key::Right) || key.is(Key::Tab)) {
            text_area_focus = true;
        }
        if (key.is(Key::Escape) || key.is(Key::BackTab)) {
            text_area_focus = false;
        }
    });

    app.on_paste([&](const std::string& text) {
        if (text_area_focus) {
            text_area_state.insert(text);
        }
    });
    app.on_render([&](Frame& frame){

        auto sections = Layout::vertical(frame.area(), {
            Constraint::fixed(4), // topbar
            Constraint::fill(),   // middle area
            Constraint::fixed(3),  // bottom bar
            Constraint::fixed(2)
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

        TextArea(text_area_state)
            .title("Text Area")
            .title_align(TitleAlign::Center)
            .line_numbers(true)
            .focused(text_area_focus)
            .draw(frame, horizontal[1])
        ;


        Box()
            .border_style(BorderStyle::Rounded)
            .title("Bottom Bar")
            .title_align(TitleAlign::Left)
            .draw(frame, sections[2])
        ;

        Gauge()
            .ratio(0.75f)
            .filled_fg(Color::lerp(Color::Green, Color::Red, 0.5))
            .draw(frame, sections[3])
        ;

    });

    app.run();
}