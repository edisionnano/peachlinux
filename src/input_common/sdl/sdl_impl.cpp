// Copyright 2018 Citra Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <algorithm>
#include <atomic>
#include <cmath>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include <SDL.h>
#include "common/logging/log.h"
#include "common/math_util.h"
#include "common/param_package.h"
#include "common/threadsafe_queue.h"
#include "core/frontend/input.h"
#include "input_common/sdl/sdl_impl.h"

namespace InputCommon::SDL {

static std::string GetGUID(SDL_Joystick* joystick) {
    const SDL_JoystickGUID guid = SDL_JoystickGetGUID(joystick);
    char guid_str[33];
    SDL_JoystickGetGUIDString(guid, guid_str, sizeof(guid_str));
    return guid_str;
}

/// Creates a ParamPackage from an SDL_Event that can directly be used to create a ButtonDevice
static Common::ParamPackage SDLEventToButtonParamPackage(SDLState& state, const SDL_Event& event);

static int SDLEventWatcher(void* user_data, SDL_Event* event) {
    auto* const sdl_state = static_cast<SDLState*>(user_data);

    // Don't handle the event if we are configuring
    if (sdl_state->polling) {
        sdl_state->event_queue.Push(*event);
    } else {
        sdl_state->HandleGameControllerEvent(*event);
    }

    return 0;
}

class SDLJoystick {
public:
    SDLJoystick(std::string guid_, int port_, SDL_Joystick* joystick, SDL_Haptic* haptic)
        : guid{std::move(guid_)}, port{port_}, sdl_joystick{joystick, &SDL_JoystickClose},
          sdl_haptic{haptic, &SDL_HapticClose} {}

    void SetButton(int button, bool value) {
        std::lock_guard lock{mutex};
        state.buttons.insert_or_assign(button, value);
    }

    bool GetButton(int button) const {
        std::lock_guard lock{mutex};
        return state.buttons.at(button);
    }

    void SetAxis(int axis, Sint16 value) {
        std::lock_guard lock{mutex};
        state.axes.insert_or_assign(axis, value);
    }

    float GetAxis(int axis) const {
        std::lock_guard lock{mutex};
        return state.axes.at(axis) / 32767.0f;
    }

    bool RumblePlay(f32 amplitude, int time) {
        if (SDL_HapticRumbleSupported(sdl_haptic.get())) {
            return SDL_HapticRumblePlay(sdl_haptic.get(), amplitude, time);
        }
    }

    std::tuple<float, float> GetAnalog(int axis_x, int axis_y) const {
        float x = GetAxis(axis_x);
        float y = GetAxis(axis_y);
        y = -y; // 3DS uses an y-axis inverse from SDL

        // Make sure the coordinates are in the unit circle,
        // otherwise normalize it.
        float r = x * x + y * y;
        if (r > 1.0f) {
            r = std::sqrt(r);
            x /= r;
            y /= r;
        }

        return std::make_tuple(x, y);
    }

    void SetHat(int hat, Uint8 direction) {
        std::lock_guard lock{mutex};
        state.hats.insert_or_assign(hat, direction);
    }

    bool GetHatDirection(int hat, Uint8 direction) const {
        std::lock_guard lock{mutex};
        return (state.hats.at(hat) & direction) != 0;
    }
    /**
     * The guid of the joystick
     */
    const std::string& GetGUID() const {
        return guid;
    }

    /**
     * The number of joystick from the same type that were connected before this joystick
     */
    int GetPort() const {
        return port;
    }

    SDL_Joystick* GetSDLJoystick() const {
        return sdl_joystick.get();
    }

    SDL_Haptic* GetSDLHaptic() const {
        return sdl_haptic.get();
    }

