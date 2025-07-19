#include <communication.h>
#include <stdio.h>
#include <esp_log.h>
#include <rom/uart.h>
#include <internet.h>
#include "driver/uart.h"
#include <string.h>
#include "rom/uart.h"
#include <sdcard.h>

#pragma region CMDS

#define LIST_CMDS \ 
    X(ACK) \
    X(DUMP_WIFI) \
    X(REDEFINE_WIFI) \
    X(TRANSFER_FILES)
#define X(command) CMD_##command, 

enum CMDS {
    CMD_NULL = 0,
    LIST_CMDS
    CMD_LAST
};
#define CMD_COUNT (CMD_LAST - 1)
#undef X

enum CMDS_CHAR {
    CMD_CHAR_ACK = '\5',
    CMD_CHAR_REDEFINE_WIFI = '\4',
    CMD_CHAR_DUMP_WIFI = '\3',
    CMD_CHAR_TRANSFER_FILES = '\6'
};

#pragma endregion

#define X(command) case CMD_CHAR_##command: reply_to_##command(); break;

esp_err_t my_read(char *fmt, void *value) {
    char readChar = 0;
    char buffer[500] = {0};
    size_t bufferOccupied = 0;

    while (readChar != '\n' && bufferOccupied <= 498)
    {
        ETS_STATUS status = uart_rx_one_char((uint8_t *)&readChar);
        if (status == ETS_OK)
        {
            buffer[bufferOccupied++] = readChar;
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    if (bufferOccupied >= 498)
    {
        ESP_LOGE("READ", "BUFFER OVERFLOW");
        return 1;
    }
    buffer[bufferOccupied++] = '\n';
    buffer[bufferOccupied] = 0;

    if (sscanf(buffer, fmt, value) != 1)
    {
        ESP_LOGE("READ", "SCANF");
        return 1;
    }
    return ESP_OK;
}

esp_err_t my_reply(char cmd) {
    printf("REPLY: %d\n", cmd);
    switch (cmd)
    {
        LIST_CMDS
    default:
        return 1;
    }
    return ESP_OK;
}

void reply_error() { printf("%c\n", (char)-1); }

void reply_to_DUMP_WIFI() {
    ESP_LOGI("READ", "DUMP WIFI");
    printf("\2SSID: %s\n\2PASSWORD: %s\n", wifi_info.ssid, wifi_info.pass);
}

void reply_to_ACK() { printf("\2%c\n", 6); }

void reply_to_REDEFINE_WIFI() {
    ESP_LOGI("READ", "REDEFINE WIFI");

    puts("\2");
    char ssid[26];
    my_read("%25s", ssid);
    ssid[25] = 0;
    printf("%s\n", ssid);

    puts("\2");
    char pass[26];
    my_read("%25s", pass);
    pass[25] = 0;
    printf("%s\n", pass);

    memcpy(wifi_info.ssid, ssid, 25);
    memcpy(wifi_info.pass, pass, 25);
    
    if(!sd_write_wifi_detail()) {
        ESP_LOGE("READ", "SDCARD ERROR");
    }

    puts("\2s");
}

void reply_to_TRANSFER_FILES() {
    ESP_LOGI("READ", "TRANSFER FILES");

    sd_print_content();

    puts("\2");
}

void communicate() {
    static char cmd = 0;
    esp_err_t res = my_read("%c\n", &cmd);
    if (res == ESP_OK) my_reply(cmd);
}
