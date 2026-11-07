#include <koto/koto.hpp>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <random>

using namespace koto;

struct Moon {
    std::string name;
    float dist;
    float speed;
    uint32_t color;
    char32_t sym;
};

struct Planet {
    std::string name;
    std::string symbol_char;
    uint32_t color;
    float orbit_r;      // Base orbital radius (AU-scaled)
    float speed;        // Orbital speed (rad/tick)
    float angle;        // Current orbital angle (rad)
    std::string type;
    std::string distance_au;
    std::string period_str;
    bool has_rings;
    std::vector<Moon> moons;
};

struct Asteroid {
    float orbit_r;
    float speed;
    float angle;
    char32_t sym;
    uint32_t color;
};

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
    bool show_orbits = true;
    float time_scale = 1.0f;
    float zoom = 1.0f;

    float pan_x = 0.0f;
    float pan_y = 0.0f;
    int locked_target = 0; // 0 = Sun, 1..9 = Planets

    // 1. Initialize Celestial Bodies
    std::vector<Planet> planets = {
        {
            .name = "Sun",
            .symbol_char = "☀",
            .color = Color::Rgb(255, 220, 60),
            .orbit_r = 0.0f,
            .speed = 0.0f,
            .angle = 0.0f,
            .type = "Yellow Dwarf Star (G2V)",
            .distance_au = "0.00 AU (Center)",
            .period_str = "230M yr Galactic Orbit",
            .has_rings = false,
            .moons = {}
        },
        {
            .name = "Mercury",
            .symbol_char = "•",
            .color = Color::Rgb(180, 185, 195),
            .orbit_r = 7.5f,
            .speed = 0.045f,
            .angle = 0.5f,
            .type = "Terrestrial Planet",
            .distance_au = "0.39 AU",
            .period_str = "88.0 Earth Days",
            .has_rings = false,
            .moons = {}
        },
        {
            .name = "Venus",
            .symbol_char = "●",
            .color = Color::Rgb(255, 215, 115),
            .orbit_r = 12.0f,
            .speed = 0.032f,
            .angle = 2.1f,
            .type = "Terrestrial Planet",
            .distance_au = "0.72 AU",
            .period_str = "224.7 Earth Days",
            .has_rings = false,
            .moons = {}
        },
        {
            .name = "Earth",
            .symbol_char = "⊕",
            .color = Color::Rgb(70, 160, 250),
            .orbit_r = 17.5f,
            .speed = 0.024f,
            .angle = 4.0f,
            .type = "Habitable Terrestrial",
            .distance_au = "1.00 AU",
            .period_str = "365.25 Earth Days",
            .has_rings = false,
            .moons = {
                { "Luna", 2.2f, 0.08f, Color::Rgb(220, 225, 230), U'·' }
            }
        },
        {
            .name = "Mars",
            .symbol_char = "•",
            .color = Color::Rgb(255, 95, 50),
            .orbit_r = 23.5f,
            .speed = 0.018f,
            .angle = 1.2f,
            .type = "Terrestrial Planet",
            .distance_au = "1.52 AU",
            .period_str = "687.0 Earth Days",
            .has_rings = false,
            .moons = {
                { "Phobos", 1.5f, 0.12f, Color::Rgb(160, 160, 160), U'·' }
            }
        },
        {
            .name = "Jupiter",
            .symbol_char = "●",
            .color = Color::Rgb(255, 175, 75),
            .orbit_r = 34.0f,
            .speed = 0.010f,
            .angle = 5.2f,
            .type = "Gas Giant (Largest)",
            .distance_au = "5.20 AU",
            .period_str = "11.86 Earth Years",
            .has_rings = false,
            .moons = {
                { "Io",       2.0f, 0.09f, Color::Rgb(255, 230, 100), U'·' },
                { "Europa",   2.8f, 0.06f, Color::Rgb(200, 230, 255), U'·' },
                { "Ganymede", 3.6f, 0.04f, Color::Rgb(210, 200, 180), U'•' },
                { "Callisto", 4.4f, 0.03f, Color::Rgb(160, 150, 140), U'·' }
            }
        },
        {
            .name = "Saturn",
            .symbol_char = "●",
            .color = Color::Rgb(250, 220, 120),
            .orbit_r = 46.0f,
            .speed = 0.007f,
            .angle = 3.3f,
            .type = "Ringed Gas Giant",
            .distance_au = "9.58 AU",
            .period_str = "29.45 Earth Years",
            .has_rings = true,
            .moons = {
                { "Titan", 3.2f, 0.05f, Color::Rgb(255, 200, 90), U'•' }
            }
        },
        {
            .name = "Uranus",
            .symbol_char = "●",
            .color = Color::Rgb(60, 220, 230),
            .orbit_r = 58.0f,
            .speed = 0.0045f,
            .angle = 0.8f,
            .type = "Ice Giant",
            .distance_au = "19.22 AU",
            .period_str = "84.0 Earth Years",
            .has_rings = false,
            .moons = {
                { "Titania", 2.2f, 0.05f, Color::Rgb(200, 240, 250), U'·' }
            }
        },
        {
            .name = "Neptune",
            .symbol_char = "●",
            .color = Color::Rgb(40, 130, 245),
            .orbit_r = 70.0f,
            .speed = 0.0032f,
            .angle = 2.8f,
            .type = "Ice Giant (Furthest)",
            .distance_au = "30.05 AU",
            .period_str = "164.8 Earth Years",
            .has_rings = false,
            .moons = {
                { "Triton", 2.4f, 0.06f, Color::Rgb(180, 210, 255), U'·' }
            }
        },
        {
            .name = "Pluto",
            .symbol_char = "•",
            .color = Color::Rgb(190, 180, 170),
            .orbit_r = 82.0f,
            .speed = 0.0022f,
            .angle = 4.8f,
            .type = "Kuiper Belt Dwarf Planet",
            .distance_au = "39.48 AU",
            .period_str = "248.0 Earth Years",
            .has_rings = false,
            .moons = {
                { "Charon", 1.4f, 0.07f, Color::Rgb(150, 145, 140), U'·' }
            }
        }
    };

    // 2. Procedural Asteroid Belt (between Mars and Jupiter)
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> ast_r(27.0f, 30.5f);
    std::uniform_real_distribution<float> ast_angle(0.0f, 6.28318f);
    std::uniform_real_distribution<float> ast_speed(0.012f, 0.016f);

    std::vector<Asteroid> asteroids;
    for (int i = 0; i < 65; ++i) {
        asteroids.push_back({
            .orbit_r = ast_r(rng),
            .speed = ast_speed(rng),
            .angle = ast_angle(rng),
            .sym = (i % 2 == 0) ? U'·' : U'.',
            .color = Color::Rgb(120, 115, 110)
        });
    }

    // 3. Deep Space Starfield
    std::uniform_int_distribution<int> star_r(0, 120);
    std::uniform_int_distribution<int> star_c(0, 240);
    std::uniform_real_distribution<float> star_phase(0.0f, 6.28f);
    std::uniform_real_distribution<float> star_speed(0.02f, 0.08f);

    std::vector<Star> stars;
    for (int i = 0; i < 120; ++i) {
        char32_t sym = U'·';
        if (i % 8 == 0) sym = U'★';
        else if (i % 4 == 0) sym = U'+';
        else if (i % 3 == 0) sym = U'✦';

        stars.push_back({
            star_r(rng),
            star_c(rng),
            star_phase(rng),
            star_speed(rng),
            sym
        });
    }

    // 4. Keyboard Controls
    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.ch == 'Q' || key.is(Key::Escape)) {
            app.quit();
        }
        else if (key.ch == ' ') {
            paused = !paused;
        }
        else if (key.ch == 't' || key.ch == 'T') {
            show_orbits = !show_orbits;
        }
        else if (key.ch >= '0' && key.ch <= '9') {
            locked_target = static_cast<int>(key.ch - static_cast<uint32_t>('0'));
        }
        else if (key.is(Key::Left) || key.ch == 'h' || key.ch == 'H') {
            locked_target = -1; // Unlink target lock
            pan_x -= 3.0f;
        }
        else if (key.is(Key::Right) || key.ch == 'l' || key.ch == 'L') {
            locked_target = -1;
            pan_x += 3.0f;
        }
        else if (key.is(Key::Up) || key.ch == 'k' || key.ch == 'K') {
            locked_target = -1;
            pan_y -= 1.5f;
        }
        else if (key.is(Key::Down) || key.ch == 'j' || key.ch == 'J') {
            locked_target = -1;
            pan_y += 1.5f;
        }
        else if (key.ch == '+' || key.ch == '=') {
            zoom = std::min(2.5f, zoom + 0.15f);
        }
        else if (key.ch == '-' || key.ch == '_') {
            zoom = std::max(0.4f, zoom - 0.15f);
        }
        else if (key.ch == ']' || key.ch == '}') {
            time_scale = std::min(4.0f, time_scale + 0.25f);
        }
        else if (key.ch == '[' || key.ch == '{') {
            time_scale = std::max(0.25f, time_scale - 0.25f);
        }
        else if (key.ch == 'r' || key.ch == 'R') {
            pan_x = 0.0f;
            pan_y = 0.0f;
            zoom = 1.0f;
            time_scale = 1.0f;
            locked_target = 0;
        }
    });

    // 5. 60 FPS Orbital Tick
    app.on_tick([&]() {
        if (!paused) {
            tick++;
            float dt = time_scale;

            // Advance planets
            for (auto& p : planets) {
                p.angle += p.speed * dt;
                if (p.angle > 6.283185f) p.angle -= 6.283185f;
            }

            // Advance asteroid belt
            for (auto& a : asteroids) {
                a.angle += a.speed * dt;
                if (a.angle > 6.283185f) a.angle -= 6.283185f;
            }
        }
    });

    // 6. 60 FPS Flicker-Free Render Pass
    app.on_render([&](Frame& frame) {
        Area screen = app.area();
        if (screen.width < 20 || screen.height < 15) return;

        float t = static_cast<float>(tick) * 0.03f;

        // Base center of screen
        float origin_c = static_cast<float>(screen.width) * 0.5f;
        float origin_r = static_cast<float>(screen.height) * 0.5f;

        // Calculate planet positions in world coordinates (relative to Sun)
        // (Account for 2:1 character cell aspect ratio with 0.48 vertical factor)
        std::vector<std::pair<float, float>> pos(planets.size());
        for (size_t i = 0; i < planets.size(); ++i) {
            if (i == 0) {
                pos[i] = {0.0f, 0.0f}; // Sun is at origin
            } else {
                float dist = planets[i].orbit_r * zoom;
                float px = std::cos(planets[i].angle) * dist;
                float py = std::sin(planets[i].angle) * (dist * 0.48f);
                pos[i] = {px, py};
            }
        }

        // Camera positioning: lock onto target body or free-pan
        float cam_x = pan_x;
        float cam_y = pan_y;
        if (locked_target >= 0 && locked_target < static_cast<int>(planets.size())) {
            cam_x = pos[static_cast<size_t>(locked_target)].first;
            cam_y = pos[static_cast<size_t>(locked_target)].second;
        }

        float sun_col = origin_c - cam_x;
        float sun_row = origin_r - cam_y;

        // A. Draw Starfield
        for (const auto& star : stars) {
            int sr = star.r % screen.height;
            int sc = star.c % screen.width;

            float tw = (std::sin(t * star.twinkle_speed * 40.0f + star.phase) + 1.0f) * 0.5f;
            uint32_t star_color = Color::lerp(Color::Rgb(30, 35, 50), Color::Rgb(210, 220, 255), tw);
            frame[sr, sc] = Cell{star.sym, star_color, Color::Reset};
        }

        // B. Draw Orbital Ellipses (if enabled)
        if (show_orbits) {
            for (size_t i = 1; i < planets.size(); ++i) {
                float r_c = planets[i].orbit_r * zoom;
                float r_r = r_c * 0.48f;

                // Sample orbital perimeter
                int steps = std::clamp(static_cast<int>(r_c * 3.14f), 30, 300);
                for (int s = 0; s < steps; ++s) {
                    float theta = (static_cast<float>(s) / static_cast<float>(steps)) * 6.283185f;
                    int ec = static_cast<int>(sun_col + std::cos(theta) * r_c);
                    int er = static_cast<int>(sun_row + std::sin(theta) * r_r);

                    if (er >= 0 && er < screen.height && ec >= 0 && ec < screen.width) {
                        // Avoid drawing orbit over existing non-empty cells
                        if (frame[er, ec].ch == U' ' || frame[er, ec].ch == U'·') {
                            frame[er, ec] = Cell{U'·', Color::Rgb(40, 48, 65), Color::Reset};
                        }
                    }
                }
            }
        }

        // C. Draw Asteroid Belt
        for (const auto& ast : asteroids) {
            float dist = ast.orbit_r * zoom;
            int ac = static_cast<int>(sun_col + std::cos(ast.angle) * dist);
            int ar = static_cast<int>(sun_row + std::sin(ast.angle) * (dist * 0.48f));

            if (ar >= 0 && ar < screen.height && ac >= 0 && ac < screen.width) {
                frame[ar, ac] = Cell{ast.sym, ast.color, Color::Reset};
            }
        }

        // D. Draw Sun with Pulsing Solar Corona
        float sun_pulse = (std::sin(t * 3.0f) + 1.0f) * 0.5f;
        uint32_t sun_core_col = Color::lerp(Color::Rgb(255, 235, 90), Color::Rgb(255, 255, 200), sun_pulse);
        uint32_t corona_col   = Color::lerp(Color::Rgb(255, 120, 20), Color::Rgb(255, 180, 40), sun_pulse);

        int sc_int = static_cast<int>(sun_col);
        int sr_int = static_cast<int>(sun_row);

        // Corona flares
        if (sr_int - 1 >= 0 && sr_int - 1 < screen.height && sc_int >= 0 && sc_int < screen.width)
            frame[sr_int - 1, sc_int] = Cell{U'✦', corona_col, Color::Reset};
        if (sr_int + 1 >= 0 && sr_int + 1 < screen.height && sc_int >= 0 && sc_int < screen.width)
            frame[sr_int + 1, sc_int] = Cell{U'✦', corona_col, Color::Reset};
        if (sr_int >= 0 && sr_int < screen.height && sc_int - 1 >= 0 && sc_int - 1 < screen.width)
            frame[sr_int, sc_int - 1] = Cell{U'☼', corona_col, Color::Reset};
        if (sr_int >= 0 && sr_int < screen.height && sc_int + 1 >= 0 && sc_int + 1 < screen.width)
            frame[sr_int, sc_int + 1] = Cell{U'☼', corona_col, Color::Reset};

        // Solar Core
        if (sr_int >= 0 && sr_int < screen.height && sc_int >= 0 && sc_int < screen.width) {
            frame[sr_int, sc_int] = Cell{U'☀', sun_core_col, Color::Reset};
        }

        // E. Draw Planets and their Moons
        for (size_t i = 1; i < planets.size(); ++i) {
            const auto& p = planets[i];
            int pc = static_cast<int>(sun_col + pos[i].first);
            int pr = static_cast<int>(sun_row + pos[i].second);

            if (pr >= 0 && pr < screen.height && pc >= 0 && pc < screen.width) {
                // If Saturn, draw ring system flanking the planet: ═(●)═
                if (p.has_rings) {
                    if (pc - 1 >= 0) frame[pr, pc - 1] = Cell{U'═', Color::Rgb(215, 195, 140), Color::Reset};
                    frame[pr, pc]     = Cell{U'●', p.color, Color::Reset};
                    if (pc + 1 < screen.width) frame[pr, pc + 1] = Cell{U'═', Color::Rgb(215, 195, 140), Color::Reset};
                } else {
                    char32_t sym = (p.symbol_char == "•") ? U'•' : ((p.symbol_char == "⊕") ? U'⊕' : U'●');
                    frame[pr, pc] = Cell{sym, p.color, Color::Reset};
                }

                // Draw Planet Moons
                for (size_t m = 0; m < p.moons.size(); ++m) {
                    const auto& moon = p.moons[m];
                    float m_angle = t * moon.speed * 40.0f + static_cast<float>(m) * 1.57f;
                    int mc = static_cast<int>(static_cast<float>(pc) + std::cos(m_angle) * (moon.dist * zoom));
                    int mr = static_cast<int>(static_cast<float>(pr) + std::sin(m_angle) * (moon.dist * zoom * 0.48f));

                    if (mr >= 0 && mr < screen.height && mc >= 0 && mc < screen.width) {
                        frame[mr, mc] = Cell{moon.sym, moon.color, Color::Reset};
                    }
                }
            }
        }

        // F. Planet HUD & Telemetry Box (Top-Left)
        Box()
            .title(" Solar System Telemetry ")
            .border_style(BorderStyle::Rounded)
            .fg(Color::DarkGray)
            .title_fg(Color::Yellow)
            .draw(frame, Area(1, 2, 46, 8));

        std::string target_name = (locked_target >= 0 && locked_target < static_cast<int>(planets.size()))
                                ? planets[static_cast<size_t>(locked_target)].name
                                : "Free Camera (Drifting)";
        Text("Locked Focus: " + target_name)
            .fg(Color::Cyan)
            .draw(frame, Area(2, 4, 42, 1));

        if (locked_target >= 0 && locked_target < static_cast<int>(planets.size())) {
            const auto& p = planets[static_cast<size_t>(locked_target)];
            std::string sub_info = "Type: " + p.type + " | Dist: " + p.distance_au;
            Text(sub_info)
                .fg(Color::White)
                .draw(frame, Area(3, 4, 42, 1));

            std::string orb_info = "Period: " + p.period_str +
                                   (p.moons.empty() ? "" : " | Moons: " + std::to_string(p.moons.size()));
            Text(orb_info)
                .fg(Color::LightGray)
                .draw(frame, Area(4, 4, 42, 1));
        } else {
            Text("Tip: Press 0-9 to lock camera on any body")
                .fg(Color::LightGray)
                .draw(frame, Area(3, 4, 42, 1));
            Text("Camera Pan: (" + std::to_string(static_cast<int>(pan_x)) + ", " +
                 std::to_string(static_cast<int>(pan_y)) + ")")
                .fg(Color::White)
                .draw(frame, Area(4, 4, 42, 1));
        }

        std::string speed_str = "Time Warp: " + std::to_string(time_scale).substr(0, 4) +
                                "x | Zoom: " + std::to_string(zoom).substr(0, 4) +
                                "x | " + (paused ? "[PAUSED]" : "[RUNNING]");
        Text(speed_str)
            .fg(paused ? Color::Red : Color::Green)
            .draw(frame, Area(5, 4, 42, 1));

        Text("Controls: 0-9=Target, +/-=Zoom, [/]=Speed, T=Trails, Q=Quit")
            .fg(Color::DarkGray)
            .draw(frame, Area(6, 4, 42, 1));
    });

    return app.run() ? 0 : 1;
}
