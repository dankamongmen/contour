/**
 * This file is part of the "libterminal" project
 *   Copyright (c) 2019 Christian Parpart <christian@parpart.family>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "AeroTerminal.h"

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cctype>

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace std::placeholders;

AeroTerminal::AeroTerminal(Config const& _config) :
    //loggingSink_{"aeroterm.log", ios::trunc},
    config_{_config},
    logger_{_config.loggingMask, &cout},
    fontManager_{},
    regularFont_{
        fontManager_.load(
            _config.fontFamily,
            static_cast<unsigned>(_config.fontSize * Window::primaryMonitorContentScale().second)
        )
    },
    window_{
        Window::Size{
            _config.terminalSize.columns * regularFont_.maxAdvance(),
            _config.terminalSize.rows * regularFont_.lineHeight()
        },
        "aeroterm",
        bind(&AeroTerminal::onKey, this, _1, _2, _3, _4),
        bind(&AeroTerminal::onChar, this, _1),
        bind(&AeroTerminal::onResize, this),
        bind(&AeroTerminal::onContentScale, this, _1, _2)
    },
    terminalView_{
        _config.terminalSize,
        window_.width(),
        window_.height(),
        regularFont_,
        _config.cursorShape,
        glm::vec4{0.9, 0.9, 0.9, 1.0}, //TODO _config.colorPalette.cursor,
        glm::vec4{0.1, 0.1, 0.1, _config.backgroundOpacity},
        _config.shell,
        glm::ortho(0.0f, static_cast<GLfloat>(window_.width()), 0.0f, static_cast<GLfloat>(window_.height())),
        bind(&AeroTerminal::onScreenUpdate, this),
        logger_
    },
    configFileChangeWatcher_{
        _config.backingFilePath,
        bind(&AeroTerminal::onConfigReload, this, _1)
    }
{
    if (!regularFont_.isFixedWidth())
        throw runtime_error{ "Regular font is not a fixed-width font." };

    if (_config.backgroundBlur)
    {
        if (!window_.enableBackgroundBlur())
            throw runtime_error{ "Could not enable background blur." };
    }

    glViewport(0, 0, window_.width(), window_.height());
}

AeroTerminal::~AeroTerminal()
{
}

int AeroTerminal::main()
{
    while (terminalView_.alive() && !glfwWindowShouldClose(window_))
    {
        bool reloadPending = configReloadPending_.load();
        if (reloadPending && atomic_compare_exchange_strong(&configReloadPending_, &reloadPending, false))
            loadConfigValues();

        if (terminalView_.shouldRender())
            render();

        glfwWaitEventsTimeout(0.5);
    }

    return EXIT_SUCCESS;
}

void AeroTerminal::render()
{
    glm::vec4 const& bg = terminalView_.defaultBackgroundColor();
    glClearColor(bg.r, bg.g, bg.b, bg.a);
    //glClearColor(0.0f, 0.0f, 0.0f, 0.5f);
    glClear(GL_COLOR_BUFFER_BIT);

    terminalView_.render();

    glfwSwapBuffers(window_);
}

void AeroTerminal::onContentScale(float _xs, float _ys)
{
    cout << fmt::format("Updated content scale to: {:.2f} by {:.2f}\n", _xs, _ys);
    // TODO: scale fontSize by factor _ys.
}

void AeroTerminal::onResize()
{
    terminalView_.resize(window_.width(), window_.height());
    terminalView_.setProjection(
        glm::ortho(
            0.0f, static_cast<GLfloat>(window_.width()),
            0.0f, static_cast<GLfloat>(window_.height())
        )
    );
    glViewport(0, 0, window_.width(), window_.height());
    render();
}

optional<terminal::Key> glfwKeyToTerminalKey(int _key)
{
    using terminal::Key;

    static auto constexpr mapping = array{
        make_pair(GLFW_KEY_ESCAPE, Key::Escape),
        make_pair(GLFW_KEY_ENTER, Key::Enter),
        make_pair(GLFW_KEY_TAB, Key::Tab),
        make_pair(GLFW_KEY_BACKSPACE, Key::Backspace),
        make_pair(GLFW_KEY_INSERT, Key::Insert),
        make_pair(GLFW_KEY_DELETE, Key::Delete),
        make_pair(GLFW_KEY_RIGHT, Key::RightArrow),
        make_pair(GLFW_KEY_LEFT, Key::LeftArrow),
        make_pair(GLFW_KEY_DOWN, Key::DownArrow),
        make_pair(GLFW_KEY_UP, Key::UpArrow),
        make_pair(GLFW_KEY_PAGE_DOWN, Key::PageDown),
        make_pair(GLFW_KEY_PAGE_UP, Key::PageUp),
        make_pair(GLFW_KEY_HOME, Key::Home),
        make_pair(GLFW_KEY_END, Key::End),
        // TODO: some of those below...
        //#define GLFW_KEY_CAPS_LOCK          280
        //#define GLFW_KEY_SCROLL_LOCK        281
        //#define GLFW_KEY_NUM_LOCK           282
        //#define GLFW_KEY_PRINT_SCREEN       283
        //#define GLFW_KEY_PAUSE              284
        make_pair(GLFW_KEY_F1, Key::F1),
        make_pair(GLFW_KEY_F2, Key::F2),
        make_pair(GLFW_KEY_F3, Key::F3),
        make_pair(GLFW_KEY_F4, Key::F4),
        make_pair(GLFW_KEY_F5, Key::F5),
        make_pair(GLFW_KEY_F6, Key::F6),
        make_pair(GLFW_KEY_F7, Key::F7),
        make_pair(GLFW_KEY_F8, Key::F8),
        make_pair(GLFW_KEY_F9, Key::F9),
        make_pair(GLFW_KEY_F10, Key::F10),
        make_pair(GLFW_KEY_F11, Key::F11),
        make_pair(GLFW_KEY_F12, Key::F12),
        // todo: F13..F25
        make_pair(GLFW_KEY_KP_0, Key::Numpad_0),
        make_pair(GLFW_KEY_KP_1, Key::Numpad_1),
        make_pair(GLFW_KEY_KP_2, Key::Numpad_2),
        make_pair(GLFW_KEY_KP_3, Key::Numpad_3),
        make_pair(GLFW_KEY_KP_4, Key::Numpad_4),
        make_pair(GLFW_KEY_KP_5, Key::Numpad_5),
        make_pair(GLFW_KEY_KP_6, Key::Numpad_6),
        make_pair(GLFW_KEY_KP_7, Key::Numpad_7),
        make_pair(GLFW_KEY_KP_8, Key::Numpad_8),
        make_pair(GLFW_KEY_KP_9, Key::Numpad_9),
        make_pair(GLFW_KEY_KP_DECIMAL, Key::Numpad_Decimal),
        make_pair(GLFW_KEY_KP_DIVIDE, Key::Numpad_Divide),
        make_pair(GLFW_KEY_KP_MULTIPLY, Key::Numpad_Multiply),
        make_pair(GLFW_KEY_KP_SUBTRACT, Key::Numpad_Subtract),
        make_pair(GLFW_KEY_KP_ADD, Key::Numpad_Add),
        make_pair(GLFW_KEY_KP_ENTER, Key::Numpad_Enter),
        make_pair(GLFW_KEY_KP_EQUAL, Key::Numpad_Equal),
        #if 0
        #define GLFW_KEY_LEFT_SHIFT         340
        #define GLFW_KEY_LEFT_CONTROL       341
        #define GLFW_KEY_LEFT_ALT           342
        #define GLFW_KEY_LEFT_SUPER         343
        #define GLFW_KEY_RIGHT_SHIFT        344
        #define GLFW_KEY_RIGHT_CONTROL      345
        #define GLFW_KEY_RIGHT_ALT          346
        #define GLFW_KEY_RIGHT_SUPER        347
        #define GLFW_KEY_MENU               348
        #endif
    };

    if (auto i = find_if(begin(mapping), end(mapping), [_key](auto const& x) { return x.first == _key; }); i != end(mapping))
        return { i->second };

    return nullopt;
}

constexpr terminal::Modifier makeModifier(int _mods)
{
    using terminal::Modifier;

    Modifier mods{};

    if (_mods & GLFW_MOD_ALT)
        mods |= Modifier::Alt;
    if (_mods & GLFW_MOD_SHIFT)
        mods |= Modifier::Shift;
    if (_mods & GLFW_MOD_CONTROL)
        mods |= Modifier::Control;
    if (_mods & GLFW_MOD_SUPER)
        mods |= Modifier::Meta;

    return mods;
}

void AeroTerminal::onKey(int _key, int _scanCode, int _action, int _mods)
{
    keyHandled_ = false;
    if (_action == GLFW_PRESS || _action == GLFW_REPEAT)
    {
        terminal::Modifier const mods = makeModifier(_mods);

        char const* keyName = glfwGetKeyName(_key, _scanCode);

        logger_.keyTrace(fmt::format(
            "key: {} {}, action:{}, mod:{:02X} ({})",
            _key,
            keyName ? keyName : "(null)",
            _action,
            static_cast<unsigned>(_mods),
            terminal::to_string(mods)));

        // Screenshot: ALT+CTRL+S
        if (_key == GLFW_KEY_S && mods == (terminal::Modifier::Control + terminal::Modifier::Alt))
        {
            auto const screenshot = terminalView_.screenshot();
            ofstream ofs{ "screenshot.vt", ios::trunc | ios::binary };
            ofs << screenshot;
            keyHandled_ = true;
        }
        else if (_key == GLFW_KEY_ENTER && mods == terminal::Modifier::Alt)
        {
            window_.toggleFullScreen();
            keyHandled_ = true;
        }
        else if (auto const key = glfwKeyToTerminalKey(_key); key.has_value())
        {
            terminalView_.send(key.value(), mods);
            keyHandled_ = true;
        }
        else if (const char* cstr = glfwGetKeyName(_key, _scanCode);
               cstr != nullptr
            && mods.some() && mods != terminal::Modifier::Shift
            && strlen(cstr) == 1
            && isalnum(*cstr))
        {
            // allow only mods + alphanumerics
            terminalView_.send(*cstr, mods);
            keyHandled_ = true;
        }
        else if (_key == GLFW_KEY_SPACE)
        {
            terminalView_.send(L' ', mods);
            keyHandled_ = true;
        }
        // else if (mods && mods != terminal::Modifier::Shift)
        //    cout << fmt::format(
        //        "key:{}, scanCode:{}, name:{} ({})",
        //        _key, _scanCode, cstr ? cstr : "(null)", terminal::to_string(mods)
        //    ) << endl;
    }
}

void AeroTerminal::onChar(char32_t _char)
{
    if (!keyHandled_)
        terminalView_.send(_char, terminal::Modifier{});

    keyHandled_ = false;
}

void AeroTerminal::onScreenUpdate()
{
    glfwPostEmptyEvent();
}

void AeroTerminal::onConfigReload(FileChangeWatcher::Event _event)
{
    configReloadPending_.store(true);
    glfwPostEmptyEvent();
}

void AeroTerminal::loadConfigValues()
{
    auto filePath = config_.backingFilePath.string();
    loadConfigFromFile(config_, filePath);

    logger_.setLogMask(config_.loggingMask);

    // TODO...
}
