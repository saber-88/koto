#include <koto/koto.hpp>
#include <cmath>
#include <vector>
#include <string>
#include <algorithm>
#include <random>

using namespace koto;

enum class SceneMode : int {
    Gargantua = 1,
    CyberGrid = 2,
    QuantumKnot = 3,
    HyperspaceWarp = 4
};

struct WarpStar {
    float x, y, z;
    float prev_z;
    uint32_t color;
};

struct QuantumParticle {
    float x, y, z;
    float orbit_speed;
    float phase;
    uint32_t color;
};

int main()
{
    App app;

    int tick = 0;
    bool paused = false;
    SceneMode mode = SceneMode::Gargantua;

    float cam_pitch = 0.0f;
    float cam_yaw   = 0.0f;
    float zoom      = 1.0f;

    // 1. Hyperspace Warp Stars
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist_xy(-120.0f, 120.0f);
    std::uniform_real_distribution<float> dist_z(5.0f, 180.0f);

    std::vector<WarpStar> warp_stars;
    warp_stars.reserve(260);
    for (int i = 0; i < 260; ++i) {
        float z = dist_z(rng);
        warp_stars.push_back({
            dist_xy(rng),
            dist_xy(rng),
            z,
            z,
            (i % 5 == 0) ? Color::Rgb(180, 225, 255) : Color::Rgb(255, 255, 255)
        });
    }

    // 2. Quantum Knot Particles
    std::uniform_real_distribution<float> part_angle(0.0f, 6.283185f);
    std::uniform_real_distribution<float> part_speed(0.02f, 0.06f);
    std::uniform_real_distribution<float> part_rad(14.0f, 26.0f);

    std::vector<QuantumParticle> quantum_particles;
    quantum_particles.reserve(90);
    for (int i = 0; i < 90; ++i) {
        float r = part_rad(rng);
        float phi = part_angle(rng);
        float theta = part_angle(rng);
        quantum_particles.push_back({
            r * std::sin(theta) * std::cos(phi),
            r * std::sin(theta) * std::sin(phi),
            r * std::cos(theta),
            part_speed(rng),
            phi,
            (i % 2 == 0) ? Color::Rgb(80, 240, 255) : Color::Rgb(255, 80, 220)
        });
    }

    // 3. Background Celestial Stars (for Black Hole)
    std::vector<std::pair<float, float>> bg_stars;
    std::uniform_real_distribution<float> star_angle(0.0f, 6.283185f);
    std::uniform_real_distribution<float> star_dist(12.0f, 90.0f);
    for (int i = 0; i < 110; ++i) {
        float a = star_angle(rng);
        float d = star_dist(rng);
        bg_stars.push_back({ std::cos(a) * d, std::sin(a) * d });
    }

    // Interactive Keybindings
    app.on_key([&](const KeyEvent& key) {
        if (key.ch == 'q' || key.ch == 'Q' || key.is(Key::Escape)) {
            app.quit();
        }
        else if (key.ch == ' ') {
            paused = !paused;
        }
        else if (key.ch == '1') {
            mode = SceneMode::Gargantua;
        }
        else if (key.ch == '2') {
            mode = SceneMode::CyberGrid;
        }
        else if (key.ch == '3') {
            mode = SceneMode::QuantumKnot;
        }
        else if (key.ch == '4') {
            mode = SceneMode::HyperspaceWarp;
        }
        else if (key.is(Key::Left) || key.ch == 'h' || key.ch == 'H') {
            cam_yaw -= 0.08f;
        }
        else if (key.is(Key::Right) || key.ch == 'l' || key.ch == 'L') {
            cam_yaw += 0.08f;
        }
        else if (key.is(Key::Up) || key.ch == 'k' || key.ch == 'K') {
            cam_pitch -= 0.06f;
        }
        else if (key.is(Key::Down) || key.ch == 'j' || key.ch == 'J') {
            cam_pitch += 0.06f;
        }
        else if (key.ch == '+' || key.ch == '=') {
            zoom = std::min(2.5f, zoom + 0.12f);
        }
        else if (key.ch == '-' || key.ch == '_') {
            zoom = std::max(0.5f, zoom - 0.12f);
        }
        else if (key.ch == 'r' || key.ch == 'R') {
            cam_pitch = 0.0f;
            cam_yaw   = 0.0f;
            zoom      = 1.0f;
        }
    });

    // 60 FPS Simulation Tick
    app.on_tick([&]() {
        if (!paused) {
            tick++;

            // Update Hyperspace Stars
            float warp_speed = (mode == SceneMode::HyperspaceWarp) ? 4.8f : 1.2f;
            for (auto& s : warp_stars) {
                s.prev_z = s.z;
                s.z -= warp_speed;
                if (s.z < 3.0f) {
                    s.z = 180.0f;
                    s.prev_z = 180.0f;
                }
            }

            // Update Quantum Particles
            for (auto& q : quantum_particles) {
                q.phase += q.orbit_speed;
                if (q.phase > 6.283185f) q.phase -= 6.283185f;
            }
        }
    });

    // 60 FPS Flicker-Free Render Pass with Half-Block Packing
    app.on_render([&](Frame& frame) {
        Area screen = app.area();
        if (screen.width < 25 || screen.height < 15) return;

        // Half-block 2x vertical subpixel resolution (Square Aspect Ratio)
        int sub_w = screen.width;
        int sub_h = screen.height * 2;
        float center_x = static_cast<float>(sub_w) * 0.5f;
        float center_y = static_cast<float>(sub_h) * 0.5f;

        float t = static_cast<float>(tick) * 0.035f;

        // Subpixel evaluation lambda per coordinate (x, y)
        auto render_subpixel = [&](float sx, float sy) -> uint32_t {
            // Apply camera pan & zoom
            float x = (sx - center_x) / zoom;
            float y = (sy - center_y) / zoom;

            // Apply camera 3D rotation (pitch & yaw)
            float cos_p = std::cos(cam_pitch), sin_p = std::sin(cam_pitch);
            float cos_y = std::cos(cam_yaw),   sin_y = std::sin(cam_yaw);

            float rx =  x * cos_y;
            float ry =  y * cos_p + (x * sin_y) * sin_p;
            x = rx;
            y = ry;

            // =================================================================
            // SCENE 1: Gargantua Relativistic Black Hole with Gravitational Lensing
            // =================================================================
            if (mode == SceneMode::Gargantua) {
                float dist = std::sqrt(x * x + y * y);
                float Rs = 9.5f; // Schwarzschild radius

                // 1. Event Horizon (Absolute Singularity Void)
                if (dist < Rs) {
                    return Color::Rgb(0, 0, 0);
                }

                // 2. Razor-Thin Photon Sphere
                if (dist < Rs * 1.14f) {
                    float p_ratio = (dist - Rs) / (Rs * 0.14f);
                    return Color::lerp(Color::Rgb(255, 255, 250), Color::Rgb(255, 200, 80), p_ratio);
                }

                // 3. Accretion Disk Physics: Direct & Gravitationally Deflected Rays
                float tilt_sin = 0.36f; // ~21 deg inclination
                float r_in = 11.5f;
                float r_out = 32.0f;

                // A. Direct hit on disk plane
                float d_direct = std::sqrt(x * x + (y / tilt_sin) * (y / tilt_sin));

                // B. Gravitational lensing deflection (ray bent over/under the event horizon)
                float defl = (Rs * 3.2f) / (dist - Rs * 0.72f);
                float y_lens = (y > 0.0f) ? (y - defl) : (y + defl);
                float d_lens = std::sqrt(x * x + (y_lens / tilt_sin) * (y_lens / tilt_sin));

                bool hit_direct = (d_direct >= r_in && d_direct <= r_out);
                bool hit_lens   = (d_lens >= r_in && d_lens <= r_out);

                if (hit_direct || hit_lens) {
                    float r_disk = hit_direct ? d_direct : d_lens;

                    // Relativistic Doppler beaming
                    // Left side approaches observer at ~0.5c -> boosted cyan/white hot!
                    // Right side recedes -> dimmed red/orange.
                    float dop = -x / r_disk;
                    float beam = std::clamp(1.0f + dop * 0.85f, 0.2f, 2.2f);

                    // Plasma swirl turbulence
                    float angle = std::atan2(y, x);
                    float swirl = (std::sin(r_disk * 1.8f - t * 4.5f + angle * 3.0f) + 1.0f) * 0.5f;

                    // Gradient: Relativistic Cyan/White -> Solar Gold -> Deep Red
                    uint32_t c_hot  = Color::Rgb(190, 245, 255);
                    uint32_t c_mid  = Color::Rgb(255, 195, 75);
                    uint32_t c_cool = Color::Rgb(220, 60, 20);

                    uint32_t base_c;
                    if (dop > 0.15f) {
                        base_c = Color::lerp(c_mid, c_hot, (dop - 0.15f) / 0.85f);
                    } else {
                        base_c = Color::lerp(c_cool, c_mid, (dop + 1.0f) / 1.15f);
                    }

                    // Modulate by swirl and beaming
                    float intensity = beam * (0.8f + swirl * 0.4f);
                    uint8_t red   = static_cast<uint8_t>(std::clamp(static_cast<float>((base_c >> 16) & 0xFF) * intensity, 10.0f, 255.0f));
                    uint8_t green = static_cast<uint8_t>(std::clamp(static_cast<float>((base_c >> 8)  & 0xFF) * intensity, 5.0f,  255.0f));
                    uint8_t blue  = static_cast<uint8_t>(std::clamp(static_cast<float>( base_c        & 0xFF) * intensity, 5.0f,  255.0f));

                    return Color::Rgb(red, green, blue);
                }

                // 4. Background Starfield with Gravitational Lensing Distortion
                for (const auto& star : bg_stars) {
                    float s_dx = x - star.first;
                    float s_dy = y - star.second;
                    if (s_dx * s_dx + s_dy * s_dy < 1.4f) {
                        return Color::Rgb(220, 230, 255);
                    }
                }

                // Deep interstellar space void
                return Color::Rgb(4, 5, 10);
            }

            // =================================================================
            // SCENE 2: Cyberpunk 80s — Neon Perspective Horizon & Chrome Gyroscope
            // =================================================================
            if (mode == SceneMode::CyberGrid) {
                float horizon = 4.0f;

                // A. Synthwave Sky & Sunset
                if (y < horizon) {
                    float sky_t = std::clamp((y + center_y) / (horizon + center_y), 0.0f, 1.0f);
                    uint32_t sky_col = Color::lerp(Color::Rgb(15, 5, 30), Color::Rgb(130, 20, 95), sky_t);

                    // Giant Neon Sun on Horizon
                    float sun_dx = x;
                    float sun_dy = y - (horizon - 6.0f);
                    float sun_dist = std::sqrt(sun_dx * sun_dx + sun_dy * sun_dy);
                    if (sun_dist <= 16.0f) {
                        // Horizontal laser blind stripes
                        int stripe = static_cast<int>(sun_dy + 20.0f) % 4;
                        if (stripe != 0 || sun_dy < 0.0f) {
                            float sun_t = (sun_dy + 16.0f) / 32.0f;
                            return Color::lerp(Color::Rgb(255, 230, 80), Color::Rgb(255, 40, 140), sun_t);
                        }
                    }

                    // Floating 3D Chrome Core in the sky
                    float core_dist = std::sqrt(x * x + (y + 1.0f) * (y + 1.0f));
                    if (core_dist <= 7.0f) {
                        float cz = std::sqrt(49.0f - core_dist * core_dist);
                        float cnx = x / 7.0f, cny = (y + 1.0f) / 7.0f, cnz = cz / 7.0f;

                        // Phong specular highlight
                        float lx = -0.577f, ly = -0.577f, lz = 0.577f;
                        float diff = std::max(0.1f, cnx * lx + cny * ly + cnz * lz);
                        float spec = std::pow(std::max(0.0f, cnx * lx + cny * ly + cnz * lz), 16.0f);

                        uint32_t base = Color::lerp(Color::Rgb(30, 20, 50), Color::Rgb(240, 100, 255), diff);
                        uint8_t sr = static_cast<uint8_t>(std::clamp(static_cast<float>((base >> 16) & 0xFF) + spec * 220.0f, 0.0f, 255.0f));
                        uint8_t sg = static_cast<uint8_t>(std::clamp(static_cast<float>((base >> 8)  & 0xFF) + spec * 220.0f, 0.0f, 255.0f));
                        uint8_t sb = static_cast<uint8_t>(std::clamp(static_cast<float>( base        & 0xFF) + spec * 255.0f, 0.0f, 255.0f));
                        return Color::Rgb(sr, sg, sb);
                    }

                    return sky_col;
                }

                // B. 3D Infinite Perspective Floor with Rolling Cyber Waves
                float dy_floor = y - horizon + 0.1f;
                float depth_z = 24.0f / dy_floor; // Perspective depth
                float world_x = x * depth_z * 0.12f;
                float world_z = depth_z + t * 4.0f; // Rolling forward

                // Grid line calculation
                float gx = std::abs(world_x - std::round(world_x));
                float gz = std::abs(world_z - std::round(world_z));
                bool is_grid = (gx < 0.08f || gz < 0.08f);

                // Depth fog
                float fog = std::clamp(1.0f - (depth_z / 42.0f), 0.0f, 1.0f);
                if (is_grid) {
                    uint32_t neon = Color::lerp(Color::Rgb(255, 0, 180), Color::Rgb(0, 240, 255), gx < 0.08f ? 1.0f : 0.0f);
                    return Color::lerp(Color::Rgb(15, 5, 25), neon, fog);
                } else {
                    return Color::lerp(Color::Rgb(10, 0, 20), Color::Rgb(30, 5, 45), fog);
                }
            }

            // =================================================================
            // SCENE 3: Quantum Knot — 3D Hyper-Torus Knot with Volumetric Aura
            // =================================================================
            if (mode == SceneMode::QuantumKnot) {
                // 3D Torus knot projection (p=2, q=3 trefoil)
                float min_dist_to_knot = 999.0f;
                float knot_depth = 0.0f;

                float knot_t = t * 0.8f;
                for (int i = 0; i < 64; ++i) {
                    float phi = static_cast<float>(i) * (6.283185f / 64.0f);
                    float r_major = 13.0f;
                    float r_minor = 5.5f;

                    // 3D parametric equations
                    float kx = (r_major + r_minor * std::cos(3.0f * phi)) * std::cos(2.0f * phi);
                    float ky = (r_major + r_minor * std::cos(3.0f * phi)) * std::sin(2.0f * phi);
                    float kz = r_minor * std::sin(3.0f * phi);

                    // 3D dynamic rotation
                    float cos_a = std::cos(knot_t), sin_a = std::sin(knot_t);
                    float rx_k =  kx * cos_a - kz * sin_a;
                    float rz_k =  kx * sin_a + kz * cos_a;
                    float ry_k =  ky * std::cos(knot_t * 0.7f) - rz_k * std::sin(knot_t * 0.7f);

                    float d = std::sqrt((x - rx_k) * (x - rx_k) + (y - ry_k) * (y - ry_k));
                    if (d < min_dist_to_knot) {
                        min_dist_to_knot = d;
                        knot_depth = rz_k;
                    }
                }

                // Render illuminated knot tube
                float tube_radius = 2.4f;
                if (min_dist_to_knot <= tube_radius) {
                    float norm = min_dist_to_knot / tube_radius;
                    float depth_fade = std::clamp((knot_depth + 18.0f) / 36.0f, 0.2f, 1.0f);

                    uint32_t knot_col = Color::lerp(Color::Rgb(255, 255, 255), Color::Rgb(0, 255, 180), norm);
                    uint8_t red   = static_cast<uint8_t>(static_cast<float>((knot_col >> 16) & 0xFF) * depth_fade);
                    uint8_t green = static_cast<uint8_t>(static_cast<float>((knot_col >> 8)  & 0xFF) * depth_fade);
                    uint8_t blue  = static_cast<uint8_t>(static_cast<float>( knot_col        & 0xFF) * depth_fade);
                    return Color::Rgb(red, green, blue);
                }

                // Glow aura around knot
                if (min_dist_to_knot <= tube_radius * 2.8f) {
                    float glow = 1.0f - (min_dist_to_knot - tube_radius) / (tube_radius * 1.8f);
                    return Color::lerp(Color::Rgb(5, 10, 20), Color::Rgb(0, 140, 110), glow * 0.6f);
                }

                // Quantum cloud particles
                for (const auto& q : quantum_particles) {
                    float px = q.x * std::cos(t * 0.5f) - q.z * std::sin(t * 0.5f);
                    float py = q.y;
                    if ((x - px) * (x - px) + (y - py) * (y - py) < 1.2f) {
                        return q.color;
                    }
                }

                return Color::Rgb(5, 8, 18);
            }

            // =================================================================
            // SCENE 4: Hyperspace Warp — 3D Lightspeed Star Streaks
            // =================================================================
            {
                // Dark hyper-tunnel backdrop with subtle radial glow
                float dist = std::sqrt(x * x + y * y);
                float tunnel_glow = std::clamp(1.0f - (dist / 65.0f), 0.0f, 0.4f);
                uint32_t bg = Color::lerp(Color::Rgb(0, 2, 8), Color::Rgb(10, 40, 90), tunnel_glow);

                // 3D Motion blur projected stars
                for (const auto& s : warp_stars) {
                    float fov = 48.0f;
                    float sx_curr = (s.x / s.z) * fov;
                    float sy_curr = (s.y / s.z) * fov;
                    float sx_prev = (s.x / s.prev_z) * fov;
                    float sy_prev = (s.y / s.prev_z) * fov;

                    // Distance to streak line segment
                    float ldx = sx_curr - sx_prev;
                    float ldy = sy_curr - sy_prev;
                    float len_sq = ldx * ldx + ldy * ldy;

                    if (len_sq > 0.01f) {
                        float t_proj = std::clamp(((x - sx_prev) * ldx + (y - sy_prev) * ldy) / len_sq, 0.0f, 1.0f);
                        float near_x = sx_prev + t_proj * ldx;
                        float near_y = sy_prev + t_proj * ldy;
                        float dist_sq = (x - near_x) * (x - near_x) + (y - near_y) * (y - near_y);

                        if (dist_sq < 1.2f) {
                            float brightness = std::clamp((180.0f - s.z) / 140.0f, 0.2f, 1.0f);
                            return Color::lerp(bg, s.color, brightness);
                        }
                    } else {
                        if ((x - sx_curr) * (x - sx_curr) + (y - sy_curr) * (y - sy_curr) < 1.0f) {
                            return s.color;
                        }
                    }
                }

                return bg;
            }
        };

        // Render full screen using packed half-blocks (2 vertical subpixels per cell)
        for (int r = 0; r < screen.height; ++r) {
            for (int c = 0; c < screen.width; ++c) {
                float sx = static_cast<float>(c);
                float sy_top = static_cast<float>(r * 2);
                float sy_bot = static_cast<float>(r * 2 + 1);

                uint32_t col_top = render_subpixel(sx, sy_top);
                uint32_t col_bot = render_subpixel(sx, sy_bot);

                frame[r, c] = Cell{U'▀', col_top, col_bot};
            }
        }

        // Floating Sci-Fi Hologram HUD (Top-Left)
        Box()
            .title(" 3D Engine Demoparty Showcase ")
            .border_style(BorderStyle::Rounded)
            .fg(Color::DarkGray)
            .title_fg(Color::Yellow)
            .draw(frame, Area(1, 2, 48, 8));

        std::string mode_str;
        switch (mode) {
            case SceneMode::Gargantua:      mode_str = "[1] Gargantua: Relativistic Black Hole"; break;
            case SceneMode::CyberGrid:      mode_str = "[2] Cyberpunk 80s: Synthwave Grid"; break;
            case SceneMode::QuantumKnot:    mode_str = "[3] Quantum Core: 3D Torus Knot"; break;
            case SceneMode::HyperspaceWarp: mode_str = "[4] Hyperspace: 3D Lightspeed Warp"; break;
        }

        Text("Active Scene: " + mode_str)
            .fg(Color::Cyan)
            .draw(frame, Area(2, 4, 44, 1));

        std::string res_str = "Subpixel Canvas: " + std::to_string(sub_w) + "x" + std::to_string(sub_h) +
                              " TrueColor | 60 FPS Diff";
        Text(res_str)
            .fg(Color::Green)
            .draw(frame, Area(3, 4, 44, 1));

        std::string cam_str = "Pitch: " + std::to_string(static_cast<int>(cam_pitch * 57.295f)) +
                              "° | Yaw: " + std::to_string(static_cast<int>(cam_yaw * 57.295f)) +
                              "° | Zoom: " + std::to_string(zoom).substr(0, 4) + "x";
        Text(cam_str)
            .fg(Color::White)
            .draw(frame, Area(4, 4, 44, 1));

        Text("Keys: [1-4]=Switch Scene, Arrows=Rotate, +/-=Zoom")
            .fg(Color::LightGray)
            .draw(frame, Area(5, 4, 44, 1));

        Text("Status: " + std::string(paused ? "[PAUSED]" : "[REAL-TIME 3D]") + " | [Space]=Pause, [Q]=Quit")
            .fg(paused ? Color::Red : Color::Yellow)
            .draw(frame, Area(6, 4, 44, 1));
    });

    app.run();
    return 0;
}
