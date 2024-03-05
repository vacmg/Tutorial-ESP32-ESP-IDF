En este proyecto, vamos a crear un logger que registra un mensaje en un archivo cada vez que el ESP32 se inicia. También imprimirá cada línea presente en el archivo de logs.

1. Hay que crear un nuevo proyecto llamado logger:
    1. Se debe ejecutar `idf.py create-project logger`.
    2. Si se va a utilizar un IDE para programar el proyecto, se debe configurar CMake en el IDE (según lo visto en el punto 11 del tutorial 1 (Hello world)).

2. En este proyecto vamos a utilizar componentes (aka librerías externas) y clases (C++). Por lo tanto, antes de continuar debemos saber que el compilador averigua automáticamente el lenguaje utilizado en función de si el archivo de código termina en `.c` o `.cpp`. 
    1. Para poder utilizar clases debemos cambiar el archivo principal `main/logger.c` a `main/logger.cpp`. 
    2. Tras hacer esto, se debe reflejar el cambio de extensión del archivo de código en el archivo de configuración `main/CMakeLists.txt`: `idf_component_register(SRCS "logger.cpp" INCLUDE_DIRS ".")`.
    3. Por otro lado, el sistema operativo espera que la función `app_main()` sea una función de C, por lo que se debe modificar la cabecera de la función para indicar que se mantenga compatibilidad con C:
        ```
        extern "C" void app_main(void)
        {

        }
        ```

3. Ahora hay que preparar la tabla de particiones:
    1. Primero se debe crear el archivo que contiene las especificaciones de las particiones: `touch partitions.csv` (se debe crear este archivo en la raíz del proyecto)
    2. Se rellena el archivo con la siguiente información:
        Una partición para el programa, llamada `bin`, de tipo `app` (codigo) y subtipo `factory` (el subtipo no es necesario a menos que se planee utilizar para actualizaciones vía OTA) de 1MB de tamaño.
        Una partición de datos llamada `storage`, de tipo `data` y subtipo `spiffs` (es un sistema de archivos, como FAT o NTFS) y tamaño 512KB.
        ```
        # Name,   Type, SubType, Offset,  Size, Flags

        bin,  app,  factory, ,  1M,
        storage,  data, spiffs, , 512K,
        ```
    3. Se configura el proyecto para utilizar la tabla de particiones creada:
        1. Se ejecuta `idf.py menuconfig`.
        2. Se navega al menu `Partition Table`.
        3. Se modifica la entrada `Partition Table` a `Custom partition table CSV`.
        4. Se modifica la entrada `Custom partition CSV file` al nombre del archivo de particiones (`pasrtitions.csv`).
        5. Se guarda y se sale de la configuración pulsando `Q` para salir y luego `Y` para guardar los cambios.
    
    
