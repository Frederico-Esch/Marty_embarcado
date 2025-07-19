#include "sensores.h"
void sensor_setup_id (struct sensor_data sensores[SENSOR_COUNT]) {
    // for(int i = 0; i < SENSOR_COUNT; i++) {
    //     sensores[i].id = i + 1;
    // }
    #define X(sensorID) sensores[sensorID - 1].id = sensorID;
    SENSOR_LIST  
    #undef X
}
void dht_read(struct sensor_data sensores[SENSOR_COUNT]) {
    float temperature, humidity;

    if(dht_read_float_data(DHT_TYPE_DHT11, GPIO_NUM_4, &humidity, &temperature) == ESP_OK){
        sensores[SENSOR_HUMIDADE_DHT11 -1].data = humidity;
        sensores[SENSOR_TEMPERATURA_DHT11 -1].data = temperature;
        printf("Temperatura: %f\n", temperature);
    }
}
