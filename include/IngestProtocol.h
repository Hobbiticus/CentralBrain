#pragma once

//in case we have more things
enum
{
    DATA_TYPE_WEATHER = 0,
};

struct IngestHeader
{
    unsigned char m_Type; //of type DATA_TYPE_
};
