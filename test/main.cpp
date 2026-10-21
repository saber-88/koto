#include "koto/app.hpp"
#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/event.hpp"
#include "koto/focus.hpp"
#include "koto/frame.hpp"
#include "koto/gauge.hpp"
#include "koto/id.hpp"
#include "koto/layout.hpp"
#include "koto/list.hpp"
#include "koto/text_area.hpp"
#include <koto/koto.hpp>
#include <string>
#include <termios.h>
#include <vector>

using namespace koto;

int main()
{
    App app;
    ListState list_s;
    TextAreaState text_area_s;
    FocusManager fm;
    fm.add(list_s).add(text_area_s).add(Id("bottom_bar"));
    
    list_s.items = {"one","two","abracadabrahahahah","four"};
    list_s.navigation = ListNavigation::Wrap;
    list_s.set_selected(0);

    float progress = 0.0f;

    app.use_focus(fm);
    app.on_tick([&](){
        progress += 0.003f;
        if(progress > 1.0f){
            progress = 0.0f;
        }
    });
    app.on_key([&](const KeyEvent& key){
        if (key.ctrl && (key.ch == 'q' || key.ch == 'Q')) {
            app.quit();
        }
        
    });

    app.on_paste([&](const std::string& text) {
        if (fm.is_focused(text_area_s)) {
            text_area_s.insert(text);
        }
    });
    app.on_render([&](Frame& frame){

        auto sections = Layout::vertical(frame.area(), {
            Constraint::fixed(4), // topbar
            Constraint::fill(),   // middle area
            Constraint::fixed(3),  // bottom bar
            Constraint::fixed(3)
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

        List(list_s)
            .border_style(BorderStyle::Thick)
            .highlight_fg("#FF0033")
            .draw(frame, horizontal[0])
            
        ;

        TextArea(text_area_s)
            .title("Text Area")
            .title_align(TitleAlign::Center)
            .line_numbers(true)
            .focused_border_fg(Color::Red)
            .draw(frame, horizontal[1])
        ;


        Box()
            .id("bottom_bar")
            .border_style(BorderStyle::Rounded)
            .title("Bottom Bar")
            .title_align(TitleAlign::Left)
            .draw(frame, sections[2])
        ;
        Box()
            .border_style(BorderStyle::Thick)
            .title("Gauge")
            .title_align(TitleAlign::Center)
            .draw(frame, sections[3])
        ;
        Gauge()
            .ratio(progress)
            .filled_fg(Color::lerp(Color::Green, Color::Red, 0.5))
            .draw(frame, sections[3].inset(1, 1, 1, 1))
        ;

    });

    app.run();
}