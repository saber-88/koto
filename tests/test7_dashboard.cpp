#include <koto/koto.hpp>
#include <string>
#include <vector>

using namespace koto;

int main()
{
    App app;

    // Simulation state
    float progress = 0.0f;
    std::vector<uint64_t> cpu_history = {10, 15, 25, 40, 35, 50, 65, 80, 70, 60, 45, 30, 20, 15, 25, 35};
    uint64_t tick_count = 0;
    bool paused = false;

    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.ch == 'Q')
        {
            app.quit();
        }
        else if (key.ch == ' ')
        {
            paused = !paused;
        }
    });

    app.on_tick([&]() {
        if (paused) return;

        tick_count++;

        // Progress bar wraps around 0% -> 100%
        progress += 0.005f;
        if (progress > 1.0f) progress = 0.0f;

        // Add a pseudo-random point to sparkline every 6 ticks (~100ms)
        if (tick_count % 6 == 0)
        {
            uint64_t next_val = (cpu_history.back() + (tick_count * 17) % 30 + 5) % 100;
            cpu_history.push_back(next_val);
            if (cpu_history.size() > 100)
            {
                cpu_history.erase(cpu_history.begin());
            }
        }
    });

    app.on_render([&](Frame& frame) {
        Area full = frame.area();
        if (full.width < 40 || full.height < 18)
        {
            Text("Terminal too small! Please resize.").align(TextAlign::Center).draw(frame, full);
            return;
        }

        // Main outer container
        Box()
            .border_style(BorderStyle::Rounded)
            .title(" koto System Monitor ")
            .title_align(TitleAlign::Left)
            .fg(Color::Cyan)
            .bg(Color::Rgb(15, 15, 18))
            .filled(true)
            .draw(frame, full);

        Area content = full.inset(1, 2, 1, 2);

        auto rows = Layout::vertical(content, {
            Constraint::fixed(2), // Subtitle & divider
            Constraint::fixed(3), // Progress gauge
            Constraint::fixed(2), // Rule divider
            Constraint::fixed(4), // Sparkline & stats
            Constraint::fixed(2), // Rule divider
            Constraint::fill(),   // Info panel
            Constraint::fixed(1), // Footer
        });

        // 1. Header with alignment
        Text("Real-Time Telemetry & Widget Showcase")
            .align(TextAlign::Center)
            .fg(Color::White)
            .bg(Color::Rgb(15, 15, 18))
            .draw(frame, rows[0]);

        // 2. Gauge with fractional sub-cells and dynamic color lerping
        // Interpolate color smoothly from Emerald (0%) to Amber (50%) to Red (100%)
        uint32_t gauge_col = (progress < 0.5f)
            ? Color::lerp(Color::Emerald, Color::Amber, progress * 2.0f)
            : Color::lerp(Color::Amber, Color::Red, (progress - 0.5f) * 2.0f);

        int pct = static_cast<int>(progress * 100.0f);
        std::string gauge_label = " Buffer Memory: " + std::to_string(pct) + "% ";

        Box()
            .border_style(BorderStyle::Single)
            .title(" Memory Buffer ")
            .fg(Color::DarkGray)
            .draw(frame, rows[1]);

        Gauge()
            .ratio(progress)
            .label(gauge_label)
            .filled_fg(gauge_col)
            .empty_fg(Color::Rgb(40, 40, 45))
            .label_fg(Color::White)
            .draw(frame, rows[1].inset(1, 1, 1, 1));

        // 3. Rule divider with custom styling
        Rule("CPU Load History")
            .style(BorderStyle::Single)
            .title_align(TitleAlign::Left)
            .fg(Color::DarkGray)
            .title_fg(Color::Cyan)
            .draw(frame, rows[2]);

        // 4. Sparkline graph
        uint64_t current_cpu = cpu_history.empty() ? 0 : cpu_history.back();
        std::string cpu_label = "Current: " + std::to_string(current_cpu) + "% | Peak: 100%";
        Text(cpu_label).fg(Color::Yellow).draw(frame, rows[3].set_height(1));

        Sparkline()
            .data(cpu_history)
            .max(100)
            .fg(Color::Cyan)
            .draw(frame, rows[3].inset(1, 0, 0, 0));

        // 5. Rule divider
        Rule("System Status")
            .style(BorderStyle::Single)
            .title_align(TitleAlign::Right)
            .fg(Color::DarkGray)
            .title_fg(Color::Emerald)
            .draw(frame, rows[4]);

        // 6. Centered and aligned information panel
        Text("Renderer: Differential Double-Buffered Engine (60 FPS)\n"
             "Color Mode: 24-bit TrueColor RGB with mathematical lerp\n"
             "Unicode: Fractional sub-block resolution (1/8th cell precision)")
            .fg(Color::LightGray)
            .draw(frame, rows[5]);

        // 7. Footer
        std::string status_text = paused ? "PAUSED (Press Space to resume)" : "RUNNING (Press Space to pause, Q to quit)";
        uint32_t status_col = paused ? Color::Amber : Color::Emerald;

        Text(status_text)
            .align(TextAlign::Center)
            .fg(status_col)
            .draw(frame, rows[6]);
    });

    app.run();
    return 0;
}
