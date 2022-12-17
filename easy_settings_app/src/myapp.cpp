//
// Created by baifeng on 2021/7/12.
//
#include "sdl2.h"
#include "common.h"
#include <map>
#include <vector>

#if defined(__vita__)
#include <psp2/power.h>
#endif

std::vector<SDL_Texture*> titles;
std::vector<SDL_Texture*> states;
SDL_Texture* icon = 0;
TTF_Font* font = 0;
int select_index = 0;

enum TITLE_TYPE {
    SETUP_SKPRX = 0,
    SET_SERVER_IPV4,
    SET_SERVER_PORT,
    QAF_SETTINGS,
    SAVE_CONFIG,
    REBOOT,
    EXIT,
    MAX,
};

enum INPUT_TYPE {
    NONE_INPUT_TYPE = 0,
    IPV4_INPUT,
    PORT_INPUT,
};

INPUT_TYPE input_type = NONE_INPUT_TYPE;

void set_state_text(TITLE_TYPE type, char const* text, SDL_Color const& c) {
    if (type >= states.size()) {
        printf("invalid type(%d) to set state text.\n", type);
        return;
    }
    if (states[type]) {
        SDL_DestroyTexture(states[type]);
        states[type] = nullptr;
    }
    if (strlen(text) == 0) {
        return;
    }
    states[type] = CreateTextTexture(sdl2::renderer(), font, text, c);
}

void init() {
    printf("init config begin.\n");
    init_config();
    printf("load config begin.\n");
    load_config();
    std::string texts[TITLE_TYPE::MAX] = {
            "SETUP NET_LOGGING_MGR.SKPRX",
            "SET SERVER IPV4",
            "SET SERVER PORT",
            "QAF SETTINGS",
            "SAVE CONFIG",
            "REBOOT",
            "EXIT",
    };
    printf("create text textures.\n");
    auto renderer = sdl2::renderer();
    font = TTF_OpenFont(sdl2::res_path("assets/prstart.ttf"), 20);
    if (font == nullptr) {
        printf("font is nullptr.\n");
        return;
    }
    for (int i = 0; i < TITLE_TYPE::MAX; ++i) {
        auto const& s = texts[i];
        auto texture = CreateTextTexture(renderer, font, s.c_str(), {255, 255, 255, 255});
        titles.emplace_back(texture);
    }
    icon = CreateTextTexture(renderer, font, "->", {255, 255, 255, 255});

    printf("set state text begin.\n");

    states.resize(5);
    /*user_data_t ud = {
            true,
            "192.168.1.1",
            "8080",
            "ENABLED",
    };
    memcpy(&user_data, &ud, sizeof(ud));*/
    if (user_data.skprx_installed) {
        set_state_text(TITLE_TYPE::SETUP_SKPRX, "(INSTALLED)", {255, 0, 0, 255});
    }
    if (strlen(user_data.ipv4) >= 1) {
        char str[128] = {0};
        snprintf(str, 128, "(%s)", user_data.ipv4);
        set_state_text(TITLE_TYPE::SET_SERVER_IPV4, str, {255, 0, 0, 255});
    }
    if (strlen(user_data.port) >= 1) {
        char str[128] = {0};
        snprintf(str, 128, "(%s)", user_data.port);
        set_state_text(TITLE_TYPE::SET_SERVER_PORT, str, {255, 0, 0, 255});
    }
    {
        char str[128] = {0};
        snprintf(str, 128, "(%s)", user_data.qaf ? "ENABLED" : "DISABLED");
        set_state_text(TITLE_TYPE::QAF_SETTINGS, str, {255, 0, 0, 255});
    }

    printf("set state text end.\n");
}

void fini() {
    if (font) {
        TTF_CloseFont(font);
    }
    for (int i = 0; i < titles.size(); ++i) {
        SDL_DestroyTexture(titles[i]);
    }
    titles.clear();
    for (int i = 0; i < states.size(); ++i) {
        SDL_DestroyTexture(states[i]);
    }
    states.clear();
    SDL_DestroyTexture(icon);
}

void update(float dt) {

}

void draw(SDL_Renderer* renderer) {
    SDL_Rect dest{0, 0, 0, 0};
    for (int i = 0; i < titles.size(); ++i) {
        auto texture = titles[i];
        dest.x = 80;
        dest.y = i * 40 + 60;
        SDL_QueryTexture(texture, nullptr, nullptr, &dest.w, &dest.h);
        SDL_RenderCopy(renderer, texture, nullptr, &dest);

        if (i < states.size()) {
            texture = states[i];
            if (texture == nullptr) {
                continue;
            }
            dest.x = 80 + dest.w + 30;
            SDL_QueryTexture(texture, nullptr, nullptr, &dest.w, &dest.h);
            SDL_RenderCopy(renderer, texture, nullptr, &dest);
        }
    }
    SDL_QueryTexture(icon, nullptr, nullptr, &dest.w, &dest.h);
    dest.x = 40 - dest.w * 0.5f;
    dest.y = select_index * 40 + 60;
    SDL_RenderCopy(renderer, icon, nullptr, &dest);
}

