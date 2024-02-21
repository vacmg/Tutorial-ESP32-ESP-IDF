# ESP32 CON ESP-IDF: HELLO WORLD

1. Instalación de la toolchain:
    1. Clonar el repositorio con la Toolchain ESP-IDF: `git clone --recurse-submodules https://github.com/espressif/esp-idf.git`
    
    2. Instalar la toolchain: `cd esp-idf; ./install.sh`

2. Activación de la Toolchain
    Para poder utilizar la toolchain, se deben exportar una serie de variables de entorno, para ello, cada vez que se abra una terminal, se debe navegar a la instalacion de idf (`cd esp-idf`) y ejecutar `. export.sh`
    Al hacer esto, se habilitará el frontent de la toolchain, que nos permitira trabajar con este entorno.
    - Tip: puedes guardar en un alias este comando editando el archivo $HOME/.bashrc y añadiendo `alias esp=". $HOME/rutaAInstalacion/esp-idf/export.sh"`

3. Creacion de un proyecto nuevo:
    1. Primero se activa el entorno, lo haremos siguiendo el paso 2: `. $HOME/rutaAInstalacion/esp-idf/export.sh`
    
    2. Creamos un proyecto vacío, para ello utilizaremos el frontend de la toolchain: `idf.py`: `idf.py create-project hello-world`
    
    3. Si navegamos dentro de la carpeta creada del proyecto, tenemos 2 elementos: `CMakeLists.txt  main`. 
        Para este ejemplo, el archivo CMakeLists.txt no hay que modificarlo, pero guarda la configuración del proyecto, que librerias externas se utilizan, etc. la carpeta main contiene el componente principal del proyecto.
    
    4. Si navegamos dentro de la carpeta main, veremos 2 archivos: `CMakeLists.txt  hello-world.c`
        Nuevamente, el CMakeLists.txt contiene la configuración del componente y el archivo hello-world.c contiene la función principal del programa, que se llama despues de iniciar el microcontrolador.
    
    5. Abrimos hello-world.c con un editor de texto y veremos lo siguiente:
        ```
        #include <stdio.h>

        void app_main(void)
        {

        }
        ```
        La funcion app_main() es el equivalente a void main() en un programa de C estandar. Ahora todo se parece un poco más. Se ha incluido la librería stdio.h, responsable de la familia de funciones printf().
    
    6. Prueba a mandar un mensaje:
        ```
        #include <stdio.h>

        void app_main(void)
        {
            printf("Hello World!!!");
        }
        ```
    7. Ahora toca probar que funciona, lo primero que hay que hacer es compilar. Para ello situate en la carpeta raiz del proyecto (hello-world) y ejecuta: `idf.py build`
    
    8. Estás programando un microcontrolador, como el programa no lo ejecuta tu ordenador, tienes que mandarselo, esto se llama flashear: `idf.py flash`
        - **IMPORTANTE:** Para evitar desgastar de más la memoria del microcontrolador, una vez se ha flasheado por primera vez un proyecto concreto, para sucesivos flasheos al cambiar el codigo, es mejor utilizar en su lugar `idf.py app-flash` para solo flsahear el codigo y no el resto de particiones con datos que no han cambiado.
        - **En caso de fallo al subir, comprobar que se pertenece al grupo DIALOUT: ** `sudo usermod -a -G dialout $USER` y reiniciar el ordenador para que surja efecto.
        
        - Tip 1: `idf.py` es inteligente, no hace falta que compiles antes de flashear, si hay algun cambio en el codigo, o si no se ha compilado antes, se hará automáticamente antes de flashear.
    
    9. ¿Habrá funcionado? Para comprobarlo hay que abrir el monitor, el puerto de comunicaciones entre tu ordenador y el microcontrolador: `idf.py monitor`
        Si todo ha salido bien, deberias ver el mensaje de Hello world justo al final del log del arranque del sistema.
        - Tip 1: Se pueden encadenar comandos en `idf.py`, por lo que todo los pasos anteriores se pueden hacer directamente escribiendo `idf.py flash monitor`
        - Tip 2: Desde el monitor se pueden hacer bastantes [cosas](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-monitor.html), para activar las acciones especiales, pulsa la tecla ctrl+T y luego pulsa la tecla de la acción correspondiente. Por ejemplo, para mostrar el menu de ayuda pulsa ctrl + T seguido de ctrl + H; o para cerrar el monitor ctrl + T seguido de ctrl + X

    10. Todo listo!!! Si quieres seguir probando, prueba a utilizar otras funciones de la libreria estandar de C, deberian funcionar igual a como lo hacen en tu ordenador.
    
    11. Bonus Track: Hasta ahora hemos utilizado la terminal y un editor de textos basico para trabajar, pero se puede hacer mucho más.
        Vamos a configurar un IDE para que sea capaz de autocompletar codigo y ayudarnos a detectar errores más facilmente. Los pasos a seguir se harán con el IDE CLion, pero la configuración es similar en otros IDEs compatibles con CMake.
        1. Abrimos la carpeta del proyecto creado con `idf.py create-project` en CLion. Debería salir un asistente de configuración de CMake. Si no sale, navegar a File -> Settings -> Build, Execution, Deployment -> CMake
        2. En la configuración de CMake seleccionamos `Ninja` como Generador y `build` como build folder.
        3. En la configuración de CMake, entramos en el menu de environment, y añadimos una nueva variable llamada `IDF_PATH` con la ruta absoluta a la carpeta de instalación de la toolchain (`esp-idf/`)
        4. En una terminal, ejecutamos la activación de la toolchain (`. export.sh`) y copiamos el PATH modificado (`echo $PATH`)
        5. En el menú de environment, editamos la variable PATH, borrando todo lo que haya y sustituyendolo por el PATH que hemos copiado antes.
        6. Dar a OK y aplicar, y ver como se recarga la configuración, y todo está disponible desde el IDE.

