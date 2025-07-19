#ifndef SENSORES_H
#define SENSORES_H

#include <errno.h>
#include <dht.h>
struct sensor_data { 
    int id;
    double data;
};

#define SENSOR_LIST \
        X(SENSOR_HUMIDADE_DHT11) \
        X(SENSOR_TEMPERATURA_DHT11)

#define X(name) name,
enum SENSORS {
    SENSOR_NULL = 0,
    SENSOR_LIST
    SENSOR_END
};
#undef X

#define SENSOR_COUNT (SENSOR_END - 1)
void sensor_setup_id (struct sensor_data sensores[SENSOR_COUNT]);
void dht_read(struct sensor_data sensores[SENSOR_COUNT]);
#endif