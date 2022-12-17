//
// Created by baifeng on 2022/12/17.
//

#ifndef SDL2_TEST_COMMON_H
#define SDL2_TEST_COMMON_H

#include <string>
#include <SDL.h>
#include <SDL_ttf.h>
#include "sdl2.h"
#include "pystring/pystring.h"

#if defined(__vita__)
#include <psp2/net/net.h>
#include <psp2/sysmodule.h>

static char initparam_buf[0x4000];
static SceNetInitParam initparam;
#endif

class Data {
public:
    Data(): _data(0), _size(0) {}
    Data(uint32_t size): _data(0), _size(0) {
        resize(size);
    }
    ~Data() {
        destroy();
    }
    bool read(std::string const& file) {
        FILE *fp = fopen(file.c_str(), "rb");
        if (fp == 0) {
            printf("%s is not exists.\n", file.c_str());
            return false;
        }
        destroy();
        fseek(fp, 0, SEEK_END);
        resize(ftell(fp));
        fseek(fp, 0, SEEK_SET);
        fread(_data, _size, 1, fp);
        fclose(fp);
        return true;
    }
    bool write(std::string const& file) {
        FILE *fp = fopen(file.c_str(), "wb");
        if (fp == 0) {
            printf("write %s fail.\n", file.c_str());
            return false;
        }
        if (_data != nullptr) {
            fwrite(_data, _size, 1, fp);
        }
        fclose(fp);
        return true;
    }
    void clear() {
        destroy();
    }
    void resize(uint32_t size) {
        this->destroy();
        this->_size = size;
        this->_data = (unsigned char*)malloc(size);
    }
    unsigned char* buffer() const {
        return _data;
    }
    uint32_t size() const {
        return this->_size;
    }
private:
    void destroy() {
        if (_data) {
            free(_data);
            _data = 0;
        }
        _size = 0;
    }
private:
    unsigned char* _data;
    uint32_t _size;
};

typedef struct {
    uint32_t magic;
    uint32_t IPv4;
    uint32_t flags;
    uint16_t port;
} NetLoggingMgrConfig_t;

typedef struct {
    bool skprx_installed;
    char ipv4[48];
    char port[16];
    bool qaf;
} user_data_t;

NetLoggingMgrConfig_t net_logging_config;
user_data_t user_data;

void init_config() {
    memset(&net_logging_config, 0, sizeof(net_logging_config));
    memset(&user_data, 0, sizeof(user_data));

#if defined(__vita__)
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);
    initparam.memory = &initparam_buf;
	initparam.size = sizeof(initparam_buf);
	initparam.flags = 0;
	sceNetInit(&initparam);
#endif
}

void save_config();
void load_config() {

    std::string config_path = "ur0:data/NetLoggingMgrConfig";

#if defined(TARGET_OS_OSX)
    config_path = "./config";
#endif

    Data d;
    if (d.read(config_path + ".ud") && d.size() == sizeof(user_data)) {
        memcpy(&user_data, d.buffer(), d.size());
    } else {
        user_data.skprx_installed = false;
        user_data.qaf = true;
        strcpy(user_data.ipv4, "192.168.1.1");
        strcpy(user_data.port, "8080");
    }
}

void save_config() {
    const char magic[4] = {'N', 'L', 'M', '\0'};
    memcpy(&net_logging_config.magic, magic, 4);

    std::string config_path = "ur0:data/NetLoggingMgrConfig";

#if defined(TARGET_OS_OSX)
    config_path = "./config";
#endif

    {
        Data d(sizeof(user_data_t));
        memcpy(d.buffer(), &user_data, sizeof(user_data));
        d.write(config_path + ".ud");
    }

    {
        printf("save config begin.\n");
        net_logging_config.flags = user_data.qaf ? 1 : 0;
        net_logging_config.port = atoi(user_data.port);
#if defined(__vita__)
        auto res = sceNetInetPton(SCE_NET_AF_INET, user_data.ipv4, &net_logging_config.IPv4);
        if(res != 1){
            printf("Error : Invalid IPv4.\n");
        }
#endif
        printf("save config end.\n");
    }

    {
        Data d(sizeof(NetLoggingMgrConfig_t));
        memcpy(d.buffer(), &net_logging_config, sizeof(net_logging_config));
        d.write(config_path + ".bin");
    }
}

SDL_Texture* CreateTextTexture(SDL_Renderer* renderer, TTF_Font* font, char const* text, SDL_Color const& c) {
    auto surface = TTF_RenderUTF8_Solid(font, text, c);
    if (surface == nullptr) {
        return nullptr;
    }
    auto texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

void save_lines_to_data(Data& d, std::vector<std::string> const& lines) {
    std::string ret;
    for (int i = 0; i < lines.size(); ++i) {
        auto const& str = lines[i];
        ret += str + "\n";
    }
    d.resize(ret.size());
    memcpy(d.buffer(), ret.data(), ret.size());
}

bool setup_skprx_plugins() {

    std::string plugins_path = "ur0:tai/net_logging_mgr.skprx";
    std::string tai_config_path = "ur0:/tai/config.txt";

#if defined(TARGET_OS_OSX)
    // for test
    plugins_path = "./net_logging_mgr.skprx";
    tai_config_path = "./config.txt";
#endif

    Data d;
    if (!d.read(sdl2::res_path("assets/net_logging_mgr.skprx"))) {
        return false;
    }
    if (!d.write(plugins_path)) {
        return false;
    }
    d.clear();
    if (!d.read(tai_config_path)) {
        d.write(tai_config_path);
    }
    std::vector<std::string> lines;
    if (d.size()) {
        pystring::split(std::string((char*)d.buffer(), d.size()), lines, "\n");
    }
    for (int i = 0; i < lines.size(); ++i) {
        auto& str = lines[i];
        if (str[0] == '#' || str[0] == '*') {
            continue;
        }
        if (pystring::find(str, "net_logging_mgr.skprx") >= 0) {
            str = plugins_path;
            save_lines_to_data(d, lines);
            return d.write(tai_config_path);
        }
    }
    for (int i = 0; i < lines.size(); ++i) {
        auto& str = lines[i];
        if (pystring::find(str, "*main") == -1) {
            continue;
        }
        lines.insert(lines.begin()+i, plugins_path);
        save_lines_to_data(d, lines);
        return d.write(tai_config_path);
    }
    lines.emplace_back(plugins_path);
    save_lines_to_data(d, lines);
    return d.write(tai_config_path);
}

#endif //SDL2_TEST_COMMON_H
