#include <koto/koto.hpp>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <random>

using namespace koto;

struct Star {
    int r, c;
    float phase;
    float twinkle_speed;
    char32_t sym;
};

int main()
{
    App app;

    int tick = 0;
    bool paused = false;
    float manual_offset_x = 0.0f;
    float manual_offset_y = 0.0f;
    float scale = 1.0f;

    // Starfield setup
    std::mt19937 rng(1337);
    std::uniform_int_distribution<int> dist_r(0, 100);
    std::uniform_int_distribution<int> dist_c(0, 200);
    std::uniform_real_distribution<float> dist_phase(0.0f, 6.28f);
    std::uniform_real_distribution<float> dist_speed(0.03f, 0.09f);

    std::vector<Star> stars;
    for (int i = 0; i < 110; ++i) {
        char32_t sym = U'·';
        if (i % 7 == 0) sym = U'★';
        else if (i % 3 == 0) sym = U'+';
        else if (i % 2 == 0) sym = U'✦';

        stars.push_back({
            dist_r(rng),
            dist_c(rng),
            dist_phase(rng),
            dist_speed(rng),
            sym
        });
    }

    // Interactive Key Handling
    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.ch == 'Q' || key.is(Key::Escape)) {
            app.quit();
        }
        else if (key.ch == ' ') {
            paused = !paused;
        }
        else if (key.is(Key::Left) || key.ch == 'h' || key.ch == 'H') {
            manual_offset_x -= 2.0f;
        }
        else if (key.is(Key::Right) || key.ch == 'l' || key.ch == 'L') {
            manual_offset_x += 2.0f;
        }
        else if (key.is(Key::Up) || key.ch == 'k' || key.ch == 'K') {
            manual_offset_y -= 1.0f;
        }
        else if (key.is(Key::Down) || key.ch == 'j' || key.ch == 'J') {
            manual_offset_y += 1.0f;
        }
        else if (key.ch == '+' || key.ch == '=') {
            scale = std::min(1.5f, scale + 0.1f);
        }
        else if (key.ch == '-' || key.ch == '_') {
            scale = std::max(0.6f, scale - 0.1f);
        }
        else if (key.ch == 'r' || key.ch == 'R') {
            manual_offset_x = 0.0f;
            manual_offset_y = 0.0f;
            scale = 1.0f;
        }
    });

    // 60 FPS Simulation Tick
    app.on_tick([&]() {
        if (!paused) {
            tick++;
        }
    });

    // Flicker-free Render Pass
    app.on_render([&](Frame& frame) {
        Area screen = app.area();
        if (screen.width < 15 || screen.height < 15) return;

        float t = static_cast<float>(tick) * 0.03f;

        // 1. Draw Twinkling Deep Space Starfield
        for (const auto& star : stars) {
            int sr = star.r % screen.height;
            int sc = star.c % screen.width;

            float tw = (std::sin(t * star.twinkle_speed * 40.0f + star.phase) + 1.0f) * 0.5f;
            uint32_t star_color = Color::lerp(Color::Rgb(40, 45, 60), Color::Rgb(220, 230, 255), tw);
            frame[sr, sc] = Cell{star.sym, star_color, Color::Reset};
        }

        // 2. Saturn Position (Gentle orbital floating)
        float base_cx = static_cast<float>(screen.width) * 0.5f + manual_offset_x;
        float base_cy = static_cast<float>(screen.height) * 0.5f + manual_offset_y;

        float cx = base_cx + std::sin(t * 0.4f) * 12.0f;
        float cy = base_cy + std::cos(t * 0.6f) * 2.8f;

        // Radius and Ring Boundaries (scaled)
        float R                = 8.5f * scale;
        float ring_in          = 11.0f * scale;
        float ring_cassini_in  = 15.0f * scale;
        float ring_cassini_out = 16.3f * scale;
        float ring_out         = 20.5f * scale;

        // Slow 3D Axial Tilt Precession
        float pitch = 0.42f + std::sin(t * 0.2f) * 0.08f; // Elevation tilt towards camera
        float roll  = -0.22f + std::cos(t * 0.15f) * 0.05f; // Rotation in screen plane

        float nx = -std::sin(roll) * std::cos(pitch);
        float ny =  std::cos(roll) * std::sin(pitch);
        float nz =  std::cos(roll) * std::cos(pitch);

        // Light Vector (Sun situated at top-left-front)
        float lx = -0.577f, ly = -0.577f, lz = 0.577f;

        // 3. Moon Titan (3D Orbit)
        float titan_angle = t * 0.7f;
        float titan_dist  = 28.0f * scale;
        float titan_x     = std::cos(titan_angle) * titan_dist;
        float titan_y     = std::sin(titan_angle) * (titan_dist * 0.35f);
        float titan_z     = -std::sin(titan_angle) * titan_dist; // Depth relative to planet
        int titan_col     = static_cast<int>(cx + titan_x);
        int titan_row     = static_cast<int>(cy + titan_y * 0.5f);

        // Bounding box for efficient raycasting
        int min_r = std::max(0, static_cast<int>(cy - ring_out * 0.7f - 3));
        int max_r = std::min(screen.height - 1, static_cast<int>(cy + ring_out * 0.7f + 3));
        int min_c = std::max(0, static_cast<int>(cx - ring_out - 4));
        int max_c = std::min(screen.width - 1, static_cast<int>(cx + ring_out + 4));

        for (int r = min_r; r <= max_r; ++r) {
            for (int c = min_c; c <= max_c; ++c) {
                float x = static_cast<float>(c) - cx;
                float y = (static_cast<float>(r) - cy) * 2.0f; // 2:1 character aspect ratio correction

                // A. Sphere Intersection Test
                float dist_sq = x * x + y * y;
                bool hit_sphere = false;
                float z_sphere = -999.0f;
                float diffuse = 0.0f;
                float sny = 0.0f;

                if (dist_sq <= R * R) {
                    hit_sphere = true;
                    z_sphere = std::sqrt(R * R - dist_sq);
                    float snx = x / R;
                    sny = y / R;
                    float snz = z_sphere / R;
                    diffuse = std::max(0.08f, snx * lx + sny * ly + snz * lz);
                }

                // B. Ring Plane Intersection Test: nx*x + ny*y + nz*z = 0
                float z_ring = -(nx * x + ny * y) / nz;
                float r_dist = std::sqrt(x * x + y * y + z_ring * z_ring);

                bool hit_ring = false;
                bool in_b_ring = false;
                if (r_dist >= ring_in && r_dist <= ring_out) {
                    // Cassini Division Gap
                    if (r_dist < ring_cassini_in || r_dist > ring_cassini_out) {
                        hit_ring = true;
                        if (r_dist >= 12.0f * scale && r_dist <= ring_cassini_in) {
                            in_b_ring = true; // Bright main B-Ring
                        }
                    }
                }

                // C. Planetary Raytraced Shadow on Ring
                bool ring_in_shadow = false;
                if (hit_ring) {
                    float p_dot_l = x * lx + y * ly + z_ring * lz;
                    float t_closest = -p_dot_l;
                    if (t_closest > 0.0f) {
                        float dist_to_center_sq = (x * x + y * y + z_ring * z_ring) - (t_closest * t_closest);
                        if (dist_to_center_sq < R * R) {
                            ring_in_shadow = true;
                        }
                    }
                }

                // D. Depth Sorting & Shading
                // If ring is in front of sphere or sphere wasn't hit:
                if (hit_ring && (!hit_sphere || z_ring > z_sphere)) {
                    float ring_lit = ring_in_shadow ? 0.15f : 1.0f;

                    char32_t ring_ch = in_b_ring ? U'═' : U'─';
                    if (r_dist < 12.5f * scale) ring_ch = U'·';

                    uint32_t ring_col = in_b_ring
                        ? Color::Rgb(static_cast<uint8_t>(235 * ring_lit),
                                     static_cast<uint8_t>(215 * ring_lit),
                                     static_cast<uint8_t>(165 * ring_lit))
                        : Color::Rgb(static_cast<uint8_t>(180 * ring_lit),
                                     static_cast<uint8_t>(165 * ring_lit),
                                     static_cast<uint8_t>(130 * ring_lit));

                    frame[r, c] = Cell{ring_ch, ring_col, Color::Reset};
                }
                else if (hit_sphere) {
                    // Sphere surface: horizontal atmospheric cloud bands
                    float band = (std::sin(sny * 12.0f + t * 0.2f) + 1.0f) * 0.5f;

                    // Golden-amber planetary palette
                    uint32_t c_gold  = Color::Rgb(240, 205, 140);
                    uint32_t c_amber = Color::Rgb(205, 160, 95);
                    uint32_t c_base  = Color::lerp(c_amber, c_gold, band);

                    // Shading density characters
                    char32_t ch = U'█';
                    if (diffuse < 0.25f) ch = U'░';
                    else if (diffuse < 0.50f) ch = U'▒';
                    else if (diffuse < 0.75f) ch = U'▓';

                    uint8_t red   = static_cast<uint8_t>(std::clamp(static_cast<float>((c_base >> 16) & 0xFF) * diffuse, 15.0f, 255.0f));
                    uint8_t green = static_cast<uint8_t>(std::clamp(static_cast<float>((c_base >> 8)  & 0xFF) * diffuse, 12.0f, 255.0f));
                    uint8_t blue  = static_cast<uint8_t>(std::clamp(static_cast<float>( c_base        & 0xFF) * diffuse, 8.0f,  255.0f));

                    frame[r, c] = Cell{ch, Color::Rgb(red, green, blue), Color::Reset};
                }
            }
        }

        // 4. Draw Moon Titan with Planetary Occlusion
        if (titan_row >= 0 && titan_row < screen.height && titan_col >= 0 && titan_col < screen.width) {
            float tx = static_cast<float>(titan_col) - cx;
            float ty = (static_cast<float>(titan_row) - cy) * 2.0f;
            float t_dist_sq = tx * tx + ty * ty;
            bool occluded = false;

            // If behind the planet body, Titan is hidden
            if (t_dist_sq <= R * R && titan_z < 0.0f) {
                occluded = true;
            }

            if (!occluded) {
                frame[titan_row, titan_col] = Cell{U'●', Color::Rgb(255, 215, 120), Color::Reset};
            }
        }

        // 5. Telemetry & Control HUD (Top-Left)
        Box()
            .title(" Saturn Orbit Simulator ")
            .border_style(BorderStyle::Rounded)
            .fg(Color::DarkGray)
            .title_fg(Color::Yellow)
            .draw(frame, Area(1, 2, 42, 7));

        Text("Target: Saturn (Sol VI) | System: 60 FPS")
            .fg(Color::White)
            .draw(frame, Area(2, 4, 38, 1));

        std::string inc_str = "Ring Tilt: " + std::to_string(static_cast<int>(pitch * 57.295f)) +
                              "° | Moon: Titan (" + (titan_z > 0.0f ? "Front" : "Back") + ")";
        Text(inc_str)
            .fg(Color::Cyan)
            .draw(frame, Area(3, 4, 38, 1));

        std::string status_str = paused ? "Status: [PAUSED] (Space to Resume)" : "Status: [ORBITING - Smooth Diff]";
        Text(status_str)
            .fg(paused ? Color::Red : Color::Green)
            .draw(frame, Area(4, 4, 38, 1));

        Text("Controls: Arrows=Move, +/-=Scale, Q=Quit")
            .fg(Color::LightGray)
            .draw(frame, Area(5, 4, 38, 1));
    });

    return app.run() ? 0 : 1;
}
