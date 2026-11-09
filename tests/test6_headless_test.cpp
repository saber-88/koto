#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main(int argc, char* argv[])
{
    std::filesystem::path exe_dir = std::filesystem::current_path();
    if (argc > 0 && argv[0])
    {
        std::filesystem::path p(argv[0]);
        if (p.has_parent_path())
        {
            std::error_code ec;
            auto can = std::filesystem::canonical(p, ec);
            if (!ec)
            {
                exe_dir = can.parent_path();
            }
            else
            {
                exe_dir = p.parent_path();
            }
        }
    }

    std::vector<std::pair<std::string, std::string>> suites = {
        {"test_headless_widgets", "Headless Widgets Suite (11 tests)"},
        {"test_headless_layout", "Headless Layout & Core Suite (5 tests)"},
        {"test_headless_focus_events", "Headless Focus & Events Suite (8 tests)"},
        {"test_headless_render_terminal", "Headless Render & Terminal Suite (6 tests)"},
        {"test_headless_regressions", "Headless Regressions & Audit Suite (7 tests)"},
    };

    std::cout << "========================================\n";
    std::cout << " Running koto Headless Snapshot Suites\n";
    std::cout << " (Isolated Process Multi-Test Runner)\n";
    std::cout << "========================================\n\n";

    for (const auto& [bin_name, title] : suites)
    {
        std::filesystem::path bin_path = exe_dir / bin_name;
        if (!std::filesystem::exists(bin_path))
        {
            if (std::filesystem::exists(std::filesystem::path("build") / bin_name))
            {
                bin_path = std::filesystem::path("build") / bin_name;
            }
            else
            {
                bin_path = bin_name;
            }
        }

        std::cout << "▶ Executing " << title << "...\n";
        int code = std::system(bin_path.c_str());
        if (code != 0)
        {
            std::cerr << "\n❌ Suite [" << bin_name << "] FAILED with exit code " << code << "\n";
            return code;
        }
        std::cout << "✔ " << title << " finished successfully.\n\n";
    }

    std::cout << "========================================\n";
    std::cout << " ALL 37 HEADLESS TESTS PASSED CLEANLY!\n";
    std::cout << "========================================\n";
    return 0;
}
