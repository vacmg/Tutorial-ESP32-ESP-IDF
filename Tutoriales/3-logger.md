En este proyecto, vamos a crear un logger que registra un mensaje en un archivo cada vez que el ESP32 se inicia. También imprimirá cada línea presente en el archivo de logs.

1. Hay que crear un nuevo proyecto llamado logger:
    1. Se debe ejecutar `idf.py create-project logger`.
    2. Si se va a utilizar un IDE para programar el proyecto, se debe configurar CMake en el IDE (según lo visto en el punto 11 del tutorial 1 (Hello world)).

2. En este proyecto vamos a utilizar componentes (aka librerias externas) y clases (C++). Por lo tanto antes de continuar debemos saber que el compilador averigua automáticamente el lenguaje utilizado en función de si el archivo de código termina en `.c` o `.cpp`. 
    1. Para poder utilizar clases debemos cambiar el archivo principal `main/logger.c` a `main/logger.cpp`. 
    2. Tras hacer esto, se debe reflejar el cambio de extensión del archivo de codigo en el archivo de configuración `main/CMakeLists.txt`: `idf_component_register(SRCS "logger.cpp" INCLUDE_DIRS ".")`.
    3. Por otro lado, el sistema operativo espera que la función `app_main()` sea una función de C, por lo que se debe modificar la cabecera de la función para indicar que se mantenga compatibilidad con C:
        ```
        extern "C" void app_main(void)
        {

        }
        ```

3. Ahora hay que preparar la tabla de particiones:
    1. Primero se debe crear el archivo que contiene las especificaciones de las particiones: `touch partitions.csv` (se debe crear este archivo en la raiz del proyecto)
    2. Se rellena el archivo con la siguiente información:
        Una partición para el programa, llamada `bin`, de tipo `app` (codigo) y subtipo `factory` (el subtipo no es necesario a menos que se planee utilizar para actualizaciones via OTA) de 1MB de tamaño.
        Una partición de datos llamada `storage`, de tipo `data` y subtipo `spiffs` (es un sistema de archivos, como FAT o NTFS) y tamaño 512KB.
        ```
        # Name,   Type, SubType, Offset,  Size, Flags

        bin,  app,  factory, ,  1M,
        storage,  data, spiffs, , 512K,
        ```
    3. Se configura el proyecto para utilizar la tabla de particiones creada:
        1. Se ejectuta `idf.py menuconfig`.
        2. Se navega al menu `Partition Table`.
        3. Se mofifica la entrada `Partition Table` a `Custom partition table CSV`.
        4. Se modifica la entrada `Custom partition CSV file` al nombre del archivo de particiones (`pasrtitions.csv`).
        5. Se guarda y se sale de la configuración pulsando `Q` para salir y luego `Y` para guardar los cambios.
    


4. Después vamos a crear el componente encargado de preparar el sistema de archivos y la partición de almacenamiento
    1. Se debe ejecutar en la raiz del proyecto `idf.py create-component -C components storage-manager`. Esto creará el componente `storage-manager` en la carpeta `components`.
    2. A partir de ahora vamos a trabajar en la carpeta `components/storage-manager/`
    3. Al igual que con `main/logger.c`, hay que cambiar la extensión del archivo `storage-manager.c` a `.cpp` y reflejar los cambios en su `CMakeList.txt`
    4. En el CMakeList.txt del componente, se debe modificar la entrada `idf_component_register` para incluir al final `REQUIRES spiffs`: `idf_component_register(SRCS "storage-manager.cpp" INCLUDE_DIRS "include" REQUIRES spiffs)`. Esto sirve para incluir la librería `spiffs` encargada de manejar el sistema de archivos de la partición `storage`.
    5. En el archivo `include/storage-manager.h`:
        1. Borramos el codigo de ejemplo y lo sustituimos por la guardia para incluir solo una vez un .h (que se debe utilizar cada vez que se crea un .h), donde `STORAGE_MANAGER_H` es un símbolo que se utiliza por convenio para este proposito (NOMBRE_DEL_ARCHIVO_H). Lo que hace es que si se realiza `#include "storage-manager.h"` por primera vez, al no estar definido el símbolo `STORAGE_MANAGER_H` se entra dentro del if y se define el símbolo junto con el resto del codigo del .h. Si luego se incluye el .h otra vez, al ya estar definido el símbolo de la guardia, no se entra dentro del if y no se duplica el código:
            ```
            #ifndef STORAGE_MANAGER_H
            #define STORAGE_MANAGER_H

            // Aquí escribiremos el codigo del .h

            #endif // STORAGE_MANAGER_H
            ```
        2. Despues vamos a configurar una constante para modificarla con `idf.py menuconfig`:
            1. Se debe crear en la raiz del componente el archivo `Kconfig.projbuild`
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
                Esto creará el simbolo `CONFIG_STORAGE_PARTITION_LABEL` de tipo string con el nombre de la partición de almacenamiento
            3. Se debe ejecutar en la raiz del proyecto `idf.py menuconfig`, ir al menú `Logger Storage Configuration` y modificar el nombre de `Storage Partition Label` a `storage`. Después salir guardando con `Q` y `Y`. Al hacer esto ya estará disponible el simbolo `CONFIG_STORAGE_PARTITION_LABEL`.
                - Si no sale el menú, recargar el proyecto de CMake usando `idf.py reconfigure`.

        4. Dentro de la guardia, incluimos las librerías `esp_err.h` y `esp_spiffs.h` y creamos una clase de tipo singleton (el tipo singleton se define en cada atributo y función de la clase), llamada `StoragePartitionManager` con las funciones publicas `mount`, `unmount` y `format`, de tipo `esp_err_t` (el tipo de error que se utiliza de forma estandar cuando se programa para ESP32) y el atributo privado con la configuración de la partición `conf` de tipo `esp_vfs_spiffs_conf_t`:
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
        1. 