    void SetSDLJoystick(SDL_Joystick* joystick) {
        sdl_joystick.reset(joystick);
    }

private:
    struct State {
        std::unordered_map<int, bool> buttons;
        std::unordered_map<int, Sint16> axes;
        std::unordered_map<int, Uint8> hats;
    } state;
    std::string guid;
    int port;
    std::unique_ptr<SDL_Joystick, decltype(&SDL_JoystickClose)> sdl_joystick;
    std::unique_ptr<SDL_Haptic, decltype(&SDL_HapticClose)> sdl_haptic;
    mutable std::mutex mutex;
};

std::shared_ptr<SDLJoystick> SDLState::GetSDLJoystickByGUID(const std::string& guid, int port) {
    std::lock_guard lock{joystick_map_mutex};
    const auto it = joystick_map.find(guid);
    if (it != joystick_map.end()) {
        while (it->second.size() <= static_cast<std::size_t>(port)) {
            auto joystick = std::make_shared<SDLJoystick>(guid, static_cast<int>(it->second.size()),
                                                          nullptr, nullptr);
            it->second.emplace_back(std::move(joystick));
        }
        return it->second[port];
    }
    auto joystick = std::make_shared<SDLJoystick>(guid, 0, nullptr, nullptr);
    return joystick_map[guid].emplace_back(std::move(joystick));
}

std::shared_ptr<SDLJoystick> SDLState::GetSDLJoystickBySDLID(SDL_JoystickID sdl_id) {
    auto sdl_joystick = SDL_JoystickFromInstanceID(sdl_id);
    const std::string guid = GetGUID(sdl_joystick);

    std::lock_guard lock{joystick_map_mutex};
    const auto map_it = joystick_map.find(guid);
    if (map_it != joystick_map.end()) {
        const auto vec_it =
            std::find_if(map_it->second.begin(), map_it->second.end(),
                         [&sdl_joystick](const std::shared_ptr<SDLJoystick>& joystick) {
                             return sdl_joystick == joystick->GetSDLJoystick();
                         });
        if (vec_it != map_it->second.end()) {
            // This is the common case: There is already an existing SDL_Joystick maped to a
            // SDLJoystick. return the SDLJoystick
            return *vec_it;
        }

        // Search for a SDLJoystick without a mapped SDL_Joystick...
        const auto nullptr_it = std::find_if(map_it->second.begin(), map_it->second.end(),
                                             [](const std::shared_ptr<SDLJoystick>& joystick) {
                                                 return !joystick->GetSDLJoystick();
                                             });
        if (nullptr_it != map_it->second.end()) {
            // ... and map it
            (*nullptr_it)->SetSDLJoystick(sdl_joystick);
            return *nullptr_it;
        }

        // There is no SDLJoystick without a mapped SDL_Joystick
        // Create a new SDLJoystick
        const int port = static_cast<int>(map_it->second.size());
        auto joystick = std::make_shared<SDLJoystick>(guid, port, sdl_joystick, nullptr);
        return map_it->second.emplace_back(std::move(joystick));
    }

    auto joystick = std::make_shared<SDLJoystick>(guid, 0, sdl_joystick, nullptr);
    return joystick_map[guid].emplace_back(std::move(joystick));
}

void SDLState::InitJoystick(int joystick_index) {
    SDL_Joystick* sdl_joystick = SDL_JoystickOpen(joystick_index);
    if (!sdl_joystick) {
        LOG_ERROR(Input, "Failed to open joystick {}", joystick_index);
        return;
    }

    SDL_Haptic* sdl_haptic = SDL_HapticOpenFromJoystick(sdl_joystick);
    if (!sdl_haptic) {
        LOG_INFO(Input, "Controller does not support haptics {}", joystick_index);
    } else {
        if (SDL_HapticRumbleInit(sdl_haptic) < 0) {
            LOG_INFO(Input, "Unable to initialize rumble {}", joystick_index);
        }
    }

    const std::string guid = GetGUID(sdl_joystick);
    std::lock_guard lock{joystick_map_mutex};
    if (joystick_map.find(guid) == joystick_map.end()) {
        auto joystick = std::make_shared<SDLJoystick>(guid, 0, sdl_joystick, sdl_haptic);
        joystick_map[guid].emplace_back(std::move(joystick));
        return;
    }
    auto& joystick_guid_list = joystick_map[guid];
    const auto it = std::find_if(
        joystick_guid_list.begin(), joystick_guid_list.end(),
        [](const std::shared_ptr<SDLJoystick>& joystick) { return !joystick->GetSDLJoystick(); });
    if (it != joystick_guid_list.end()) {
        (*it)->SetSDLJoystick(sdl_joystick);
        return;
    }
    const int port = static_cast<int>(joystick_guid_list.size());
    auto joystick = std::make_shared<SDLJoystick>(guid, port, sdl_joystick, sdl_haptic);
    joystick_guid_list.emplace_back(std::move(joystick));
}

void SDLState::CloseJoystick(SDL_Joystick* sdl_joystick) {
    const std::string guid = GetGUID(sdl_joystick);

    std::shared_ptr<SDLJoystick> joystick;
    {
        std::lock_guard lock{joystick_map_mutex};
        // This call to guid is safe since the joystick is guaranteed to be in the map
        const auto& joystick_guid_list = joystick_map[guid];
        const auto joystick_it =
            std::find_if(joystick_guid_list.begin(), joystick_guid_list.end(),
                         [&sdl_joystick](const std::shared_ptr<SDLJoystick>& joystick) {
                             return joystick->GetSDLJoystick() == sdl_joystick;
                         });
        joystick = *joystick_it;
    }

    // Destruct SDL_Joystick outside the lock guard because SDL can internally call the
    // event callback which locks the mutex again.
    joystick->SetSDLJoystick(nullptr);
}

void SDLState::HandleGameControllerEvent(const SDL_Event& event) {
    switch (event.type) {
    case SDL_JOYBUTTONUP: {
        if (auto joystick = GetSDLJoystickBySDLID(event.jbutton.which)) {
            joystick->SetButton(event.jbutton.button, false);
        }
        break;
    }
    case SDL_JOYBUTTONDOWN: {
        if (auto joystick = GetSDLJoystickBySDLID(event.jbutton.which)) {
            joystick->SetButton(event.jbutton.button, true);
        }
        break;
    }
    case SDL_JOYHATMOTION: {
        if (auto joystick = GetSDLJoystickBySDLID(event.jhat.which)) {
            joystick->SetHat(event.jhat.hat, event.jhat.value);
        }
        break;
    }
    case SDL_JOYAXISMOTION: {
        if (auto joystick = GetSDLJoystickBySDLID(event.jaxis.which)) {
            joystick->SetAxis(event.jaxis.axis, event.jaxis.value);
        }
        break;
    }
    case SDL_JOYDEVICEREMOVED:
        LOG_DEBUG(Input, "Controller removed with Instance_ID {}", event.jdevice.which);
        CloseJoystick(SDL_JoystickFromInstanceID(event.jdevice.which));
        break;
    case SDL_JOYDEVICEADDED:
        LOG_DEBUG(Input, "Controller connected with device index {}", event.jdevice.which);
        InitJoystick(event.jdevice.which);
        break;
    }
}

void SDLState::CloseJoysticks() {
    std::lock_guard lock{joystick_map_mutex};
    joystick_map.clear();
}

class SDLButton final : public Input::ButtonDevice {
public:
    explicit SDLButton(std::shared_ptr<SDLJoystick> joystick_, int button_)
        : joystick(std::move(joystick_)), button(button_) {}

