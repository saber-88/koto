#include "koto/box.hpp"
#include "koto/color.hpp"
#include "koto/tabs.hpp"
#include <koto/koto.hpp>
#include <string>
#include <vector>

using namespace koto;

int main()
{
    App app;

    const std::vector<std::string> tab_titles = {
        "1: 日本語 (Japanese)",
        "2: 中文 (Chinese)",
        "3: 한국어 (Korean)",
        "4: 🚀 Emojis (絵文字)",
        "5: 📏 Alignment Ruler"
    };

    TabsState tab_state(tab_titles);
    TextInputState input("琴 (koto) - Modern C++23 TUI 🚀");

    app.on_key([&](const KeyEvent& key) {
        if (key.ctrl && (key.ch == 'q' || key.ch == 'Q'))
        {
            app.quit();
            return;
        }

        // Tab switching via numbers 1-5, F1/F2, or Tab/BackTab
        if (!key.ctrl && !key.alt && key.ch >= '1' && key.ch <= '5')
        {
            tab_state.set_selected(static_cast<size_t>(key.ch - '1'));
            return;
        }
        if (key.is(Key::F1) || key.is(Key::BackTab))
        {
            tab_state.select_prev();
            return;
        }
        if (key.is(Key::F2) || key.is(Key::Tab))
        {
            tab_state.select_next();
            return;
        }

        // Otherwise pass keys to interactive TextInput
        input.handle_event(key);
    });

    app.on_render([&](Frame& frame) {
        Area full = frame.area();
        if (full.width < 50 || full.height < 20)
        {
            Text("Terminal too small! Please resize to at least 50x20.").align(TextAlign::Center).draw(frame, full);
            return;
        }

        // Main outer frame
        Box()
            .border_style(BorderStyle::Rounded)
            .title(" koto CJK & Emoji Terminal Inspector ")
            .title_align(TitleAlign::Left)
            .fg(Color::Cyan)
            .bg(Color::Rgb(15, 17, 23))
            .filled(true)
            .draw(frame, full);

        Area content = full.inset(1, 2, 1, 2);

        auto rows = Layout::vertical(content, {
            Constraint::fixed(2), // Tab navigation bar (Underline style is 2 rows tall)
            Constraint::fixed(1), // Divider rule
            Constraint::fill(),   // Main display area
            Constraint::fixed(3), // Interactive TextInput box
            Constraint::fixed(1), // Footer instructions
        });

        Tabs(tab_state)
            .style(TabsStyle::Underline)
            .active_fg(Color::Black)
            .active_bg(Color::Emerald)
            .inactive_fg(Color::White)
            .separator_fg(Color::DarkGray)
            .draw(frame, rows[0]);

        // 2. Rule divider
        Rule().style(BorderStyle::Single).fg(Color::DarkGray).draw(frame, rows[1]);

        // 3. Tab Contents
        Area display_area = rows[2].inset(1, 1, 1, 1);

        if (tab_state.selected == 0) // Japanese
        {
            std::string jp_text =
                "【日本語テキスト検証 - Japanese Typography】\n\n"
                "古池や 蛙飛びこむ 水の音 （松尾芭蕉）\n"
                "祇園精舎の鐘の声、諸行無常の響きあり。\n\n"
                "・ 漢字 (Kanji):      琴 桜 龍 侍 夢 碧 霞 翠 鳳 凰\n"
                "・ ひらがな (Hiragana): あいうえお かきくけこ さしすせそ\n"
                "・ カタカナ (Katakana): アイウエオ カキクケコ サシスセソ\n"
                "・ 全角英数 (Fullwidth): ＡＢＣＤＥＦ １２３４５６\n\n"
                "中央揃え (Centered):\n";

            Text(jp_text).fg(Color::White).draw(frame, display_area);

            Area center_sub = display_area.set_row(display_area.row + 11).set_height(1);
            Text("★★★ 日本語の中央揃えテスト ★★★")
                .align(TextAlign::Center)
                .fg(Color::Cyan)
                .draw(frame, center_sub);

            Area right_sub = display_area.set_row(display_area.row + 13).set_height(1);
            Text("右揃えテキスト (Right-aligned) ➔")
                .align(TextAlign::Right)
                .fg(Color::Yellow)
                .draw(frame, right_sub);
        }
        else if (tab_state.selected == 1) // Chinese
        {
            std::string cn_text =
                "【中文排版与字宽验证 - Chinese Typography】\n\n"
                "床前明月光，疑是地上霜。举头望明月，低头思故乡。（李白）\n"
                "白日依山尽，黄河入海流。欲穷千里目，更上一层楼。（王之涣）\n\n"
                "・ 简体中文 (Simplified): 琴棋书画 风华正茂 纵横捭阖\n"
                "・ 繁體中文 (Traditional): 琴棋書畫 風華正茂 縱橫捭闔\n"
                "・ 中文标点 (Punctuation): 《 》 【 】 “ ” ‘ ’ —— ……\n\n";

            Text(cn_text).fg(Color::White).draw(frame, display_area);

            Area center_sub = display_area.set_row(display_area.row + 10).set_height(1);
            Text("【 居中对齐测试 - Centered Chinese 】")
                .align(TextAlign::Center)
                .fg(Color::Emerald)
                .draw(frame, center_sub);
        }
        else if (tab_state.selected == 2) // Korean
        {
            std::string kr_text =
                "【한국어 타이포그래피 검증 - Korean Typography】\n\n"
                "나랏말싸미 듕귁에 달아 문자와로 서르 사맛디 아니할쎄.\n"
                "이런 젼차로 어린 백셩이 니르고져 홇 배 이셔도\n"
                "마참내 제 뜻을 시러 펴디 몯핧 노미 하니라. (훈민정음)\n\n"
                "・ 한글 음절 (Hangul Syllables): 가나다라마바사 아자차카타파하\n"
                "・ 현대 낱말 (Modern Words):    터미널 프레임워크 고성능 렌더러\n\n";

            Text(kr_text).fg(Color::White).draw(frame, display_area);

            Area center_sub = display_area.set_row(display_area.row + 9).set_height(1);
            Text("🇰🇷 [ 대한민국 서울특별시 - 대한민국 중앙 정렬 ] 🇰🇷")
                .align(TextAlign::Center)
                .fg(Color::Amber)
                .draw(frame, center_sub);
        }
        else if (tab_state.selected == 3) // Emojis
        {
            std::string emoji_text =
                "【2-Column Unicode Emojis & Pictographs】\n\n"
                "Tech & Coding:     💻 🚀 🦀 ⚡ 🤖 🛠️ 🔬 📡 🛰️ 💡 📦\n"
                "Nature & Animals:  🦁 🐯 🦄 🐬 🦅 🌲 🌸 🍀 🌊 ☀️ 🌙\n"
                "Food & Activity:   🍕 🍔 🍣 ☕ 🍺 🎮 🎯 🏆 🎨 🎸 🎹\n"
                "Symbols & Signs:   🔥 ⭐ 💎 💖 ⚠️ ⛔ 🔄 🔔 📌 🚩 ✨\n\n"
                "Notice: Each emoji occupies exactly TWO physical terminal columns.\n"
                "No overlapping characters, no clipped margins, no tearing.\n";

            Text(emoji_text).fg(Color::White).draw(frame, display_area);

            Area center_sub = display_area.set_row(display_area.row + 10).set_height(1);
            Text("🚀 🦀 🔥 💎 [ FULL-WIDTH EMOJI ALIGNMENT ] 💎 🔥 🦀 🚀")
                .align(TextAlign::Center)
                .fg(Color::Cyan)
                .draw(frame, center_sub);
        }
        else if (tab_state.selected == 4) // Alignment Ruler
        {
            // Rulers displaying exact column indices (0 through 39)
            std::string r1 = "Col Index:  0         1         2         3         4         5";
            std::string r2 = "Ruler:      012345678901234567890123456789012345678901234567890";
            std::string r3 = "ASCII:      A B C D E F G H I J K L M N O P Q R S T U V W X Y Z ";
            std::string r4 = "CJK (2col): 琴  棋  書  畫  東  京  北  京  ソ  ウ  ル  한  글  ";
            std::string r5 = "Emoji(2col):🚀  🦀  🎉  🔥  🤖  💻  🍕  ⭐  💡  ⚡  🎯  💎  ✨  ";
            std::string r6 = "Mixed:      |A|琴|B|🚀|C|中|D|🦀|E|日|F|🔥|G|한|H|💻|I|文|J|⭐|";

            std::string ruler_text =
                "【Visual Alignment Stress Test & Verification Ruler】\n\n"
                "Look down each column: Wide glyphs must span exactly 2 characters.\n\n" +
                r1 + "\n" + r2 + "\n\n" +
                r3 + "\n" + r4 + "\n" + r5 + "\n\n" +
                r6 + "\n\n" +
                "Result: 100% vertical grid alignment with zero column drift!";

            Text(ruler_text).fg(Color::White).draw(frame, display_area);
        }

        // 4. Interactive Live TextInput for typing/pasting CJK & Emojis
        Box()
            .border_style(BorderStyle::Rounded)
            .title(" Live UTF-8 Input (Type or Paste CJK/Emojis here) ")
            .title_align(TitleAlign::Left)
            .fg(Color::Emerald)
            .draw(frame, rows[3]);

        TextInput(input)
            .placeholder("Type Japanese, Chinese, Korean, or Emojis here...")
            .border_style(BorderStyle::None)
            .draw(frame, rows[3].inset(1, 2, 1, 2));

        // 5. Footer Instructions
        Text("Switch Tabs: [1-5] or [Tab] | Type into Input | Quit: Ctrl+Q")
            .align(TextAlign::Center)
            .fg(Color::Gray)
            .draw(frame, rows[4]);
    });

    app.run();
    return 0;
}
