#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>
#include <esp_event.h>
#include "sensores.h"

struct wifi_detail { char ssid[26]; char pass[26]; };
extern struct wifi_detail wifi_info;

bool wifi_connect(EventGroupHandle_t, struct wifi_detail*);
void post_function(EventGroupHandle_t wait_group, struct sensor_data sensores[SENSOR_COUNT], int* r_status_read, int* r_status_send);
esp_err_t wifi_init_from_flash();
void set_systemTime_SNTP();
#endif