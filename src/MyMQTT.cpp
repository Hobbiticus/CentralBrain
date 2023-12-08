#include "MyMQTT.h"

MyMQTT::MyMQTT(int bufSize)
: MQTTClient(bufSize)
{}

bool MyMQTT::connect(const char clientID[], const char username[], const char password[], bool skip)
{
    m_ClientID = clientID;
    m_Username = username;
    m_Password = password;

    return MQTTClient::connect(clientID, username, password, skip);
}

bool MyMQTT::reconnect()
{
    return MQTTClient::connect(m_ClientID.c_str(), m_Username.c_str(), m_Password.c_str());
}