    bool GetStatus() const override {
        return joystick->GetButton(button);
    }

    bool SetRumblePlay(f32 amplitude, int time) const override {
        return joystick->RumblePlay(amplitude, time);
    }

private:
    std::shared_ptr<SDLJoystick> joystick;
    int button;
};

class SDLDirectionButton final : public Input::ButtonDevice {
public:
    explicit SDLDirectionButton(std::shared_ptr<SDLJoystick> joystick_, int hat_, Uint8 direction_)
        : joystick(std::move(joystick_)), hat(hat_), direction(direction_) {}

    bool GetStatus() const override {
        return joystick->GetHatDirection(hat, direction);
    }

private:
    std::shared_ptr<SDLJoystick> joystick;
    int hat;
    Uint8 direction;
};

class SDLAxisButton final : public Input::ButtonDevice {
public:
    explicit SDLAxisButton(std::shared_ptr<SDLJoystick> joystick_, int axis_, float threshold_,
                           bool trigger_if_greater_)
        : joystick(std::move(joystick_)), axis(axis_), threshold(threshold_),
          trigger_if_greater(trigger_if_greater_) {}

    bool GetStatus() const override {
        const float axis_value = joystick->GetAxis(axis);
        if (trigger_if_greater) {
            return axis_value > threshold;
        }
        return axis_value < threshold;
    }

private:
    std::shared_ptr<SDLJoystick> joystick;
    int axis;
    float threshold;
    bool trigger_if_greater;
};

class SDLAnalog final : public Input::AnalogDevice {
public:
    SDLAnalog(std::shared_ptr<SDLJoystick> joystick_, int axis_x_, int axis_y_, float deadzone_)
        : joystick(std::move(joystick_)), axis_x(axis_x_), axis_y(axis_y_), deadzone(deadzone_) {}

