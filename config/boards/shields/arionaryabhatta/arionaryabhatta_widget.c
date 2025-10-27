/*
 * Copyright (c) 2025 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/display.h>
#include <zmk/display/status_screen.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/battery.h>

#include <lvgl.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

LV_IMG_DECLARE(balloon);

struct status_state {
    lv_obj_t *connection_icon;
    lv_obj_t *time_label;
    lv_obj_t *date_label;
    lv_obj_t *battery_icon;
};

static struct status_state state = {
    .connection_icon = NULL,
    .time_label = NULL,
    .date_label = NULL,
    .battery_icon = NULL
};

// WiFi icon symbols
#define WIFI_ICON LV_SYMBOL_WIFI
#define NO_WIFI_ICON "  "

static void set_connection_status() {
    if (state.connection_icon == NULL) {
        return;
    }
    
    if (zmk_usb_is_powered() || zmk_ble_active_profile_is_connected()) {
        lv_label_set_text(state.connection_icon, WIFI_ICON);
    } else {
        lv_label_set_text(state.connection_icon, NO_WIFI_ICON);
    }
}

static void set_battery_status() {
    if (state.battery_icon == NULL) {
        return;
    }
    
    uint8_t battery_level = zmk_battery_state_of_charge();
    
    if (battery_level > 80) {
        lv_label_set_text(state.battery_icon, LV_SYMBOL_BATTERY_FULL);
    } else if (battery_level > 60) {
        lv_label_set_text(state.battery_icon, LV_SYMBOL_BATTERY_3);
    } else if (battery_level > 40) {
        lv_label_set_text(state.battery_icon, LV_SYMBOL_BATTERY_2);
    } else if (battery_level > 20) {
        lv_label_set_text(state.battery_icon, LV_SYMBOL_BATTERY_1);
    } else {
        lv_label_set_text(state.battery_icon, LV_SYMBOL_BATTERY_EMPTY);
    }
}

static void update_time() {
    if (state.time_label == NULL) {
        return;
    }
    
    int64_t uptime_ms = k_uptime_get();
    int total_seconds = (uptime_ms / 1000) % 86400;
    int hours = (total_seconds / 3600) % 24;
    int minutes = (total_seconds % 3600) / 60;
    
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hours, minutes);
    lv_label_set_text(state.time_label, time_str);
}

lv_obj_t *zmk_display_status_screen() {
    lv_obj_t *screen = lv_obj_create(NULL);
    
    // Set black background
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    
    // Top status bar container
    lv_obj_t *top_bar = lv_obj_create(screen);
    lv_obj_set_size(top_bar, 128, 16);
    lv_obj_align(top_bar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_opa(top_bar, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_bar, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(top_bar, 0, LV_PART_MAIN);
    
    // WiFi icon (left)
    state.connection_icon = lv_label_create(top_bar);
    lv_obj_align(state.connection_icon, LV_ALIGN_LEFT_MID, 2, 0);
    lv_obj_set_style_text_color(state.connection_icon, lv_color_white(), LV_PART_MAIN);
    
    // Date (center)
    state.date_label = lv_label_create(top_bar);
    lv_obj_align(state.date_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(state.date_label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(state.date_label, "27/10/2025");
    
    // Battery icon (right)
    state.battery_icon = lv_label_create(top_bar);
    lv_obj_align(state.battery_icon, LV_ALIGN_RIGHT_MID, -2, 0);
    lv_obj_set_style_text_color(state.battery_icon, lv_color_white(), LV_PART_MAIN);
    
    // Large time display
    state.time_label = lv_label_create(screen);
    lv_obj_align(state.time_label, LV_ALIGN_CENTER, 0, 8);
    lv_obj_set_style_text_color(state.time_label, lv_color_white(), LV_PART_MAIN);
    
    // Try to use large font if available
#if CONFIG_LV_FONT_MONTSERRAT_48
    lv_obj_set_style_text_font(state.time_label, &lv_font_montserrat_48, LV_PART_MAIN);
#elif CONFIG_LV_FONT_MONTSERRAT_32
    lv_obj_set_style_text_font(state.time_label, &lv_font_montserrat_32, LV_PART_MAIN);
#endif
    
    // Initialize all displays
    set_connection_status();
    set_battery_status();
    update_time();
    
    return screen;
}
