#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/display.h>
#include <zephyr/drivers/sensor.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
LOG_MODULE_REGISTER(app);

#define DHT_NODE DT_PATH(dht11)

#define DISPLAY_WIDTH  256
#define DISPLAY_HEIGHT 120

static lv_obj_t *label_temp, *label_humid, *label_heat_index;

int main(void)
{
    const struct device *display_dev = DEVICE_DT_GET(DT_CHOSEN(zephyr_display));
    const struct device *dht_dev = DEVICE_DT_GET(DHT_NODE);

    if (!device_is_ready(display_dev)) {
        LOG_ERR("Display not ready");
        return 1;
    }

    if (!device_is_ready(dht_dev)) {
        LOG_ERR("DHT11 sensor not ready");
        return 1;
    }

    lv_init();
    lv_obj_set_style_bg_color(lv_scr_act(), lv_color_white(), LV_PART_MAIN);
    lv_obj_set_style_bg_opa(lv_scr_act(), LV_OPA_COVER, 0);
    
    // Create temperature label
    label_temp = lv_label_create(lv_scr_act());
    lv_obj_align(label_temp, LV_ALIGN_TOP_LEFT, 10, 10);
    lv_obj_set_width(label_temp, 300);
    lv_obj_set_height(label_temp, 31);
    lv_obj_set_style_bg_opa(label_temp, LV_OPA_TRANSP, LV_PART_MAIN);

    // Create humidity label
    label_humid = lv_label_create(lv_scr_act());
    lv_obj_align(label_humid, LV_ALIGN_TOP_LEFT, 10, 40);
     lv_obj_set_width(label_humid, 300);
    lv_obj_set_height(label_humid, 31);
        lv_obj_set_style_bg_opa(label_humid, LV_OPA_TRANSP, LV_PART_MAIN);


    // Create heat index label
    label_heat_index = lv_label_create(lv_scr_act());
    lv_obj_align(label_heat_index, LV_ALIGN_TOP_LEFT, 10, 70);
         lv_obj_set_width(label_heat_index, 300);
    lv_obj_set_height(label_heat_index, 31);
            lv_obj_set_style_bg_opa(label_heat_index, LV_OPA_TRANSP, LV_PART_MAIN);


    lv_obj_set_style_text_font(label_temp, &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_text_font(label_humid, &lv_font_montserrat_24, LV_PART_MAIN);
        lv_obj_set_style_text_font(label_heat_index, &lv_font_montserrat_24, LV_PART_MAIN);


    display_blanking_off(display_dev);
    k_msleep(500);

    int last_temp = INT32_MIN;
    int last_humid = INT32_MIN;
    char temp_str[64];
    char humid_str[64];
    char heat_str[64];

    while (1) {
    struct sensor_value temp, humidity;

    if (sensor_sample_fetch(dht_dev) == 0 &&
        sensor_channel_get(dht_dev, SENSOR_CHAN_AMBIENT_TEMP, &temp) == 0 &&
        sensor_channel_get(dht_dev, SENSOR_CHAN_HUMIDITY, &humidity) == 0) {

        if (temp.val1 != last_temp || humidity.val1 != last_humid) {
            last_temp = temp.val1;
            last_humid = humidity.val1;

            int temp_c_int = temp.val1 * 100 + temp.val2 / 10000;
            int rh_int     = humidity.val1 * 100 + humidity.val2 / 10000;

            float temp_f = temp_c_int * 9.0f / 500.0f + 32.0f; // 100x scale
            float rh_f = rh_int / 100.0f;

            float HI_f = -42.379f + 2.04901523f * temp_f + 10.14333127f * rh_f
                       - 0.22475541f * temp_f * rh_f - 0.00683783f * temp_f * temp_f
                       - 0.05481717f * rh_f * rh_f + 0.00122874f * temp_f * temp_f * rh_f
                       + 0.00085282f * temp_f * rh_f * rh_f
                       - 0.00000199f * temp_f * temp_f * rh_f * rh_f;

            int HI_c_int = (int)(((HI_f - 32.0f) * 5.0f / 9.0f) * 100.0f);

            snprintf(temp_str, sizeof(temp_str), "Temp: %d.%02d C", temp_c_int / 100, temp_c_int % 100);
            snprintf(humid_str, sizeof(humid_str), "Humidity: %d.%02d %%", rh_int / 100, rh_int % 100);
            snprintf(heat_str, sizeof(heat_str), "Heat Index: %d.%02d C",
                     (temp_c_int >= 2600 ? HI_c_int : temp_c_int) / 100,
                     (temp_c_int >= 2600 ? HI_c_int : temp_c_int) % 100);
                
            lv_label_set_text(label_temp, temp_str);
            lv_label_set_text(label_humid, humid_str);
            lv_label_set_text(label_heat_index, heat_str);

            lv_obj_invalidate(label_temp);
            lv_obj_invalidate(label_humid);
            lv_obj_invalidate(label_heat_index);

            lv_timer_handler();
            lv_refr_now(NULL); 
        } else {
            LOG_INF("Values unchanged, skipping redraw");
        }
    } else {
        LOG_ERR("Failed to read from DHT11");
    }

    k_sleep(K_SECONDS(1));
}


    return 0;
}
