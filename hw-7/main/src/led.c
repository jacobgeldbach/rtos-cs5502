#include "led_strip.h"
#include "led.h"

#define LED_GPIO    19
#define LED_COUNT   4
#define LED_MASK    (LED_COUNT - 1)

led_strip_handle_t led_strip;

static const char led_strip_task_tag[40] = "LED Strip task";

void v_light_led_strip(void* pv_parameters)
{ 
    ESP_LOGI(led_strip_task_tag, "Starting LED strip task");
    /* Define and init our onboard 4 led strip */
    led_strip_config_t strip_config = {
        .strip_gpio_num = LED_GPIO,
        .max_leds = LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .color_component_format = LED_STRIP_COLOR_COMPONENT_FMT_GRBW, //ours are RGBW in the spec not sure why this works better
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .mem_block_symbols = 64,
    };

    led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip);

    int current = 0;

    /* Default to start */
    led_cmd_t cmd = {
        .red        = 50,
        .green      = 0,
        .blue       = 0,
        .white      = 0,
        .speed      = 500,
        .clockwise  = true,
        .solid      = true,
    };

    while (1) {
        led_cmd_t notif;

        /* Don't block, as we want the LED lighting loop to execute when there are no */
        if (xQueueReceive(led_cmd_queue, &notif, 0) == pdTRUE) {
            ESP_LOGI(led_strip_task_tag, "Received led_cmd notification, populating cmd values");
            
            cmd.red = notif.red;
            cmd.green = notif.green;
            cmd.blue = notif.blue;
            cmd.white = notif.white;
            cmd.speed = notif.speed;
            cmd.clockwise = notif.clockwise;
            cmd.solid = notif.solid;

        }

        if (!cmd.solid) {
            // turn all LEDs off first
            for (int i = 0; i < LED_COUNT; i++) {
                led_strip_set_pixel(led_strip, i, 0, 0, 0);
            }

            /* Turn current led in chain on */
            led_strip_set_pixel(led_strip, current, cmd.red, cmd.green, cmd.blue);
            
            if (cmd.clockwise) {
                current = (current + 1) & LED_MASK;
            }
            else {
                current = (current - 1) & LED_MASK;
            }
        }
        else {
            /* Light all 4 since solid set */
            led_strip_set_pixel(led_strip, 0, cmd.red, cmd.green, cmd.blue);
            led_strip_set_pixel(led_strip, 1, cmd.red, cmd.green, cmd.blue);
            led_strip_set_pixel(led_strip, 2, cmd.red, cmd.green, cmd.blue);
            led_strip_set_pixel(led_strip, 3, cmd.red, cmd.green, cmd.blue);
        }

        led_strip_refresh(led_strip);

        vTaskDelay(pdMS_TO_TICKS(cmd.speed));
    }
}