4. Después vamos a crear el componente encargado de preparar el sistema de archivos y la partición de almacenamiento
    1. Se debe ejecutar en la raíz del proyecto `idf.py create-component -C components storage-manager`. Esto creará el componente `storage-manager` en la carpeta `components`.
    2. A partir de ahora vamos a trabajar en la carpeta `components/storage-manager/`
    3. Al igual que con `main/logger.c`, hay que cambiar la extensión del archivo `storage-manager.c` a `.cpp` y reflejar los cambios en su `CMakeList.txt`
    4. En el CMakeList.txt del componente, se debe modificar la entrada `idf_component_register` para incluir al final `REQUIRES spiffs`: `idf_component_register(SRCS "storage-manager.cpp" INCLUDE_DIRS "include" REQUIRES spiffs)`. Esto sirve para incluir la librería `spiffs` encargada de manejar el sistema de archivos de la partición `storage`.
    5. En el archivo `include/storage-manager.h`:
        1. Borramos el código de ejemplo y lo sustituimos por la guardia para incluir solo una vez un `.h` (que se debe utilizar cada vez que se crea un `.h`), donde `STORAGE_MANAGER_H` es un símbolo que se utiliza por convenio para este propósito (NOMBRE_DEL_ARCHIVO_H). Lo que hace es que si se realiza `#include "storage-manager.h"` por primera vez, al no estar definido el símbolo `STORAGE_MANAGER_H` se entra dentro del if y se define el símbolo junto con el resto del código del `.h`. Si luego se incluye el `.h` otra vez, al ya estar definido el símbolo de la guardia, no se entra dentro del if y no se duplica el código:
            ```
            #ifndef STORAGE_MANAGER_H
            #define STORAGE_MANAGER_H

            // Aquí escribiremos el codigo del .h

            #endif // STORAGE_MANAGER_H
            ```
        2. Después vamos a configurar una constante para modificarla con `idf.py menuconfig`:
            1. Se debe crear en la raíz del componente el archivo `Kconfig.projbuild`
            2. Dentro del archivo se debe escribir la siguiente información:
                ```
                menu "Logger Storage Configuration"
                    config STORAGE_PARTITION_LABEL
                        string "Storage Partition Label"
                        default "storage"
                        help
                            The label of the partition to use for storage. This partition must be
                            defined in the partition table CSV file.
                endmenu
                ```
                Esto creará el símbolo `CONFIG_STORAGE_PARTITION_LABEL` de tipo string con el nombre de la partición de almacenamiento
            3. Se debe ejecutar en la raíz del proyecto `idf.py menuconfig`, ir al menú `Logger Storage Configuration` y modificar el nombre de `Storage Partition Label` a `storage`. Después salir guardando con `Q` y `Y`. Al hacer esto ya estará disponible el símbolo `CONFIG_STORAGE_PARTITION_LABEL`.
                - Si no sale el menú, recargar el proyecto de CMake usando `idf.py reconfigure`.

       3. Dentro de la guardia, incluimos las librerías `esp_err.h` y `esp_spiffs.h` y creamos una clase de tipo singleton (el tipo singleton se define en cada atributo y función de la clase), llamada `StoragePartitionManager` con las funciones públicas `mount`, `unmount` y `format`, de tipo `esp_err_t` (el tipo de error que se utiliza de forma estándar cuando se programa para ESP32) y el atributo privado con la configuración de la partición `conf` de tipo `esp_vfs_spiffs_conf_t`:
            ```
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
            ```
    6. En el archivo `storage-manager.cpp`:
        1. Incluimos las librerías necesarias y la etiqueta para registrar los mensajes de error que puedan darse:
            ```
            #include "storage-manager.h"
            #include "esp_log.h"

            const char* COMPONENT_TAG_STORAGE_PARTITION_MANAGER = "STORAGE_PARTITION_MANAGER";
            ```
        2. Escribimos la definición de la función `mount()`:
            ```
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
            ```
            Registramos el sistema de archivos en el sistema operativo, y comprobamos que la partición es consistente.
        3. Escribimos la definición de la función `unmount()`:
            ```
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
            ```
        4. Escribimos la definición de la función `format()`:
            ```
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
            ```
    7. Con esto tenemos terminado el componente StoragePartitionManager, vamos a probar que funciona, para ello vamos a cargar un fichero y leerlo desde el ESP32:
        1. En el `CMakeList.txt` del módulo vamos a añadir la siguiente línea para poder nosotros añadir archivos directamente al flashear el proyecto: `spiffs_create_partition_image(storage "../../storage_spiffs_root" FLASH_IN_PROJECT)`
        2. En la raíz del proyecto vamos a crear una carpeta llamada `storage_spiffs_root`.
        3. Dentro de la carpeta vamos a crear un archivo llamado `readme.txt` y vamos a escribir el texto de nuestra elección dentro.
        4. Ahora vamos a ir a la raíz del proyecto y modificaremos `main/logger.cpp` para añadir lo siguiente:
            ```
            #include "esp_log.h"
            #include "storage-manager.h"

            const char* MAIN_TAG = "MAIN";

            extern "C" void app_main(void)
            {
                StoragePartitionManager::mount();

                FILE* file = fopen("/storage/readme.txt", "r");
                if (file == nullptr)
                {
                    ESP_LOGE(MAIN_TAG, "Failed to open file for reading");
                }
                else
                {
                    char line[1024];
                    while (fgets(line, sizeof(line), file))
                    {
                        printf("%s", line);
                    }
                    fclose(file);
                }

                StoragePartitionManager::unmount();
            }
            ```
            Como ves, una vez montado el sistema de archivos, se comporta como una carpeta y se puede utilizar con las funciones de C de manejo de archivos.


5. Ahora debemos de añadir la funcionalidad de logger. Queremos que cada vez que se inicie el ESP32, se guarde una línea en el log que diga: `Inicio nº X` siendo X el número de veces que se ha iniciado anteriormente, y además, queremos que se impriman todas las líneas guardadas. Para ello:
    1. Vamos a comentar el código que lee el archivo `readme.txt` (desde `StoragePartitionManager::mount()` hasta `StoragePartitionManager::unmount()` sin incluir ninguna de esas 2 líneas).
    2. Ahora vamos a trabajar justo debajo de las líneas comentadas. Vamos a guardar en un archivo cual es el número del último inicio, para poder leerlo al iniciar el sistema, y actualizarlo: lo primero es leer el número ya escrito:
        ```
        int bootNumber = 0;  
        FILE\* logFile = nullptr;  
        FILE\* bootNumberFile = fopen("/storage/bootNumber.txt", "r");  
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
        ```
        Si no se puede abrir el archivo (primer inicio), solamente inicializamos la variable `bootNumber` a 0
    3. Ahora vamos a guardar en el archivo del número de inicio el valor actualizado contando este inicio, y vamos a añadir al final del archivo de log la línea correspondiente a este inicio.
        ```
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
           fprintf(logFile, "Inicio nº %d\\n", bootNumber);  
           fclose(logFile);  
           }  
        }
        ```
    4. Por último, vamos a leer el archivo de log entero:
        ```
        logFile = fopen("/storage/log.txt", "r");  
        if (logFile == nullptr)  
        {  
           ESP_LOGE(MAIN_TAG, "Failed to open /storage/log.txt for reading");  
        }  
        else  
        {  
           char line\[1024\];  
         while (fgets(line, sizeof(line), logFile))  
           {  
           printf("%s", line);  
           }  
           fclose(logFile);  
        }
        ```
6. Solo queda probar que funciona correctamente, para ello:
    - Resetea los archivos con `idf.py flash`
    - Observa el funcionamiento con `idf.py monitor`
        - Usando ctrl+T y después ctrl+R, se reinicia el ESP32
        - Usando ctrl+T y después ctrl+X, se sale del monitor
