#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "esp_err.h"
#include "esp_spiffs.h"

class StoragePartitionManager
{
    public:
    static esp_err_t mount();
    static esp_err_t format();
    static esp_err_t unmount();
    private:
    static constexpr esp_vfs_spiffs_conf_t conf = {
            .base_path = "/" CONFIG_STORAGE_PARTITION_LABEL,
            .partition_label = CONFIG_STORAGE_PARTITION_LABEL,
            .max_files = 5,
            .format_if_mount_failed = false
    };
};

#endif // STORAGE_MANAGER_H