    std::tuple<float, float> GetStatus() const override {
        const auto [x, y] = joystick->GetAnalog(axis_x, axis_y);
        const float r = std::sqrt((x * x) + (y * y));
        if (r > deadzone) {
            return std::make_tuple(x / r * (r - deadzone) / (1 - deadzone),
                                   y / r * (r - deadzone) / (1 - deadzone));
        }
        return std::make_tuple<float, float>(0.0f, 0.0f);
    }

    bool GetAnalogDirectionStatus(Input::AnalogDirection direction) const override {
        const auto [x, y] = GetStatus();
        const float directional_deadzone = 0.4f;
        switch (direction) {
        case Input::AnalogDirection::RIGHT:
            return x > directional_deadzone;
        case Input::AnalogDirection::LEFT:
            return x < -directional_deadzone;
        case Input::AnalogDirection::UP:
            return y > directional_deadzone;
        case Input::AnalogDirection::DOWN:
            return y < -directional_deadzone;
        }
        return false;
    }

private:
    std::shared_ptr<SDLJoystick> joystick;
    const int axis_x;
    const int axis_y;
    const float deadzone;
};

/// A button device factory that creates button devices from SDL joystick
class SDLButtonFactory final : public Input::Factory<Input::ButtonDevice> {
public:
    explicit SDLButtonFactory(SDLState& state_) : state(state_) {}

