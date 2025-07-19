#ifndef SDCARD_H
#define SDCARD_H

#include "internet.h"
#include "sensores.h"

bool sd_initialize_card();
bool sd_read_wifi_detail(struct wifi_detail*);
bool sd_write_wifi_detail();
void save_data(struct sensor_data[SENSOR_COUNT]);
void sd_print_content(void);
#endif