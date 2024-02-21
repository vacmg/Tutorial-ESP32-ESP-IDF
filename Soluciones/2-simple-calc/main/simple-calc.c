#include <stdio.h>
#include <string.h>
#include <esp_console.h>
#include <linenoise/linenoise.h>
#include "argtable3/argtable3.h"
#include <esp_log.h>
#include <esp_vfs_dev.h>
#include "driver/uart.h"

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

static struct
{
    arg_int_t *a;
    arg_int_t *b;
    arg_end_t *end;
} operation_args;

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

static int sub_operation(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &operation_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, operation_args.end, argv[0]);
        return 1;
    }

    printf("Result: %d\n", operation_args.a->ival[0] - operation_args.b->ival[0]);
    return 0;
}

static void register_rest(void)
{
    operation_args.a = arg_int1(NULL, NULL, "A", "First operand of the subtraction");
    operation_args.b = arg_int1(NULL, NULL, "B", "Second operand of the subtraction");
    operation_args.end = arg_end(2);

    const esp_console_cmd_t cmd = {
            .command = "rest",
            .help = "Perform the subtraction of two integers: A - B",
            .hint = NULL,
            .func = &sub_operation,
            .argtable = &operation_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

static int mult_operation(int argc, char **argv)
{
    int nerrors = arg_parse(argc, argv, (void **) &operation_args);
    if (nerrors != 0)
    {
        arg_print_errors(stderr, operation_args.end, argv[0]);
        return 1;
    }

    printf("Result: %d\n", operation_args.a->ival[0] * operation_args.b->ival[0]);
    return 0;
}

static void register_mult(void)
{
    operation_args.a = arg_int1(NULL, NULL, "A", "First operand of the multiplication");
    operation_args.b = arg_int1(NULL, NULL, "B", "Second operand of the multiplication");
    operation_args.end = arg_end(2);

    const esp_console_cmd_t cmd = {
            .command = "mult",
            .help = "Perform the multiplication of two integers: A * B",
            .hint = NULL,
            .func = &mult_operation,
            .argtable = &operation_args
    };
    ESP_ERROR_CHECK( esp_console_cmd_register(&cmd) );
}

void registerCommands(void)
{
    esp_console_register_help_command();
    register_sum();
    register_rest();
    register_mult();
}

void app_main(void)
{
    initialize_console();

    #define PROMPT_TEXT "calc>"
    const char* prompt = LOG_COLOR_W PROMPT_TEXT LOG_RESET_COLOR;

    /* Register commands */
    registerCommands();

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

    /* Main loop */
    while(true)
    {
        /* Get a line using linenoise.
         * The line is returned when ENTER is pressed.
         */
        char* line = linenoise(prompt);
        if (line == NULL)
        { /* Break on EOF or error */
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

    esp_console_deinit();
}
