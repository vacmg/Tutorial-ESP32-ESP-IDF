#include "storage-manager.h"
#include "esp_log.h"

const char* COMPONENT_TAG_STORAGE_PARTITION_MANAGER = "STORAGE_PARTITION_MANAGER";

esp_err_t StoragePartitionManager::mount()
{
    ESP_LOGI(COMPONENT_TAG_STORAGE_PARTITION_MANAGER, "Initializing '" CONFIG_STORAGE_PARTITION_LABEL "' partition");

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK)
    {
        if (ret == ESP_FAIL)
        {
            ESP_LOGE(COMPONENT_TAG_STORAGE_PARTITION_MANAGER, "Failed to mount '" CONFIG_STORAGE_PARTITION_LABEL "' filesystem");
        }
        else if (ret == ESP_ERR_NOT_FOUND)
        {
            ESP_LOGE(COMPONENT_TAG_STORAGE_PARTITION_MANAGER, "Failed to find '" CONFIG_STORAGE_PARTITION_LABEL "' SPIFFS partition");
        }
        else
        {
            ESP_LOGE(COMPONENT_TAG_STORAGE_PARTITION_MANAGER, "Failed to initialize '" CONFIG_STORAGE_PARTITION_LABEL "' SPIFFS partition: %s", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK)
    {
        ESP_LOGE(COMPONENT_TAG_STORAGE_PARTITION_MANAGER, "Failed to get '" CONFIG_STORAGE_PARTITION_LABEL "' partition information: %s", esp_err_to_name(ret));
        return ret;
    }
    else
    {
        ESP_LOGI(COMPONENT_TAG_STORAGE_PARTITION_MANAGER, "'" CONFIG_STORAGE_PARTITION_LABEL "' partition size: total: %d, used: %d", total, used);
    }

    // Check the consistency of reported partition size info.
    if (used > total)
    {
        ESP_LOGW(COMPONENT_TAG_STORAGE_PARTITION_MANAGER, "Number of used bytes cannot be larger than total.");
        return ESP_FAIL;
    }
    return ret;
}

esp_err_t StoragePartitionManager::format()
{
    ESP_LOGI(COMPONENT_TAG_STORAGE_PARTITION_MANAGER, "Formatting '" CONFIG_STORAGE_PARTITION_LABEL "' partition");
    esp_err_t ret = esp_spiffs_format(conf.partition_label);
    if (ret != ESP_OK)
    {
        ESP_LOGE(COMPONENT_TAG_STORAGE_PARTITION_MANAGER, "Failed to format '" CONFIG_STORAGE_PARTITION_LABEL "' partition: %s", esp_err_to_name(ret));
        return ret;
    }
    return ret;
}

esp_err_t StoragePartitionManager::unmount()
{
    ESP_LOGI(COMPONENT_TAG_STORAGE_PARTITION_MANAGER, "Unmounting '" CONFIG_STORAGE_PARTITION_LABEL "' partition");
    esp_err_t ret = esp_vfs_spiffs_unregister(conf.partition_label);
    if (ret != ESP_OK)
    {
        ESP_LOGE(COMPONENT_TAG_STORAGE_PARTITION_MANAGER, "Failed to unmount '" CONFIG_STORAGE_PARTITION_LABEL "' partition: %s", esp_err_to_name(ret));
    }
    return ret;
}
