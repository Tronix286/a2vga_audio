#include <pico/stdlib.h>
#include <pico/multicore.h>
#include "abus.h"
#include "board_config.h"
#include "config.h"
#include "render.h"
#include "vga.h"
#include "hardware/vreg.h"


static void core1_main() {
    vga_init();
    render_init();
    render_loop();
}

int main() {
    // Adjust system clock for better dividing into other clocks
    //set_sys_clock_khz(CONFIG_SYSCLOCK * 1000, true);

    vreg_set_voltage(VREG_VOLTAGE_1_30);
    sleep_ms(10);
    set_sys_clock_khz(378000, true);


    // Setup the on-board LED for debugging
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

    gpio_init(IRQ_PIN);
    gpio_set_dir(IRQ_PIN, GPIO_OUT);
    gpio_put(IRQ_PIN, 0);
    
    config_load();

    multicore_launch_core1(core1_main);

    abus_init();
    abus_loop();
}
