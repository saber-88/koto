#include "koto/list.hpp"
#include <koto/koto.hpp>
#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;
using namespace koto;

struct FileItem
{
    std::string name;
    bool is_dir = false;
    uintmax_t size = 0;
};

// Helper to list entries in a directory
auto list_directory(const fs::path& path) -> std::vector<FileItem>
{
    std::vector<FileItem> items;
    std::error_code ec;

    if (!fs::exists(path, ec) || !fs::is_directory(path, ec))
    {
        return items;
    }

    for (const auto& entry : fs::directory_iterator(path, fs::directory_options::skip_permission_denied, ec))
    {
        FileItem item;
        item.name = entry.path().filename().string();
        item.is_dir = entry.is_directory(ec);
        if (!item.is_dir)
        {
            item.size = entry.file_size(ec);
        }
        items.push_back(std::move(item));
    }

    // Sort: directories first, then alphabetically
    std::sort(items.begin(), items.end(), [](const FileItem& a, const FileItem& b) {
        if (a.is_dir != b.is_dir)
        {
            return a.is_dir > b.is_dir;
        }
        return a.name < b.name;
    });

    return items;
}

// Helper to format byte sizes
auto format_size(uintmax_t bytes) -> std::string
{
    if (bytes < 1024) return std::to_string(bytes) + " B";
    if (bytes < 1024 * 1024) return std::to_string(bytes / 1024) + " KB";
    return std::to_string(bytes / (1024 * 1024)) + " MB";
}

