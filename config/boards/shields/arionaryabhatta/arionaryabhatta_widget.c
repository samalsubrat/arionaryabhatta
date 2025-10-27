/*
 * Copyright (c) 2025 The ZMK Contributors
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zmk/display.h>
#include <zmk/display/widgets/battery_status.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/events/battery_state_changed.h>
#include <zmk/battery.h>

#include <lvgl.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

static sys_slist_t widgets = SYS_SLIST_STATIC_INIT(&widgets);

// WiFi Icon (using Connected/Disconnected status)
static lv_obj_t *connection_icon;
// Time display
static lv_obj_t *time_label;
// Date display  
static lv_obj_t *date_label;
// Battery icon
static lv_obj_t *battery_icon;

// Connection status
static bool is_connected = false;

// Time tracking
static struct k_work_delayable time_update_work;

// WiFi icon symbols (using LVGL symbols)
#define WIFI_ICON LV_SYMBOL_WIFI
#define NO_WIFI_ICON "  "

static void update_connection_icon() {
    if (connection_icon == NULL) {
        return;
    }
    
    if (zmk_usb_is_powered()) {
        lv_label_set_text(connection_icon, WIFI_ICON);
        is_connected = true;
    } else if (zmk_ble_active_profile_is_connected()) {
        lv_label_set_text(connection_icon, WIFI_ICON);
        is_connected = true;
    } else {
        lv_label_set_text(connection_icon, NO_WIFI_ICON);
        is_connected = false;
    }
}

static void update_time_display() {
    if (time_label == NULL) {
        return;
    }
    
    // Get current uptime and convert to time format
    int64_t uptime_ms = k_uptime_get();
    int total_seconds = (uptime_ms / 1000) % 86400; // Wrap at 24 hours
    int hours = (total_seconds / 3600) % 24;
    int minutes = (total_seconds % 3600) / 60;
    
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hours, minutes);
    lv_label_set_text(time_label, time_str);
}

static void update_date_display() {
    if (date_label == NULL) {
        return;
    }
    
    // Static date display - you can make this dynamic if you have RTC
    lv_label_set_text(date_label, "27/10/2025");
}

static void update_battery_icon() {
    if (battery_icon == NULL) {
        return;
    }
    
    uint8_t battery_level = zmk_battery_state_of_charge();
    
    // Battery icon based on level
    if (battery_level > 80) {
        lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_FULL);
    } else if (battery_level > 60) {
        lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_3);
    } else if (battery_level > 40) {
        lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_2);
    } else if (battery_level > 20) {
        lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_1);
    } else {
        lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_EMPTY);
    }
}

static void time_update_work_handler(struct k_work *work) {
    update_time_display();
    // Schedule next update in 1 second
    k_work_schedule(&time_update_work, K_SECONDS(1));
}

static void widget_battery_status_update_cb(struct battery_state_state state) {
    update_battery_icon();
}

static int widget_event_listener(const zmk_event_t *eh) {
    update_connection_icon();
    return 0;
}

ZMK_DISPLAY_WIDGET_LISTENER(widget_battery_status, struct battery_state_state,
                            widget_battery_status_update_cb, NULL)

ZMK_LISTENER(widget, widget_event_listener)
ZMK_SUBSCRIPTION(widget, zmk_usb_conn_state_changed);

int zmk_widget_status_init(lv_obj_t *parent) {
    lv_obj_t *screen = parent;
    
    // Set background to black
    lv_obj_set_style_bg_color(screen, lv_color_black(), LV_PART_MAIN);
    
    // ====== TOP ROW: WiFi Icon, Date, Battery ======
    lv_obj_t *top_row = lv_obj_create(screen);
    lv_obj_set_size(top_row, 128, 20);
    lv_obj_set_pos(top_row, 0, 0);
    lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(top_row, 2, LV_PART_MAIN);
    
    // WiFi Icon (left)
    connection_icon = lv_label_create(top_row);
    lv_obj_align(connection_icon, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_color(connection_icon, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(connection_icon, WIFI_ICON);
    
    // Date (center)
    date_label = lv_label_create(top_row);
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(date_label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(date_label, "27/10/2025");
    
    // Battery Icon (right)
    battery_icon = lv_label_create(top_row);
    lv_obj_align(battery_icon, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_color(battery_icon, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(battery_icon, LV_SYMBOL_BATTERY_FULL);
    
    // ====== MAIN TIME DISPLAY ======
    time_label = lv_label_create(screen);
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, 5);
    lv_obj_set_style_text_color(time_label, lv_color_white(), LV_PART_MAIN);
    
    // Set large font for time
    static lv_style_t time_style;
    lv_style_init(&time_style);
    lv_style_set_text_font(&time_style, &lv_font_montserrat_48);
    lv_obj_add_style(time_label, &time_style, LV_PART_MAIN);
    
    lv_label_set_text(time_label, "20:20");
    
    // Initialize values
    update_connection_icon();
    update_date_display();
    update_battery_icon();
    update_time_display();
    
    // Start time update timer
    k_work_init_delayable(&time_update_work, time_update_work_handler);
    k_work_schedule(&time_update_work, K_SECONDS(1));
    
    return 0;
}

lv_obj_t *zmk_widget_status_obj() {
    return NULL;
}