    /**
     * Creates a button device from a joystick button
     * @param params contains parameters for creating the device:
     *     - "guid": the guid of the joystick to bind
     *     - "port": the nth joystick of the same type to bind
     *     - "button"(optional): the index of the button to bind
     *     - "hat"(optional): the index of the hat to bind as direction buttons
     *     - "axis"(optional): the index of the axis to bind
     *     - "direction"(only used for hat): the direction name of the hat to bind. Can be "up",
     *         "down", "left" or "right"
     *     - "threshold"(only used for axis): a float value in (-1.0, 1.0) which the button is
     *         triggered if the axis value crosses
     *     - "direction"(only used for axis): "+" means the button is triggered when the axis
     * value is greater than the threshold; "-" means the button is triggered when the axis
     * value is smaller than the threshold
     */
    std::unique_ptr<Input::ButtonDevice> Create(const Common::ParamPackage& params) override {
        const std::string guid = params.Get("guid", "0");
        const int port = params.Get("port", 0);

        auto joystick = state.GetSDLJoystickByGUID(guid, port);

        if (params.Has("hat")) {
            const int hat = params.Get("hat", 0);
            const std::string direction_name = params.Get("direction", "");
            Uint8 direction;
            if (direction_name == "up") {
                direction = SDL_HAT_UP;
            } else if (direction_name == "down") {
                direction = SDL_HAT_DOWN;
            } else if (direction_name == "left") {
                direction = SDL_HAT_LEFT;
            } else if (direction_name == "right") {
                direction = SDL_HAT_RIGHT;
            } else {
                direction = 0;
            }
            // This is necessary so accessing GetHat with hat won't crash
            joystick->SetHat(hat, SDL_HAT_CENTERED);
            return std::make_unique<SDLDirectionButton>(joystick, hat, direction);
        }

        if (params.Has("axis")) {
            const int axis = params.Get("axis", 0);
            const float threshold = params.Get("threshold", 0.5f);
            const std::string direction_name = params.Get("direction", "");
            bool trigger_if_greater;
            if (direction_name == "+") {
                trigger_if_greater = true;
            } else if (direction_name == "-") {
                trigger_if_greater = false;
            } else {
                trigger_if_greater = true;
                LOG_ERROR(Input, "Unknown direction {}", direction_name);
            }
            // This is necessary so accessing GetAxis with axis won't crash
            joystick->SetAxis(axis, 0);
            return std::make_unique<SDLAxisButton>(joystick, axis, threshold, trigger_if_greater);
        }

        const int button = params.Get("button", 0);
        // This is necessary so accessing GetButton with button won't crash
        joystick->SetButton(button, false);
        return std::make_unique<SDLButton>(joystick, button);
    }

private:
    SDLState& state;
};

/// An analog device factory that creates analog devices from SDL joystick
class SDLAnalogFactory final : public Input::Factory<Input::AnalogDevice> {
public:
    explicit SDLAnalogFactory(SDLState& state_) : state(state_) {}
    /**
     * Creates analog device from joystick axes
     * @param params contains parameters for creating the device:
     *     - "guid": the guid of the joystick to bind
     *     - "port": the nth joystick of the same type
     *     - "axis_x": the index of the axis to be bind as x-axis
     *     - "axis_y": the index of the axis to be bind as y-axis
     */
    std::unique_ptr<Input::AnalogDevice> Create(const Common::ParamPackage& params) override {
        const std::string guid = params.Get("guid", "0");
        const int port = params.Get("port", 0);
        const int axis_x = params.Get("axis_x", 0);
        const int axis_y = params.Get("axis_y", 1);
        const float deadzone = std::clamp(params.Get("deadzone", 0.0f), 0.0f, .99f);

        auto joystick = state.GetSDLJoystickByGUID(guid, port);

        // This is necessary so accessing GetAxis with axis_x and axis_y won't crash
        joystick->SetAxis(axis_x, 0);
        joystick->SetAxis(axis_y, 0);
        return std::make_unique<SDLAnalog>(joystick, axis_x, axis_y, deadzone);
    }

private:
    SDLState& state;
};

SDLState::SDLState() {
    using namespace Input;
    RegisterFactory<ButtonDevice>("sdl", std::make_shared<SDLButtonFactory>(*this));
    RegisterFactory<AnalogDevice>("sdl", std::make_shared<SDLAnalogFactory>(*this));

    // If the frontend is going to manage the event loop, then we dont start one here
    start_thread = !SDL_WasInit(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);
    if (start_thread && SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC) < 0) {
        LOG_CRITICAL(Input, "SDL_Init(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC) failed with: {}",
                     SDL_GetError());
        return;
    }
    if (SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1") == SDL_FALSE) {
        LOG_ERROR(Input, "Failed to set hint for background events with: {}", SDL_GetError());
    }

    SDL_AddEventWatch(&SDLEventWatcher, this);

    initialized = true;
    if (start_thread) {
        poll_thread = std::thread([this] {
            using namespace std::chrono_literals;
            while (initialized) {
                SDL_PumpEvents();
                std::this_thread::sleep_for(10ms);
            }
        });
    }
    // Because the events for joystick connection happens before we have our event watcher added, we
    // can just open all the joysticks right here
    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        InitJoystick(i);
    }
}

SDLState::~SDLState() {
    using namespace Input;
    UnregisterFactory<ButtonDevice>("sdl");
    UnregisterFactory<AnalogDevice>("sdl");

    CloseJoysticks();
    SDL_DelEventWatch(&SDLEventWatcher, this);

    initialized = false;
    if (start_thread) {
        poll_thread.join();
        SDL_QuitSubSystem(SDL_INIT_JOYSTICK | SDL_INIT_HAPTIC);
    }
}