int main()
{
    fs::path current_path = fs::current_path();

    // ListState configured with wrap navigation
    ListState current_list_state({}, ListNavigation::Wrap);
    ListState parent_list_state({}, ListNavigation::Clamp);

    std::vector<FileItem> current_items;
    std::vector<FileItem> parent_items;
    std::vector<std::string> preview_lines;
    std::string preview_title = " Preview ";
    TextAreaState preview_editor_state;

    // Updates only the preview panel (called on j/k navigation)
    auto update_preview = [&]() {
        preview_lines.clear();
        if (!current_items.empty() && current_list_state.selected < static_cast<int>(current_items.size()))
        {
            const auto& selected = current_items[static_cast<size_t>(current_list_state.selected)];
            fs::path target = current_path / selected.name;

            if (selected.is_dir)
            {
                preview_title = " Dir: " + selected.name + " ";
                auto children = list_directory(target);
                if (children.empty())
                {
                    preview_lines.push_back("  (empty directory)");
                }
                else
                {
                    for (const auto& child : children)
                    {
                        preview_lines.push_back((child.is_dir ? "📁 " : "📄 ") + child.name);
                    }
                }
            }
            else
            {
                preview_title = " File: " + selected.name + " (" + format_size(selected.size) + ") ";
                std::ifstream file(target);
                if (file.is_open())
                {
                    std::string line;
                    int count = 0;
                    while (std::getline(file, line) && count < 60)
                    {
                        std::string clean;
                        for (char c : line)
                        {
                            if (c == '\t') clean += "    ";
                            else if (c >= 32) clean += c;
                        }
                        preview_lines.push_back(clean);
                        count++;
                    }
                    if (preview_lines.empty())
                    {
                        preview_lines.push_back("  (empty file)");
                    }
                }
                else
                {
                    preview_lines.push_back("  (cannot preview file)");
                }
            }
        }
        else
        {
            preview_title = " Preview ";
            preview_lines.push_back("  (no items)");
        }
        preview_editor_state = TextAreaState(preview_lines);
    };

    // Reloads directories from disk (only called on cd / h / l)
    auto reload_directory = [&]() {
        current_items = list_directory(current_path);

        std::vector<std::string> current_names;
        for (const auto& item : current_items)
        {
            current_names.push_back((item.is_dir ? "📁 " : "📄 ") + item.name);
        }
        current_list_state.set_items(std::move(current_names));

        if (current_path.has_parent_path() && current_path != current_path.parent_path())
        {
            parent_items = list_directory(current_path.parent_path());
        }
        else
        {
            parent_items.clear();
        }

        std::vector<std::string> parent_names;
        for (const auto& item : parent_items)
        {
            parent_names.push_back((item.is_dir ? "📁 " : "📄 ") + item.name);
        }
        parent_list_state.set_items(std::move(parent_names));

        update_preview();
    };

    reload_directory();

    App app;

    // Vim keyboard navigation using ListState's built-in event handling
    app.on_key([&](const KeyEvent& key) {
        // 1. ListState handles j, k, g, G, Up, Down, Home, End with WRAP automatically!
        if (current_list_state.handle_event(key))
        {
            update_preview();
            return;
        }

        // 2. h or Left: go up to parent directory
        if (key.ch == 'h' || key.is(Key::Left))
        {
            if (current_path.has_parent_path() && current_path != current_path.parent_path())
            {
                current_path = current_path.parent_path();
                reload_directory();
            }
            return;
        }

        // 3. l or Right or Enter: enter directory
        if (key.ch == 'l' || key.is(Key::Right) || key.is(Key::Enter))
        {
            if (!current_items.empty() && current_list_state.selected < static_cast<int>(current_items.size()))
            {
                const auto& selected = current_items[static_cast<size_t>(current_list_state.selected)];
                if (selected.is_dir)
                {
                    current_path = current_path / selected.name;
                    reload_directory();
                }
            }
            return;
        }

        // 4. q: quit
        if (key.ch == 'q')
        {
            app.quit();
            return;
        }
    });

    // Pure UI rendering
    app.on_render([&](Frame& frame) {
        Area area = frame.area();

        if (area.width < 40 || area.height < 10)
        {
            return;
        }

        // Split vertically into: Top Path Bar (3), Main 3 Columns (fill), Bottom Status Bar (1)
        auto v_sections = Layout::vertical(area, {
            Constraint::fixed(3),
            Constraint::fill(),
            Constraint::fixed(1)
        });

        // 1. Top Path Bar
        Box()
            .border_style(BorderStyle::Rounded)
            .fg(Color::Cyan)
            .title(" 📁 yazi-style file manager (koto) ")
            .title_align(TitleAlign::Left)
            .title_fg(Color::White)
            .draw(frame, v_sections[0]);

        Text("Path: " + current_path.string())
            .fg(Color::Cyan)
            .draw(frame, v_sections[0].inset(1, 2, 1, 2));

        // 2. Main 3 Columns: Parent (20%), Active Dir (35%), Preview (45%)
        auto h_cols = Layout::horizontal(v_sections[1], {
            Constraint::percent(20),
            Constraint::percent(35),
            Constraint::fill()
        });

        // Column 1: Parent Directory
        Box()
            .border_style(BorderStyle::Rounded)
            .fg(Color::DarkGray)
            .title(" Parent ")
            .title_align(TitleAlign::Left)
            .title_fg(Color::Gray)
            .draw(frame, h_cols[0]);

        List(parent_list_state)
            .fg(Color::Gray)
            .draw(frame, h_cols[0].inset(1, 1, 1, 1));

        // Column 2: Current Directory (Active with wrap navigation!)
        Box()
            .border_style(BorderStyle::Rounded)
            .fg(Color::Emerald)
            .title(" " + current_path.filename().string() + " (" + std::to_string(current_items.size()) + ") [WRAP] ")
            .title_align(TitleAlign::Left)
            .title_fg(Color::Emerald)
            .draw(frame, h_cols[1]);

        List(current_list_state)
            .fg(Color::White)
            .highlight_fg(Color::Emerald)
            .highlight_bg(Color::Rgb(34, 51, 34))
            .draw(frame, h_cols[1].inset(1, 1, 1, 1));

        // Column 3: Preview Panel using TextArea widget with line numbers!
        TextArea(preview_editor_state)
            .border_style(BorderStyle::Rounded)
            .title(preview_title)
            .title_align(TitleAlign::Left)
            .line_numbers(true)
            .fg(Color::LightGray)
            .line_number_fg(Color::DarkGray)
            .focused(false)
            .draw(frame, h_cols[2]);

        // 3. Status Bar at the bottom
        std::string selected_info = "No items";
        if (!current_items.empty() && current_list_state.selected < static_cast<int>(current_items.size()))
        {
            const auto& sel = current_items[static_cast<size_t>(current_list_state.selected)];
            selected_info = (sel.is_dir ? "[DIR] " : "[FILE] ") + sel.name + 
                            (sel.is_dir ? "" : " | " + format_size(sel.size));
        }

        std::string status = " " + selected_info + "  │  h/←: parent | l/→/Enter: open | j/↓: down (wraps) | k/↑: up (wraps) | q: quit";
        Text(status)
            .fg(Color::Gray)
            .draw(frame, v_sections[2]);
    });

    // Start App
    app.run();

    return 0;
}
