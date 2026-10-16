#include <koto/koto.hpp>
#include <array>
#include <string>

using namespace koto;

int main()
{
    App app;

    // Board state: 0 = empty, 1 = 'X', 2 = 'O'
    std::array<std::array<int, 3>, 3> board = {};
    int cur_r = 0;
    int cur_c = 0;
    int current_player = 1; // 1 = X, 2 = O
    std::string status = "Player X's Turn (Arrows/hjkl to move, Enter to place, R to restart)";

    auto check_winner = [&]() -> int {
        // Rows & Cols
        for (int i = 0; i < 3; i++)
        {
            if (board[static_cast<size_t>(i)][0] != 0 &&
                board[static_cast<size_t>(i)][0] == board[static_cast<size_t>(i)][1] &&
                board[static_cast<size_t>(i)][1] == board[static_cast<size_t>(i)][2])
            {
                return board[static_cast<size_t>(i)][0];
            }
            if (board[0][static_cast<size_t>(i)] != 0 &&
                board[0][static_cast<size_t>(i)] == board[1][static_cast<size_t>(i)] &&
                board[1][static_cast<size_t>(i)] == board[2][static_cast<size_t>(i)])
            {
                return board[0][static_cast<size_t>(i)];
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

    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'r' || key.ch == 'R')
        {
            board = {};
            current_player = 1;
            status = "Game restarted! Player X's Turn";
            return;
        }

        int winner = check_winner();
        if (winner != 0) return; // Game over, wait for 'r'

        if (key.is(Key::Up) || key.ch == 'k')
        {
            cur_r = (cur_r + 2) % 3;
        }
        else if (key.is(Key::Down) || key.ch == 'j')
        {
            cur_r = (cur_r + 1) % 3;
        }
        else if (key.is(Key::Left) || key.ch == 'h')
        {
            cur_c = (cur_c + 2) % 3;
        }
        else if (key.is(Key::Right) || key.ch == 'l')
        {
            cur_c = (cur_c + 1) % 3;
        }
        else if (key.is(Key::Enter) || key.ch == ' ')
        {
            if (board[static_cast<size_t>(cur_r)][static_cast<size_t>(cur_c)] == 0)
            {
                board[static_cast<size_t>(cur_r)][static_cast<size_t>(cur_c)] = current_player;
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
        }
    });

    app.on_render([&](Frame& frame) {
        Area full = frame.area();
        if (full.width < 30 || full.height < 15) return;

        // Outer window box
        Box().border_style(BorderStyle::Rounded)
            .title("Tic-Tac-Toe")
            .fg("#00E5FF")
            .bg("#111111")
            .filled(true)
            .draw(frame, full);

        // Center a 30x15 box for the grid
        Area center_area = Layout::center(full.inset(1, 1, 2, 1), 36, 15);

        auto rows = Layout::vertical(center_area, {
            Constraint::fixed(5),
            Constraint::fixed(5),
            Constraint::fixed(5),
        });

        for (int r = 0; r < 3; r++)
        {
            auto cols = Layout::horizontal(rows[static_cast<size_t>(r)], {
                Constraint::fixed(12),
                Constraint::fixed(12),
                Constraint::fixed(12),
            });

            for (int c = 0; c < 3; c++)
            {
                Area cell_area = cols[static_cast<size_t>(c)];
                bool is_cursor = (r == cur_r && c == cur_c);

                // Highlight border if active cell
                BorderStyle style = is_cursor ? BorderStyle::Thick : BorderStyle::Single;
                std::string border_color = is_cursor ? "#00FF88" : "#444444";
                std::string bg_color = is_cursor ? "#222222" : "#181818";

                Box().border_style(style)
                    .fg(border_color)
                    .bg(bg_color)
                    .filled(true)
                    .draw(frame, cell_area);

                // Draw X or O inside cell
                int val = board[static_cast<size_t>(r)][static_cast<size_t>(c)];
                std::string marker = " ";
                std::string marker_color = "#FFFFFF";
                if (val == 1) { marker = "X"; marker_color = "#FF3366"; }
                else if (val == 2) { marker = "O"; marker_color = "#00D4FF"; }

                Area text_area = cell_area.inset(2, 5, 2, 5);
                Text(marker).fg(marker_color).bg(bg_color).draw(frame, text_area);
            }
        }

        // Status bar at bottom
        Area status_area = full.set_row(full.bottom() - 2).set_height(1).inset(0, 3, 0, 3);
        Text(status).fg("#FFFF00").bg("#111111").draw(frame, status_area);

        Area footer_area = full.set_row(full.bottom() - 1).set_height(1).inset(0, 3, 0, 3);
        Text("Navigation: Arrows / hjkl | Select: Space/Enter | Restart: R | Quit: Ctrl+Q")
            .fg("#666666")
            .bg("#111111")
            .draw(frame, footer_area);
    });

    app.run();
    return 0;
}