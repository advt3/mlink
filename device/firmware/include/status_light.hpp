#ifndef STATUS_LIGHT_HPP
#define STATUS_LIGHT_HPP

#include <cstdint>

class StatusLight {
public:
    struct Color {
        uint8_t r, g, b;
        static constexpr Color Black() { return {0, 0, 0}; }
        static constexpr Color Green() { return {0, 0x1F, 0}; }
    };

    StatusLight() = default;

    bool initialize();
    void set_color(Color color);
    
    // Inline common logic to keep implementation files focused
    void set_state(bool on) {
        set_color(on ? Color::Green() : Color::Black());
    }

private:
    bool m_initialized = false;
};

#endif // STATUS_LIGHT_HPP
