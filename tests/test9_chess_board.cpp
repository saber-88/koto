#include <koto/koto.hpp>
#include <array>
#include <cstdint>
#include <string>

using namespace koto;

struct Piece
{
    char32_t symbol = 0;
    int color = 0; // 0 = empty, 1 = white, 2 = black
    std::string name = "";
};

int main()
{
    App app;

    // 8x8 Chess board representation
    std::array<std::array<Piece, 8>, 8> board = {};

    auto init_board = [&]() {
        board = {};

        // Black major pieces (Row 0)
        board[0][0] = {U'♜', 2, "Black Rook"};
        board[0][1] = {U'♞', 2, "Black Knight"};
        board[0][2] = {U'♝', 2, "Black Bishop"};
        board[0][3] = {U'♛', 2, "Black Queen"};
        board[0][4] = {U'♚', 2, "Black King"};
        board[0][5] = {U'♝', 2, "Black Bishop"};
        board[0][6] = {U'♞', 2, "Black Knight"};
        board[0][7] = {U'♜', 2, "Black Rook"};

        // Black pawns (Row 1)
        for (size_t c = 0; c < 8; ++c) {
            board[1][c] = {U'♟', 2, "Black Pawn"};
        }

        // White pawns (Row 6)
        for (size_t c = 0; c < 8; ++c) {
            board[6][c] = {U'♙', 1, "White Pawn"};
        }

        // White major pieces (Row 7)
        board[7][0] = {U'♖', 1, "White Rook"};
        board[7][1] = {U'♘', 1, "White Knight"};
        board[7][2] = {U'♗', 1, "White Bishop"};
        board[7][3] = {U'♕', 1, "White Queen"};
        board[7][4] = {U'♔', 1, "White King"};
        board[7][5] = {U'♗', 1, "White Bishop"};
        board[7][6] = {U'♘', 1, "White Knight"};
        board[7][7] = {U'♖', 1, "White Rook"};
    };

    init_board();

    int cur_r = 6;
    int cur_c = 4; // Start at e2 (White King's Pawn)
    int sel_r = -1;
    int sel_c = -1;
    std::string message = "White's turn. Use Arrows/hjkl to navigate, Space to select piece, R to reset.";

    auto pos_name = [](int r, int c) -> std::string {
        char col_char = static_cast<char>('a' + c);
        char row_char = static_cast<char>('8' - r);
        return std::string{col_char, row_char};
    };

    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.ch == 'Q')
        {
            app.quit();
            return;
        }
        if (key.ch == 'r' || key.ch == 'R')
        {
            init_board();
            sel_r = -1;
            sel_c = -1;
            message = "Board reset to starting position. White to move.";
            return;
        }

        // Navigation
        if (key.is(Key::Up) || key.ch == 'k')
        {
            cur_r = (cur_r > 0) ? cur_r - 1 : 7;
        }
        else if (key.is(Key::Down) || key.ch == 'j')
        {
            cur_r = (cur_r < 7) ? cur_r + 1 : 0;
        }
        else if (key.is(Key::Left) || key.ch == 'h')
        {
            cur_c = (cur_c > 0) ? cur_c - 1 : 7;
        }
        else if (key.is(Key::Right) || key.ch == 'l')
        {
            cur_c = (cur_c < 7) ? cur_c + 1 : 0;
        }
        else if (key.is(Key::Enter) || key.ch == ' ')
        {
            if (sel_r == -1)
            {
                // Select piece to move
                if (board[static_cast<size_t>(cur_r)][static_cast<size_t>(cur_c)].color != 0)
                {
                    sel_r = cur_r;
                    sel_c = cur_c;
                    message = "Selected " + board[static_cast<size_t>(sel_r)][static_cast<size_t>(sel_c)].name +
                              " at " + pos_name(sel_r, sel_c) + ". Move cursor and press Space to place.";
                }
            }
            else
            {
                // Move piece to target square
                if (sel_r == cur_r && sel_c == cur_c)
                {
                    // Deselect
                    sel_r = -1;
                    sel_c = -1;
                    message = "Selection canceled.";
                }
                else
                {
                    board[static_cast<size_t>(cur_r)][static_cast<size_t>(cur_c)] =
                        board[static_cast<size_t>(sel_r)][static_cast<size_t>(sel_c)];
                    board[static_cast<size_t>(sel_r)][static_cast<size_t>(sel_c)] = {};

                    message = "Moved piece from " + pos_name(sel_r, sel_c) + " to " + pos_name(cur_r, cur_c) + "!";
                    sel_r = -1;
                    sel_c = -1;
                }
            }
        }
    });

    app.on_render([&](Frame& frame) {
        Area full = frame.area();
        if (full.width < 50 || full.height < 22)
        {
            Text("Terminal too small! Need at least 50x22 for Chess.").align(TextAlign::Center).draw(frame, full);
            return;
        }

        // Outer window frame
        Box()
            .border_style(BorderStyle::Rounded)
            .title(" koto Chess (琴棋) ")
            .title_align(TitleAlign::Left)
            .fg(Color::Cyan)
            .bg(Color::Rgb(20, 20, 24))
            .filled(true)
            .draw(frame, full);

        // Center a 38x20 envelope for the board and coordinates
        // 8 squares of width 4 = 32 columns. 8 squares of height 2 = 16 rows.
        // Rank labels take 3 cols on left, File labels take 1 row on bottom.
        Area center_envelope = Layout::center(full.inset(1, 1, 3, 1), 38, 20);

        // Outer board border
        Box()
            .border_style(BorderStyle::Rounded)
            .fg(Color::DarkGray)
            .draw(frame, center_envelope);

        // Board area inside the box, leaving 3 cols on left for rank numbers and 1 row below for file letters
        Area board_area(center_envelope.row + 1, center_envelope.col + 4, 32, 16);

        // 8x8 Grid: exactly 8 rows of height 2, 8 columns of width 4
        auto chess_grid = Layout::grid_uniform(board_area, 8, 8);

        // Draw Rank labels (8 down to 1 on the left)
        for (size_t r = 0; r < 8; ++r)
        {
            std::string rank_label = std::to_string(8 - r);
            Area rank_area(chess_grid[r][0].row, center_envelope.col + 2, 1, 1);
            Text(rank_label).fg(Color::Yellow).draw(frame, rank_area);
        }

        // Draw File labels (a through h on the bottom row inside the box)
        for (size_t c = 0; c < 8; ++c)
        {
            std::string file_label = std::string(1, static_cast<char>('a' + c));
            Area file_area(board_area.bottom(), chess_grid[0][c].col, chess_grid[0][c].width, 1);
            Text(file_label).align(TextAlign::Center).fg(Color::Yellow).draw(frame, file_area);
        }

        // Render each square and chess piece
        chess_grid.for_each([&](size_t r, size_t c, const Area& square) {
            bool is_cursor = (static_cast<int>(r) == cur_r && static_cast<int>(c) == cur_c);
            bool is_selected = (static_cast<int>(r) == sel_r && static_cast<int>(c) == sel_c);
            bool is_dark = ((r + c) % 2 == 1);

            // Wood theme colors
            uint32_t square_bg = is_dark ? Color::Rgb(181, 136, 99) : Color::Rgb(240, 217, 181);

            if (is_selected)
            {
                square_bg = Color::Rgb(246, 246, 105); // Highlight selected piece in bright yellow
            }
            else if (is_cursor)
            {
                square_bg = Color::Rgb(80, 200, 120); // Highlight cursor in emerald green
            }

            // Fill entire square with background color
            frame.fill(square, U' ', Color::Default, square_bg);

            // Render chess piece
            const Piece& piece = board[r][c];
            if (piece.color != 0)
            {
                uint32_t piece_fg = (piece.color == 1) ? Color::White : Color::Black;
                char piece_buf[5] = {};
                encode_utf8(piece.symbol, piece_buf);

                // Center piece vertically & horizontally in square
                Area piece_area = square;
                if (piece_area.height > 1)
                {
                    piece_area.row += (piece_area.height - 1) / 2;
                    piece_area.height = 1;
                }
                Text(piece_buf)
                    .align(TextAlign::Center)
                    .fg(piece_fg)
                    .bg(square_bg)
                    .draw(frame, piece_area);
            }
        });

        // Message and status bar at bottom
        Area status_area = full.set_row(full.bottom() - 2).set_height(1).inset(0, 3, 0, 3);
        Text(message).fg(Color::Yellow).bg(Color::Rgb(20, 20, 24)).draw(frame, status_area);

        Area footer_area = full.set_row(full.bottom() - 1).set_height(1).inset(0, 3, 0, 3);
        std::string cursor_info = "Square: " + pos_name(cur_r, cur_c) + " | Navigation: Arrows/hjkl | Select/Place: Space | Reset: R | Quit: Q";
        Text(cursor_info).fg(Color::Gray).bg(Color::Rgb(20, 20, 24)).draw(frame, footer_area);
    });

    app.run();
    return 0;
}
