#include <sdmmc_cmd.h>
#include <esp_vfs.h>
#include <esp_vfs_fat.h>
#include "internet.h"
#include "sensores.h"
#include "sdcard.h"
#include <dirent.h>
#include <stdio.h>

sdmmc_card_t* sdcard = NULL;
bool card_initialized = false;

bool sd_initialize_card() {
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
        .format_if_mount_failed = false,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_config = {
        .mosi_io_num = 23,
        .miso_io_num = 19,
        .sclk_io_num = 18,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000
    };


    host.max_freq_khz = 400;
    card_initialized = spi_bus_initialize(host.slot, &bus_config, SDSPI_DEFAULT_DMA) == ESP_OK;
    if (card_initialized) ESP_LOGI("sdcard initialize","spi bus ok");
    else {
        ESP_LOGE("sdcard initialize", "spi bus error");
        return false;
    }
    
    sdspi_device_config_t device_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    device_config.gpio_cs = 5;
    device_config.host_id = host.slot;

    card_initialized = esp_vfs_fat_sdspi_mount ("/sdcard", &host, &device_config, &mount_config, &sdcard) == ESP_OK;
    if(card_initialized) ESP_LOGI("sdcard read", "mount ok");
    else {
        ESP_LOGE("sdcard read", "mount error");
        return false;
    }

    return true;
}

bool sd_read_wifi_detail(struct wifi_detail *wifi_info) {
    if (!card_initialized) {
        ESP_LOGE("sdcard read", "card not initialized");
        return false;
    }

    FILE* file = fopen("/sdcard/wifi_config.txt", "r");
    if (file == NULL){
        fclose(file);
        return false;
    }
    // ssize_t getline(char **restrict lineptr, size_t *restrict n, FILE *restrict stream);
    bool read_correctly = true;
    
    //reading ssid
    int c = 0;
    int char_count = -1;
    while((c = fgetc(file), c != '\n' && char_count + 1 < 25)) {
        wifi_info->ssid[++char_count] = c;
    }
    wifi_info->ssid[char_count+1] = 0;
    if (char_count == 24 && c != '\n') {
        ESP_LOGE("sdcard read", "SSID too big");
        read_correctly = false;
        goto FINISH;
    }

    //reading pass
    c = 0;
    char_count = -1;
    while((c = fgetc(file), c != '\n' && c != EOF && char_count + 1 < 25)) {
        wifi_info->pass[++char_count] = c;
    }
    wifi_info->pass[char_count+1] = 0;
    if (char_count == 24 && c != '\n' && c != EOF) {
        ESP_LOGE("sdcard read", "PASS too big");
        read_correctly = false;
        goto FINISH;
    }
    
FINISH:
    fclose(file);
    return read_correctly;
}

bool sd_write_wifi_detail(){
    if(!card_initialized) {
        ESP_LOGE("sdcard write", "card not initialized");
        return false;
    }

    wifi_info.ssid[25] = 0;
    wifi_info.pass[25] = 0;

    FILE* file = fopen("/sdcard/wifi_config.txt", "w+");
    if (file == NULL){
        fclose(file);
        return false;
    }
    if (fprintf(file, "%s\n%s", wifi_info.ssid, wifi_info.pass) <= 0) {
        fclose(file);
        return false;
    } 
    fclose(file);

    return true;
}

void save_data(struct sensor_data sensores[SENSOR_COUNT]) {
    char file_name[20] = {0};
    int file_name_num ;
    do{ 
        file_name_num = rand();
        sprintf(file_name,"/sdcard/%u", file_name_num); 
    } while(access(file_name, F_OK) == 0);
    FILE* file = fopen(file_name, "w");
    if(file != NULL) {
       //for(int i; i < SENSOR_COUNT; i++){ 
       //fprintf(file, "%d, %lf\n", sensores[i].id, sensores[i].data);
       //}
       #define X(name) fprintf(file, #name", %d, %lf\n", sensores[name -1 ].id, sensores[name - 1].data);
       SENSOR_LIST
       #undef X
       fclose(file);
    }

}      

static void sd_print_file_content(char* path) {
        FILE* file = fopen(path, "r");
        if(file == NULL){
            ESP_LOGE("SD", "ERROR OPENING FILE");
            fclose(file);
            return;
        }

        int c = 0;
        printf("\t");
        while((c = fgetc(file)) != EOF) {
            putchar(c);
            if (c == '\n') printf("\t");
        }

        fclose(file);
}

void sd_print_content(void) {
    DIR *dir = opendir("/sdcard");
    if(dir == NULL) {
        puts("COULDN'T OPEN DIR");
    }

    struct dirent* content;
    while((content = readdir(dir)) != NULL) {
        printf("%s:\n", content->d_name);

        char path[270] = "/sdcard/\0";
        strcat(path, content->d_name);
        sd_print_file_content(path);
    }

    closedir(dir);
}