static Common::ParamPackage SDLEventToButtonParamPackage(SDLState& state, const SDL_Event& event) {
    Common::ParamPackage params({{"engine", "sdl"}});

    switch (event.type) {
    case SDL_JOYAXISMOTION: {
        const auto joystick = state.GetSDLJoystickBySDLID(event.jaxis.which);
        params.Set("port", joystick->GetPort());
        params.Set("guid", joystick->GetGUID());
        params.Set("axis", event.jaxis.axis);
        if (event.jaxis.value > 0) {
            params.Set("direction", "+");
            params.Set("threshold", "0.5");
        } else {
            params.Set("direction", "-");
            params.Set("threshold", "-0.5");
        }
        break;
    }
    case SDL_JOYBUTTONUP: {
        const auto joystick = state.GetSDLJoystickBySDLID(event.jbutton.which);
        params.Set("port", joystick->GetPort());
        params.Set("guid", joystick->GetGUID());
        params.Set("button", event.jbutton.button);
        break;
    }
    case SDL_JOYHATMOTION: {
        const auto joystick = state.GetSDLJoystickBySDLID(event.jhat.which);
        params.Set("port", joystick->GetPort());
        params.Set("guid", joystick->GetGUID());
        params.Set("hat", event.jhat.hat);
        switch (event.jhat.value) {
        case SDL_HAT_UP:
            params.Set("direction", "up");
            break;
        case SDL_HAT_DOWN:
            params.Set("direction", "down");
            break;
        case SDL_HAT_LEFT:
            params.Set("direction", "left");
            break;
        case SDL_HAT_RIGHT:
            params.Set("direction", "right");
            break;
        default:
            return {};
        }
        break;
    }
    }
    return params;
}

namespace Polling {

class SDLPoller : public InputCommon::Polling::DevicePoller {
public:
    explicit SDLPoller(SDLState& state_) : state(state_) {}

    void Start() override {
        state.event_queue.Clear();
        state.polling = true;
    }

    void Stop() override {
        state.polling = false;
    }

protected:
    SDLState& state;
};

class SDLButtonPoller final : public SDLPoller {
public:
    explicit SDLButtonPoller(SDLState& state_) : SDLPoller(state_) {}

    Common::ParamPackage GetNextInput() override {
        SDL_Event event;
        while (state.event_queue.Pop(event)) {
            switch (event.type) {
            case SDL_JOYAXISMOTION:
                if (std::abs(event.jaxis.value / 32767.0) < 0.5) {
                    break;
                }
                [[fallthrough]];
            case SDL_JOYBUTTONUP:
            case SDL_JOYHATMOTION:
                return SDLEventToButtonParamPackage(state, event);
            }
        }
        return {};
    }
};

class SDLAnalogPoller final : public SDLPoller {
public:
    explicit SDLAnalogPoller(SDLState& state_) : SDLPoller(state_) {}

    void Start() override {
        SDLPoller::Start();

        // Reset stored axes
        analog_x_axis = -1;
        analog_y_axis = -1;
        analog_axes_joystick = -1;
    }

    Common::ParamPackage GetNextInput() override {
        SDL_Event event;
        while (state.event_queue.Pop(event)) {
            if (event.type != SDL_JOYAXISMOTION || std::abs(event.jaxis.value / 32767.0) < 0.5) {
                continue;
            }
            // An analog device needs two axes, so we need to store the axis for later and wait for
            // a second SDL event. The axes also must be from the same joystick.
            const int axis = event.jaxis.axis;
            if (analog_x_axis == -1) {
                analog_x_axis = axis;
                analog_axes_joystick = event.jaxis.which;
            } else if (analog_y_axis == -1 && analog_x_axis != axis &&
                       analog_axes_joystick == event.jaxis.which) {
                analog_y_axis = axis;
            }
        }
        Common::ParamPackage params;
        if (analog_x_axis != -1 && analog_y_axis != -1) {
            const auto joystick = state.GetSDLJoystickBySDLID(event.jaxis.which);
            params.Set("engine", "sdl");
            params.Set("port", joystick->GetPort());
            params.Set("guid", joystick->GetGUID());
            params.Set("axis_x", analog_x_axis);
            params.Set("axis_y", analog_y_axis);
            analog_x_axis = -1;
            analog_y_axis = -1;
            analog_axes_joystick = -1;
            return params;
        }
        return params;
    }

private:
    int analog_x_axis = -1;
    int analog_y_axis = -1;
    SDL_JoystickID analog_axes_joystick = -1;
};
} // namespace Polling

SDLState::Pollers SDLState::GetPollers(InputCommon::Polling::DeviceType type) {
    Pollers pollers;

    switch (type) {
    case InputCommon::Polling::DeviceType::Analog:
        pollers.emplace_back(std::make_unique<Polling::SDLAnalogPoller>(*this));
        break;
    case InputCommon::Polling::DeviceType::Button:
        pollers.emplace_back(std::make_unique<Polling::SDLButtonPoller>(*this));
        break;
    }

    return pollers;
}

} // namespace InputCommon::SDL