void on_keydown(int key) {
    if (key == sdl2::KeyCode::UP) {
        select_index -= 1;
        select_index = select_index <= -1 ? TITLE_TYPE::MAX - 1 : select_index;
    } else if (key == sdl2::KeyCode::DOWN) {
        select_index += 1;
        select_index = select_index >= TITLE_TYPE::MAX ? 0 : select_index;
    } else if (key == sdl2::KeyCode::A) {
        if (select_index == TITLE_TYPE::SAVE_CONFIG) {
            save_config();
            set_state_text(TITLE_TYPE::SAVE_CONFIG, "(OK)", {255, 0, 0, 255});
        } else if (select_index == TITLE_TYPE::EXIT) {
            sdl2::quit();
        } else if (select_index == TITLE_TYPE::SETUP_SKPRX) {
            if (setup_skprx_plugins()) {
                set_state_text(TITLE_TYPE::SETUP_SKPRX, "(INSTALLED)", {255, 0, 0, 255});
                user_data.skprx_installed = true;
            } else {
                set_state_text(TITLE_TYPE::SETUP_SKPRX, "(INSTALL FAIL!)", {255, 255, 0, 255});
            }
        } else if (select_index == TITLE_TYPE::REBOOT) {
#if defined(__vita__)
            scePowerRequestColdReset();
#endif
        } else if (select_index == TITLE_TYPE::QAF_SETTINGS) {
            user_data.qaf = !user_data.qaf;
            net_logging_config.flags = user_data.qaf ? 1 : 0;
            char str[32] = {0};
            snprintf(str, 32, "(%s)", user_data.qaf ? "ENABLED" : "DISABLED");
            set_state_text(TITLE_TYPE::QAF_SETTINGS, str, {255, 0, 0, 255});
        } else if (select_index == TITLE_TYPE::SET_SERVER_IPV4) {
            input_type = INPUT_TYPE::IPV4_INPUT;
            SDL_StartTextInput();
        } else if (select_index == TITLE_TYPE::SET_SERVER_PORT) {
            input_type = INPUT_TYPE::PORT_INPUT;
            SDL_StartTextInput();
        }
    }
}

void on_keyup(int key) {

}

static std::map<char, sdl2::KeyCode::Type> key_map = {
        {'w', sdl2::KeyCode::UP},
        {'s', sdl2::KeyCode::DOWN},
        {'a', sdl2::KeyCode::LEFT},
        {'d', sdl2::KeyCode::RIGHT},
        {'l', sdl2::KeyCode::A},
};

void on_event(SDL_Event const& e) {
    if (e.type == SDL_JOYBUTTONDOWN) {
        // psp按键处理
        on_keydown(sdl2::psp_key(e.jbutton.button));
    } else if (e.type == SDL_JOYBUTTONUP) {
        on_keyup(sdl2::psp_key(e.jbutton.button));
    } else if (e.type == SDL_KEYDOWN) {
        // 键盘按键处理
        on_keydown(key_map[e.key.keysym.sym]);
    } else if (e.type == SDL_KEYUP) {
        on_keyup(key_map[e.key.keysym.sym]);
    } else if (e.type == SDL_TEXTINPUT) {
        printf("keyboard input text: %s, input type = %d\n", e.text.text, input_type);
        if (input_type == INPUT_TYPE::IPV4_INPUT) {
            strcpy(user_data.ipv4, e.text.text);
            char str[128] = {0};
            snprintf(str, 128, "(%s)", user_data.ipv4);
            set_state_text(TITLE_TYPE::SET_SERVER_IPV4, str, {255, 0, 0, 255});
        } else if (input_type == INPUT_TYPE::PORT_INPUT) {
            strcpy(user_data.port, e.text.text);
            char str[128] = {0};
            snprintf(str, 128, "(%s)", user_data.port);
            set_state_text(TITLE_TYPE::SET_SERVER_PORT, str, {255, 0, 0, 255});
        }
    }
}

int sdl2_main() {
    sdl2::window_title = "PrincessLog";
    sdl2::screen_size = {960, 544};
    sdl2::user_init = &init;
    sdl2::user_fini = &fini;
    sdl2::user_update = &update;
    sdl2::user_render = &draw;
    sdl2::user_event = &on_event;
    return sdl2::run();
}