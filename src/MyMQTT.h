#pragma once

#include "MQTT.h"

class MyMQTT : public MQTTClient
{
public:
    //overrides
    explicit MyMQTT(int bufSize = 128);
    bool connect(const char clientID[], const char username[], const char password[], bool skip = false);

    //new!
    bool reconnect();
private:
    String m_ClientID;
    String m_Username;
    String m_Password;
};
