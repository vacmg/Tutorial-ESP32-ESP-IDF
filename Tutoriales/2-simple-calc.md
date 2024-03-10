# Simple Calculator
    Este proyecto consiste en crear una calculadora basica que acepte 3 operaciones de numeros enteros: la suma, resta y multiplicación utilizando la terminal como entrada. El objetivo es aprender a manejar el componente esp_console, y la salida por terminal.

1. Crea un proyecto vacío llamado simple-calc, utilizando el comando `idf.py create project`
    - Recuerda primero activar el entorno con `. ...esp-idf/export.sh`

> [!NOTE] 
> En el tutorial 1, se te recomendo que guardases este comando en una variable de entorno, en ese caso la llamaban esp, y esto paso puedes hacerlo llamando a ese alias.
> Si no lo hiciste y quieres hacerlo, puedes llamarlo con `alias esp=". $HOME/rutaAInstalacion/esp-idf/export.sh"`


2. Para implementar una consola interactiva, se debe implementar un sistema REPL (Read Eval Print Loop). Por suerte, esp-idf tiene una [librería](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/system/console.html) que podemos utilizar.
    Vamos a trabajar en el componente main (simple-calc.c) la configuración de la terminal
    
    1. Primero debemos incluir las librerías que vamos a necesitar al principio del programa:
        ```
        #include <stdio.h>
        #include <string.h>
        #include <esp_console.h>
        #include <linenoise/linenoise.h>
        #include "argtable3/argtable3.h"
        #include <esp_log.h>
        #include <esp_vfs_dev.h>
        #include "driver/uart.h"
        ```
    
    2. Después se debe configurar el modo de operación de la terminal, estos ajustes no vamos a verlos a fondo, pero si destacar que se está configurando una librería para el autocompletado al pulsar tabulador, y otra para guardar el historial de comandos.
        ```
        static void initialize_console(void)
        {
            /* Drain stdout before reconfiguring it */
            fflush(stdout);
            fsync(fileno(stdout));

            /* Disable buffering on stdin */
            setvbuf(stdin, NULL, _IONBF, 0);

            /* Minicom, screen, idf_monitor send CR when ENTER key is pressed */
            esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
            /* Move the caret to the beginning of the next line on '\n' */
            esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

            /* Configure UART. Note that REF_TICK is used so that the baud rate remains
             * correct while APB frequency is changing in light sleep mode.
             */
            const uart_config_t uart_config =
                    {
                    .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
                    .data_bits = UART_DATA_8_BITS,
                    .parity = UART_PARITY_DISABLE,
                    .stop_bits = UART_STOP_BITS_1,
                    #if SOC_UART_SUPPORT_REF_TICK
                    .source_clk = UART_SCLK_REF_TICK,
                    #elif SOC_UART_SUPPORT_XTAL_CLK
                    .source_clk = UART_SCLK_XTAL,
                    #endif
            };
            /* Install UART driver for interrupt-driven reads and writes */
            ESP_ERROR_CHECK( uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0) );
            ESP_ERROR_CHECK( uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

            /* Tell VFS to use UART driver */
            esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);

            /* Configure the console */
            esp_console_config_t console_config = ESP_CONSOLE_CONFIG_DEFAULT();
            console_config.max_cmdline_length = 64;
            ESP_ERROR_CHECK(esp_console_init(&console_config));

            /* Configure linenoise line completion library */

            /* Enable multiline editing. If not set, long commands will scroll within
             * single line.
             */
            linenoiseSetMultiLine(1);

            /* Tell linenoise where to get command completions and hints */
            linenoiseSetCompletionCallback(&esp_console_get_completion);
            linenoiseSetHintsCallback((linenoiseHintsCallback*) &esp_console_get_hint);

            /* Set command history size */
            linenoiseHistorySetMaxLen(10);

            /* Set command maximum length */
            linenoiseSetMaxLineLen(console_config.max_cmdline_length);

            /* Don't return empty lines */
            linenoiseAllowEmpty(false);
        }
        ```
    
    3. En la funcion `main()`, lo primero que haremos es llamar a la función de configuración de la terminal `initialize_console()` y definir el prompt:
        ```
        #define PROMPT_TEXT "calc>"
        const char* prompt = LOG_COLOR_W PROMPT_TEXT LOG_RESET_COLOR;
        ```
        Hemos decidido darle un color al prompt para que no se vea en blanco (el color elegido es el mismo de los mensajes de warning, amarillo)
        
    4. Creamos una nueva función llamada `registerCommands()`, la llamamos en la funcion `main()`, debajo de la definición del prompt y en la definición escribimos:
        ```
        void registerCommands(void)
        {
            esp_console_register_help_command();
        }

        ```
        Para registrar otros futuros comandos, se añadirán aquí.
        El comando help es un comando integrado que automáticamente muestra un mensaje de ayuda de todos los comandos registrados.
        
    5. En la funcion `main()`: Debajo de la llamada a `registerCommands()` ponemos el mensaje inicial con las instrucciones de uso, y realizamos una comprobación para saber si el terminal es interactivo (soporta caracteres escapados).
        ```
        printf("\n"
           "Type 'help' to get the list of commands.\n"
           "Use UP/DOWN arrows to navigate through command history.\n"
           "Press TAB when typing command name to auto-complete.\n\n");

        /* Figure out if the terminal supports escape sequences */;
        if (linenoiseProbe()) /* zero indicates success */
        {
            printf("\n"
                   "Your terminal application does not support escape sequences.\n"
                   "Line editing and history features are disabled.\n\n");
            linenoiseSetDumbMode(1);
            /* Since the terminal doesn't support escape sequences,
             * don't use color codes in the prompt.
             */
            prompt = PROMPT_TEXT;
        }
        ```
        
    6. ¡¡¡Por fín hemos terminado de configurar la terminal!!! Ahora solo queda ponerla en marcha. Como hemos dicho al principio, se debe implementar un sistema REPL (Read Eval Print Loop), por lo que vamos a necesitar un bucle en el que leamos una línea, procesemos el comando y devolvamos una respuesta, y volvamos al principio a leer la siguiente línea. Si leemos una línea vacía, la ignoramos.
        ```
        while(true)
        {
            /* Get a line using linenoise.
             * The line is returned when ENTER is pressed.
             */
            char* line = linenoise(prompt);
            if (line == NULL)
            { /* Back to read on EOF or error */
                continue;
            }
            /* Add the command to the history if not empty*/
            if (strlen(line) > 0)
            {
                linenoiseHistoryAdd(line);
            }

            /* Try to run the command */
            int ret;
            esp_err_t err = esp_console_run(line, &ret);
            if (err == ESP_ERR_NOT_FOUND)
            {
                printf("Unrecognized command\n");
            }
            else if (err == ESP_OK && ret != ESP_OK)
            {
                printf("Command returned non-zero error code: 0x%x (%s)\n", ret, esp_err_to_name(ret));
            }
            else if (err != ESP_OK)
            {
                printf("Internal error: %s\n", esp_err_to_name(err));
                break;
            }
            /* linenoise allocates line buffer on the heap, so need to free it */
            linenoiseFree(line);
        }
        ```
    7. Si ocurriera un error, antes de terminar la ejecución y fuera del bucle principal, debemos liberar los recursos de la consola. Para ello escribimos `esp_console_deinit()`

