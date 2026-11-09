#include <koto/koto.hpp>
#include <array>
#include <string>

using namespace koto;

int main()
{
    App app;
    FocusManager fm;
    fm.navigation_policy(FocusNavPolicy::SpatialArrows);
    app.use_focus(fm);

    // Board state: 0 = empty, 1 = 'X', 2 = 'O'
    std::array<std::array<int, 3>, 3> board = {};
    int current_player = 1; // 1 = X, 2 = O
    std::string status = "Player X's Turn (Click any cell, or use Arrows + Enter)";

    auto check_winner = [&]() -> int {
        // Rows & Cols
        for (size_t i = 0; i < 3; i++)
        {
            if (board[i][0] != 0 && board[i][0] == board[i][1] && board[i][1] == board[i][2])
            {
                return board[i][0];
            }
            if (board[0][i] != 0 && board[0][i] == board[1][i] && board[1][i] == board[2][i])
            {
                return board[0][i];
            }
        }
        // Diagonals
        if (board[0][0] != 0 && board[0][0] == board[1][1] && board[1][1] == board[2][2])
        {
            return board[0][0];
        }
        if (board[0][2] != 0 && board[0][2] == board[1][1] && board[1][1] == board[2][0])
        {
            return board[0][2];
        }
        // Check for draw
        bool full = true;
        for (const auto& row : board)
        {
            for (int cell : row)
            {
                if (cell == 0) full = false;
            }
        }
        if (full) return -1; // Draw
        return 0; // Game in progress
    };

    auto make_move = [&](size_t r, size_t c) {
        if (check_winner() != 0) return;
        if (board[r][c] == 0)
        {
            board[r][c] = current_player;
            int res = check_winner();
            if (res == 1)
            {
                status = "🎉 Player X WINS! Press 'R' to play again.";
            }
            else if (res == 2)
            {
                status = "🎉 Player O WINS! Press 'R' to play again.";
            }
            else if (res == -1)
            {
                status = "🤝 It's a DRAW! Press 'R' to play again.";
            }
            else
            {
                current_player = (current_player == 1) ? 2 : 1;
                status = (current_player == 1) ? "Player X's Turn" : "Player O's Turn";
            }
        }
    };

    // Hotkey: 'R' to restart, 'Ctrl+Q' or 'Q' to quit
    app.on_key([&](const KeyEvent& key) {
        if ((key.ctrl && (key.ch == 'q' || key.ch == 'Q')) || key.ch == 'q' || key.ch == 'Q')
        {
            app.quit();
            return;
        }
        if (key.ch == 'r' || key.ch == 'R')
        {
            board = {};
            current_player = 1;
            status = "Game restarted! Player X's Turn";
        }
    });

    app.on_render([&](Frame& frame) {
        Area full = app.area();
        if (full.width < 30 || full.height < 15) return;

        // Outer window box
        Box().border_style(BorderStyle::Rounded)
            .title(" Tic-Tac-Toe ")
            .title_align(TitleAlign::Left)
            .fg(Color::Cyan)
            .bg(Color::Rgb(17, 17, 17))
            .filled(true)
            .draw(frame, full);

        // Center a 36x15 box for the grid
        Area center_area = Layout::center(full.inset(1, 1, 2, 1), 36, 15);
        auto grid = Layout::grid_uniform(center_area, 3, 3);

        // Declarative Grid of Buttons
        grid.for_each([&](size_t r, size_t c, const Area& cell_area) {
            int val = board[r][c];
            std::string marker = (val == 1) ? "X" : (val == 2 ? "O" : " ");
            uint32_t marker_color = (val == 1) ? Color::Rgb(255, 51, 102) : Color::Cyan;

            Button(marker)
                .id(Id("cell", r, c))
                .focus(fm)
                .border_style(BorderStyle::Single)
                .fg(Color::Rgb(68, 68, 68))
                .bg(Color::Rgb(24, 24, 24))
                .label_fg(marker_color)
                .focused_fg(Color::Emerald)
                .focused_bg(Color::Rgb(34, 34, 34))
                .focused_border_fg(Color::Emerald)
                .focused_label_fg(marker_color)
                .on_click([&, r, c] {
                    make_move(r, c);
                })
                .draw(frame, cell_area);
        });

        // Status bar at bottom
        Area status_area = full.set_row(full.bottom() - 2).set_height(1).inset(0, 3, 0, 3);
        Text(status).fg(Color::Yellow).bg(Color::Rgb(17, 17, 17)).draw(frame, status_area);

        Area footer_area = full.set_row(full.bottom() - 1).set_height(1).inset(0, 3, 0, 3);
        Text("Controls: Left Click to place | Arrows (2D spatial) | Space/Enter | Restart: R | Quit: Ctrl+Q")
            .fg(Color::DarkGray)
            .bg(Color::Rgb(17, 17, 17))
            .draw(frame, footer_area);
    });

    return app.run() ? 0 : 1;
}
