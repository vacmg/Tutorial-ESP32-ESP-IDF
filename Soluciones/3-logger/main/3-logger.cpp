#include "esp_log.h"
#include "storage-manager.h"

const char* MAIN_TAG = "MAIN";

extern "C" void app_main(void)
{
    StoragePartitionManager::mount();

//    FILE* file = fopen("/storage/readme.txt", "r");
//    if (file == nullptr)
//    {
//        ESP_LOGE(MAIN_TAG, "Failed to open file for reading");
//    }
//    else
//    {
//        char line[1024];
//        while (fgets(line, sizeof(line), file))
//        {
//            printf("%s", line);
//        }
//        fclose(file);
//    }

    int bootNumber = 0;
    FILE* logFile = nullptr;
    FILE* bootNumberFile = fopen("/storage/bootNumber.txt", "r");
    if (bootNumberFile != nullptr)
    {
        fscanf(bootNumberFile, "%d", &bootNumber);
        fclose(bootNumberFile);
        ESP_LOGI(MAIN_TAG, "Last boot number: %d", bootNumber);

    }
    else
    {
        ESP_LOGW(MAIN_TAG, "Failed to open /storage/bootNumber.txt for reading: creating a new file");
        bootNumber = 0;
    }

    bootNumberFile = fopen("/storage/bootNumber.txt", "w");
    if (bootNumberFile == nullptr)
    {
        ESP_LOGE(MAIN_TAG, "Failed to open /storage/bootNumber.txt for writing");
    }
    else
    {
        bootNumber++;
        fprintf(bootNumberFile, "%d", bootNumber);
        fclose(bootNumberFile);

        logFile = fopen("/storage/log.txt", "a");
        if (logFile == nullptr)
        {
            ESP_LOGE(MAIN_TAG, "Failed to open /storage/log.txt for appending");
        }
        else
        {
            fprintf(logFile, "Inicio nº %d\n", bootNumber);
            fclose(logFile);
        }
    }

    logFile = fopen("/storage/log.txt", "r");
    if (logFile == nullptr)
    {
        ESP_LOGE(MAIN_TAG, "Failed to open /storage/log.txt for reading");
    }
    else
    {
        char line[1024];
        while (fgets(line, sizeof(line), logFile))
        {
            printf("%s", line);
        }
        fclose(logFile);
    }

    StoragePartitionManager::unmount();
}
