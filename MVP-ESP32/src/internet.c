#include "internet.h"
#include <esp_wifi.h>
#include <esp_log.h>
#include <esp_http_client.h>
#include <errno.h>
#include <nvs_flash.h>
#include <time.h>
#include <sys/time.h>
#include "esp_attr.h"
#include "esp_sleep.h"
#include "esp_sntp.h"
#include "ds1302.h"


#define WAIT_TIME_FOR_WIFI (40000 / portTICK_PERIOD_MS)
#define WAIT_TIME_FOR_POST (10000 / portTICK_PERIOD_MS)

#define WIFI_BIT BIT0
#define POST_BIT BIT1
static const char *TAG = "wifi station";

#define URL_CREATE_READING "http://api.marty-sta.com.br/Sensors/CreateReading"
#define URL_SEND_DATA "http://api.marty-sta.com.br/Sensors/SendData"

esp_event_handler_instance_t ip_event_instance;
esp_event_handler_instance_t ip6_event_instance;

volatile int status_read = 0;
volatile int status_send = 0;

void event_got_ip(void *event_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    EventGroupHandle_t wait_group = event_arg;
     if(event_id == IP_EVENT_GOT_IP6){
        ip_event_got_ip6_t* ipv6 = event_data;
        ESP_LOGI("IP", "IPV6" IPV6STR, IPV62STR(ipv6->ip6_info.ip));
        ESP_LOGI("INDEX", "%d", ipv6->ip_index);
        if(ipv6->ip_index > 0) xEventGroupSetBits(wait_group, WIFI_BIT);
     }
     if(event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* ipv4 = event_data;
        ESP_LOGI("IP", "IPV4");
        esp_netif_create_ip6_linklocal(ipv4->esp_netif);
     }
}

bool wifi_connect(EventGroupHandle_t wait_group, struct wifi_detail *wifi_info) {

    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, event_got_ip, wait_group, &ip_event_instance);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_GOT_IP6, event_got_ip, wait_group, &ip6_event_instance);

    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);

    //TODO: ler SSID e o PASSWORD cartao SD
    wifi_config_t wifi_config = {
        .sta = { .ssid = {}, .password = {}, }
    };
    memcpy(wifi_config.sta.ssid, wifi_info->ssid, sizeof wifi_info->ssid);
    memcpy(wifi_config.sta.password, wifi_info->pass, sizeof wifi_info->pass);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();

    EventBits_t wait_result = xEventGroupWaitBits(wait_group, BIT0, pdFALSE, pdTRUE, WAIT_TIME_FOR_WIFI);
    //esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_event_instance);
    return  (wait_result & WIFI_BIT) != 0;
}
    
struct post_callback_userdata { bool is_send; EventGroupHandle_t wait_group;};

esp_err_t post_callback(esp_http_client_event_t *evt) { 
    if (evt->event_id == HTTP_EVENT_ON_FINISH) { 
        struct post_callback_userdata* user_data  = (struct post_callback_userdata*)evt->user_data;
        xEventGroupSetBits(user_data->wait_group, POST_BIT);
        ESP_LOGI("Deu bom","HTTP_EVENT_ON_DATA" );
        if(user_data->is_send) {
            status_send = esp_http_client_get_status_code(evt->client);
        } 
        else status_read = esp_http_client_get_status_code(evt->client);
        } 
    return ESP_OK;}

void post_function(EventGroupHandle_t wait_group, struct sensor_data sensores[SENSOR_COUNT], int* r_status_read, int* r_status_send) {
    xEventGroupClearBits(wait_group, POST_BIT);
    struct post_callback_userdata user_data = {
        .is_send = false,
        .wait_group = wait_group
    };
    esp_http_client_config_t config_post = {
        .url = URL_CREATE_READING,
        .method = HTTP_METHOD_POST,
        .event_handler = post_callback,
        .user_data = &user_data
    };
    esp_http_client_handle_t client = esp_http_client_init(&config_post);
    esp_http_client_perform(client);

    EventBits_t wait_result = xEventGroupWaitBits(wait_group, POST_BIT, pdFALSE, pdTRUE, WAIT_TIME_FOR_POST);
    user_data.is_send = true;

    if ((wait_result & POST_BIT) == POST_BIT) {
        esp_http_client_set_url(client, URL_SEND_DATA);
        esp_http_client_set_method(client, HTTP_METHOD_POST);
        esp_http_client_set_header(client, "Content-Type", "application/sensor-data");
        esp_http_client_set_post_field(client, (void*)sensores, sizeof(*sensores)*SENSOR_COUNT);
        esp_http_client_perform(client);
    }
    *r_status_read = status_read;
    *r_status_send = status_send;
    esp_http_client_cleanup(client);
}

esp_err_t wifi_init_from_flash() {
     esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    esp_netif_init();
    esp_event_loop_create_default();
    return ret;
}

ds1302_t rtc = {
        .ce_pin = GPIO_NUM_15, //RST
        .io_pin = GPIO_NUM_2,  //DAT
        .sclk_pin = GPIO_NUM_21,  //CLK
        .ch =  false
};

void get_current_date_time(){
	time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    setenv("TZ", "UTC+03:00", 1);
    tzset();
    localtime_r(&now, &timeinfo);
    ds1302_set_time(&rtc, &timeinfo);
    ds1302_get_time(&rtc, &timeinfo);
    
    printf("%d:%d:%d - %d/%d/%d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, timeinfo.tm_mday, timeinfo.tm_mon + 1 , timeinfo.tm_year + 1900);
}

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

static void obtain_time(void) {

    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    };
    time(&now);
    localtime_r(&now, &timeinfo);
}

void set_systemTime_SNTP()  {

time_t now;
struct tm timeinfo;
time(&now);
localtime_r(&now, &timeinfo);
if (timeinfo.tm_year < (2016 - 1900)) {
    ESP_LOGI(TAG, "Time is not set yet. Connecting to WiFi and getting time over NTP.");
    obtain_time();
    time(&now);
}
}

