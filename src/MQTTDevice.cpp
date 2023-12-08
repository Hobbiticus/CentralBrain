#include "MQTTDevice.h"

MQTTDevice::MQTTDevice(MyMQTT& client, String name, String id)
: m_Client(client), m_Name(name), m_ID(id)
{

}
