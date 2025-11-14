#pragma once

#include "MQTTDevice.h"

class MQTTSensor
{
public:
    //device classes: https://www.home-assistant.io/integrations/sensor/#device-class
    MQTTSensor(MQTTDevice& device, String name, String id);

    bool Init(String devClass, String units, int expireTimeSecs = 600);
    bool PublishValue(String value);

private:
    MQTTDevice& m_Device;
    String m_Name;
    String m_ID;

    String m_ValueTopic;
};
