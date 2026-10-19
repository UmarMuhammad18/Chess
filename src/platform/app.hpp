#pragma once
#include "sdl_context.hpp"
#include "input.hpp"
#include <functional>

namespace platform {
    class App {
    public:
        using EventHook = std::function<void(const SDL_Event&)>;

        bool init();
        void poll_events();
        bool is_running() const { return !input_sys.quit_requested(); }
        // Every SDL event is also passed to the hook (used to feed Dear ImGui)
        void set_event_hook(EventHook hook) { event_hook = std::move(hook); }
        SDLContext& get_sdl() { return sdl; }
        const Input& get_input() const { return input_sys; }
    private:
        SDLContext sdl;
        Input input_sys;
        EventHook event_hook;
    };
}
