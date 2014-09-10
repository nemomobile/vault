/**
 * @file debug.cpp
 * @brief Debug tracing support
 * @author Denis Zalevskiy <denis.zalevskiy@jolla.com>
 * @copyright (C) 2014 Jolla Ltd.
 * @par License: LGPL 2.1 http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html
 */

#include <qtaround/debug.hpp>
#include <mutex>
#include <string>
#include <iostream>

namespace debug {

namespace {

int current_level = static_cast<int>(Level::Error);
std::mutex mutex;
bool is_init = false;

void init()
{
    if (!is_init) {
        std::lock_guard<std::mutex> l(mutex);
        if (is_init)
            return;
        auto c = ::getenv("CUTES_DEBUG");
        if (c) {
            std::string name(c);
            current_level = name.size() ? std::stoi(name) : (int)Level::Critical;
        }
        is_init = true;
    }
}

}

QDebug stream()
{
    init();
    return qDebug();
}

void level(Level level)
{
    init();
    current_level = static_cast<int>(level);
}

bool is_tracing_level(Level level)
{
    init();
    auto l = static_cast<int>(level);
    return current_level ? l >= current_level : false;
}

}
