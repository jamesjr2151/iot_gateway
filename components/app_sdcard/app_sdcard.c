#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "esp_log.h"
#include "driver/sdspi_host.h"
#include "driver/spi_common.h"
#include "app_sdcard.h"

static const char *TAG = "APP_SDCARD";
static sdmmc_card_t *s_card = NULL;

#define MOUNT_POINT "/sdcard"

esp_err_t app_sdcard_init(void)
{
    esp_err_t ret;

    ESP_LOGI(TAG, "Initializing SD card via SPI");

    // Options for mounting the filesystem
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };

    ESP_LOGI(TAG, "Initializing SPI bus");
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_NUM_MOSI,
        .miso_io_num = SD_PIN_NUM_MISO,
        .sclk_io_num = SD_PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };

    // Initialize the SPI bus on SPI3_HOST (SPI2_HOST is often used for other peripherals)
    ret = spi_bus_initialize(SPI3_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SPI bus.");
        return ret;
    }

    // This configures the SD SPI host
    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SPI3_HOST;

    // This configures the SPI device on the SPI bus
    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_NUM_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting FAT filesystem");
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &s_card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s).", esp_err_to_name(ret));
        }
        spi_bus_free(host.slot);
        return ret;
    }

    ESP_LOGI(TAG, "Filesystem mounted successfully");

    // Print card info
    sdmmc_card_print_info(stdout, s_card);

    return ESP_OK;
}

esp_err_t app_sdcard_deinit(void)
{
    if (s_card == NULL) {
        return ESP_ERR_INVALID_STATE;
    }

    // Unmount partition and disable SPI peripheral
    esp_vfs_fat_sdcard_unmount(MOUNT_POINT, s_card);
    ESP_LOGI(TAG, "Card unmounted");

    // Deinitialize the SPI bus
    spi_bus_free(SPI3_HOST);
    
    s_card = NULL;
    return ESP_OK;
}
