#include <fmt/base.h>
#include <fmt/color.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include "tui/screen.hpp"
#include "tui/cell.hpp"
#include "tui/colors.hpp"

size_t Tui::Screen::m_rows = 0;
size_t Tui::Screen::m_cols = 0;

void Tui::Screen::updateSize(){
    struct winsize w;
    
    if(ioctl(STDOUT_FILENO,  TIOCGWINSZ, &w) == -1){
        fmt::println(stderr, "[ERR]:failed to get terminal dimensions");
        m_initScreen = false;
        return;
    }
    
    m_rows = w.ws_row;
    m_cols = w.ws_col;

    Cell defaultCell;
    defaultCell.cellItem = "";
    defaultCell.fg = FG::Default;
    defaultCell.bg = BG::Default;
    
    frontBuffer.assign(m_rows * m_cols , defaultCell);
    backBuffer.assign(m_rows * m_cols , defaultCell);

}

void Tui::Screen::checkResize() {
    struct winsize w;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0) {
        if (w.ws_row != m_rows || w.ws_col != m_cols) {  // only if changed
            m_rows = w.ws_row;
            m_cols = w.ws_col;
            Cell defaultCell;
            defaultCell.cellItem = "";
            defaultCell.fg = FG::Default;
            defaultCell.bg = BG::Default;
            frontBuffer.assign(m_rows * m_cols, defaultCell);
            backBuffer.assign(m_rows * m_cols, defaultCell);
        }
    }
}

Tui::Screen::Screen(){
    updateSize();
}

Tui::Screen::~Screen(){
    if (m_initScreen && m_altBuffer) {
        
        tcsetattr(STDIN_FILENO, TCSANOW, &og_tio);
        fmt::print("\x1b[?1049l");
        
    }
}

void Tui::Screen::init(bool altBuffer){
    this->m_altBuffer = altBuffer;
    
    if (m_altBuffer) {
        
        tcgetattr(STDIN_FILENO, &og_tio);
        raw_tio = og_tio;
        
        raw_tio.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &raw_tio);
        
        fmt::print("\x1b[?1049h");
    }
    m_initScreen = true;
}

void Tui::Screen::exit(){
    if (m_initScreen && m_altBuffer) {
        
        tcsetattr(STDIN_FILENO, TCSANOW, &og_tio);
        fmt::print("\x1b[?1049l");
        
    }
}

std::vector<Cell>& Tui::Screen::getBackBuffer(){
    return backBuffer;
}

void Tui::Screen::render(){
    
    checkResize();
    
    for (size_t r { 0 }; r < m_rows; ++r) {
        for (size_t c { 0 }; c < m_cols; ++c) {
            size_t idx = r * m_cols + c ;
            if (!(backBuffer[idx] == frontBuffer[idx])) {
                if (backBuffer[idx].fg == FG::Default && backBuffer[idx].bg == BG::Default) {
                    
                    fmt::print("\x1b[{};{}H", r + 1, c + 1);
                    fmt::print("{}",backBuffer[idx].cellItem);
                }
                else {
                    fmt::print("\x1b[{};{}H", r + 1, c + 1);
                    fmt::print("{}",Paint
                                (
                                    backBuffer[idx].cellItem ,
                                    backBuffer[idx].fg ,
                                    backBuffer[idx].bg ,
                                    backBuffer[idx].style
                                )
                            );
                }
                frontBuffer[idx] = backBuffer[idx];
            }
        }
    }

    fflush(stdout);
}

size_t Tui::Screen::getRows(){
    return m_rows;
}
size_t Tui::Screen::getCols(){
    return m_cols;
}

void Tui::Screen::hideCursor(){
    fmt::print("\x1b[?25l");
}
void Tui::Screen::showCursor(){
    fmt::print("\x1b[?25h");
}

void Tui::Screen::clearScreen(){
    Cell defaultCell;
    defaultCell.cellItem = "";
    defaultCell.fg = FG::Default;
    defaultCell.bg = BG::Default;
    defaultCell.style = Style::None;
    backBuffer.assign(m_rows * m_cols ,defaultCell );
}
