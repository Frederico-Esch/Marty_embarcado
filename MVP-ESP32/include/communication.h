#ifndef COMMUNICATION_H
#define COMMUNICATION_H
#include <esp_task.h>

esp_err_t my_read(char* fmt, void* value);
esp_err_t my_reply(char cmd);
void communicate(void);

void reply_error(void);
void reply_to_DUMP_WIFI(void);
void reply_to_REDEFINE_WIFI(void);
void reply_to_ACK(void);
void reply_to_TRANSFER_FILES(void);
#endif