#include "switch.h"

///////////////////////////////////////////////////////////////////////////
//   CONSTRUCTOR, INIT() AND LOOP()
///////////////////////////////////////////////////////////////////////////
Switch::Switch(void)
{
    m_state = false;

    pinMode(RELAY_PIN, OUTPUT);
    pinMode(LED_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT);
}

void Switch::init(void)
{
    m_state = false;
}

bool Switch::loop(void)
{
    static bool isDown = false;

    int buttonState = digitalRead(BUTTON_PIN);

    if (buttonState == LOW && isDown == true) {
        setState(!getState());
        isDown = !isDown;
        return true;
    }
    else if (buttonState == HIGH && isDown == false) {
        isDown = !isDown;
    }

    return false;
}

///////////////////////////////////////////////////////////////////////////
//   STATE
///////////////////////////////////////////////////////////////////////////
bool Switch::getState(void)
{
    return m_state;
}

bool Switch::setState(bool p_state)
{
    // checks if the given state is different from the actual state
    if (p_state == m_state) {
        return false;
    }

    if (p_state) {
        m_state = true;
        digitalWrite(RELAY_PIN, HIGH);
        digitalWrite(LED_PIN, LOW);
    }
    else {
        m_state = false;
        digitalWrite(RELAY_PIN, LOW);
        digitalWrite(LED_PIN, HIGH);
    }

    return true;
}