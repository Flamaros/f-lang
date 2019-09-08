#pragma once

#include <string>

struct Logger;
struct VK_Renderer;
struct Window;

using real = float;

enum Application_State : uint8_t {
    running = 0x01,
};

struct Globals
{
    Logger*             logger = nullptr;
    VK_Renderer*        renderer = nullptr;
    Window*             window = nullptr;
    std::string         data_path;
    Application_State   application_state = Application_State::running;
};

extern thread_local Globals    globals;

// @TODO Voir s'il ne faut pas mettre les membres de la structure en thread_local au cas par cas
// le logger doit à priori être thread_safe
