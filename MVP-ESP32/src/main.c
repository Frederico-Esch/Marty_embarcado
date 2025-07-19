#include <esp_netif.h>
#include <esp_event.h>
#include <freertos/event_groups.h>
#include <esp_wifi.h>
#include <esp_err.h>
#include <nvs_flash.h>
#include <esp_log.h>
#include <sdmmc_cmd.h>
#include <esp_vfs.h>
#include <esp_vfs_fat.h>
#include <errno.h>
#include <dht.h>
#include <driver/gpio.h>
#include <esp_http_client.h>
#include <unistd.h>
#include "internet.h"
#include "sensores.h"
#include "sdcard.h"
#include "communication.h"

#define LED_WIFI GPIO_NUM_27
#define LED_SD GPIO_NUM_25
#define BUTTON_COM GPIO_NUM_12
#define LED_COMMUNICATION GPIO_NUM_26
#define LED_SENSOR GPIO_NUM_14


bool is_wifi_connected = false;
static EventGroupHandle_t wait_group = NULL;
struct sensor_data sensores[SENSOR_COUNT];

struct wifi_detail wifi_info;

void communication_routine(){
    ESP_LOGI("MAIN ROUTINE", "COMMUNICATION");
    for(;;){
        ESP_LOGI("MAIN ROUTINE", "ALIVE");
        communicate();
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

void sensor_routine() {
    ESP_LOGI("MAIN ROUTINE", "SENSOR");

    wait_group = xEventGroupCreate();

    if (strlen(wifi_info.ssid) && strlen(wifi_info.pass)) {
        ESP_LOGI("wifi","%s\n%s\n", wifi_info.ssid, wifi_info.pass);
        gpio_set_level(LED_SD, 1);

        is_wifi_connected = wifi_connect(wait_group, &wifi_info);
        if (is_wifi_connected) gpio_set_level(LED_WIFI, 1);
    }

    ESP_LOGI("wifi", "is connected %d\n", is_wifi_connected);
    while(true) {
        dht_read(sensores);
        int status_read, status_send;
        post_function(wait_group, sensores, &status_read, &status_send);
        printf("%d %d\n", status_read, status_send);
        vTaskDelay(1000*600 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    sensor_setup_id(sensores);
    wifi_init_from_flash();
    
    gpio_set_direction(BUTTON_COM, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BUTTON_COM, GPIO_PULLUP_ONLY);

    gpio_set_pull_mode(GPIO_NUM_4, GPIO_PULLUP_ONLY);

    gpio_set_direction(LED_WIFI,GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_SD,GPIO_MODE_OUTPUT);
    gpio_set_level(LED_WIFI,0);
    gpio_set_level(LED_SD,0);

    gpio_set_direction(LED_SENSOR, GPIO_MODE_OUTPUT);
    gpio_set_direction(LED_COMMUNICATION, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_SENSOR,0);
    gpio_set_level(LED_COMMUNICATION,0);
    

    
    vTaskDelay(500 / portTICK_PERIOD_MS);

    sd_initialize_card();

    memset(&wifi_info, 0, sizeof(wifi_info));
    bool has_wifi_info = sd_read_wifi_detail(&wifi_info);

    if (!has_wifi_info) {
        memset(wifi_info.ssid, 0, sizeof(wifi_info.ssid));
        memset(wifi_info.pass, 0, sizeof(wifi_info.pass));
    }

    if (gpio_get_level(BUTTON_COM)) {
        gpio_set_level(LED_SENSOR, 1);
        sensor_routine();
    }
    else {
        gpio_set_level(LED_COMMUNICATION, 1);
        communication_routine();
    }
}