3. Una vez programada la terminal, es hora de crear los comandos de las operaciones de la calculadora: `sum a b`, `rest a b` y `mult a b`
    
    1. Empecemos por la suma: Debemos crear una estructura que contenga los argumentos de nuestro comando, siguiendo la documentación de [argTable](https://www.argtable.org/):
        ```
        static struct
        {
            arg_int_t *a;
            arg_int_t *b;
            arg_end_t *end;
        } operation_args;
        ```
        a y b son argumentos de tipo entero, y end se necesita para que funcione el parser.
    
    2. Nuestra función de suma es muy sencilla: Recibe argc y argv como parámetros, que se pasan al parser, este los guarda en la estructura creada anteriormente y a partir de ahí se pueden utilizar con normalidad.
        ```
        static int add_operation(int argc, char **argv)
        {
            int nerrors = arg_parse(argc, argv, (void **) &operation_args);
            if (nerrors != 0)
            {
                arg_print_errors(stderr, operation_args.end, argv[0]);
                return 1;
            }

            printf("Result: %d\n", operation_args.a->ival[0] + operation_args.b->ival[0]);
            return 0;
        }
        ```
    3. Para registrar el comando, se utiliza la función `esp_console_cmd_register()`, pero para que nos sea más cómodo, vamos a crearnos una función llamada `register_sum()` que registre el comando sum:
        ```
        static void register_sum(void)
        {
            operation_args.a = arg_int1(NULL, NULL, "A", "First operand of the addition");
            operation_args.b = arg_int1(NULL, NULL, "B", "Second operand of the addition");
            operation_args.end = arg_end(2);

            const esp_console_cmd_t cmd = {
                    .command = "add",
                    .help = "Perform the addition of two integers: A + B",
                    .hint = NULL,
                    .func = &add_operation,
                    .argtable = &operation_args
            };
            ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
        }
        ```
        Se definen la forma que tendrán los argumentos de la función y sus nombres, y el número de argumentos en .end
        Por último se configura el nombre del comando, su mensaje de ayuda y la función que se utiliza para ejecutar el comando (`add_operation()`).
    
    4. Solo queda añadir la llamada a la función `register_sum()` en `registerCommands()`:
        ```
        void registerCommands(void)
        {
            esp_console_register_help_command();
            register_sum();
        }
        ```
    
    5. Para comprobar el funcionamiento correcto de la terminal, prueba a realizar operaciones como `add 3 5` y `add 3 R`
    6. Para el resto de comandos, hay que repetir este mismo proceso, ya que son muy similares.

4. Compila el proyecto con `idf.py build` y flashea el microcontrolador con `idf.py flash` (`idf.py app-flash` para futuros flasheos).

5. Abre el monitor con `idf.py monitor` y prueba los comandos que has creado. Si todo ha ido bien, deberías ver el resultado de las operaciones en la terminal.
