#pragma once
#ifndef _SWITCH_
#define _SWITCH_

#include <ESP8266WiFi.h> // https://github.com/esp8266/Arduino
#include "config.h"

class Switch
{
public:
    Switch(void);

    void init(void);
    bool loop(void);

    bool getState(void);
    bool setState(bool p_state);

private:
    bool m_state;
};

#endif