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

struct zmk_widget_status {
    sys_snode_t node;
    lv_obj_t *obj;
    lv_obj_t *connection_icon;
    lv_obj_t *time_label;
    lv_obj_t *date_label;
    lv_obj_t *battery_icon;
};

static struct k_work_delayable time_update_work;

// WiFi icon symbols (using LVGL symbols)
#define WIFI_ICON LV_SYMBOL_WIFI
#define NO_WIFI_ICON "  "

static void update_connection_icon(lv_obj_t *icon) {
    if (icon == NULL) {
        return;
    }
    
    if (zmk_usb_is_powered() || zmk_ble_active_profile_is_connected()) {
        lv_label_set_text(icon, WIFI_ICON);
    } else {
        lv_label_set_text(icon, NO_WIFI_ICON);
    }
}

static void update_time_display(lv_obj_t *label) {
    if (label == NULL) {
        return;
    }
    
    // Get current uptime and convert to time format
    int64_t uptime_ms = k_uptime_get();
    int total_seconds = (uptime_ms / 1000) % 86400; // Wrap at 24 hours
    int hours = (total_seconds / 3600) % 24;
    int minutes = (total_seconds % 3600) / 60;
    
    char time_str[6];
    snprintf(time_str, sizeof(time_str), "%02d:%02d", hours, minutes);
    lv_label_set_text(label, time_str);
}

static void update_battery_icon(lv_obj_t *icon) {
    if (icon == NULL) {
        return;
    }
    
    uint8_t battery_level = zmk_battery_state_of_charge();
    
    // Battery icon based on level
    if (battery_level > 80) {
        lv_label_set_text(icon, LV_SYMBOL_BATTERY_FULL);
    } else if (battery_level > 60) {
        lv_label_set_text(icon, LV_SYMBOL_BATTERY_3);
    } else if (battery_level > 40) {
        lv_label_set_text(icon, LV_SYMBOL_BATTERY_2);
    } else if (battery_level > 20) {
        lv_label_set_text(icon, LV_SYMBOL_BATTERY_1);
    } else {
        lv_label_set_text(icon, LV_SYMBOL_BATTERY_EMPTY);
    }
}

static void time_update_work_handler(struct k_work *work) {
    // This will be called every second - update time for all widgets
    // For simplicity, we'll handle updates in the main init function
}

int zmk_widget_status_init(struct zmk_widget_status *widget, lv_obj_t *parent) {
    widget->obj = lv_obj_create(parent);
    lv_obj_set_size(widget->obj, 128, 64);
    
    // Set background to black
    lv_obj_set_style_bg_color(widget->obj, lv_color_black(), LV_PART_MAIN);
    lv_obj_set_style_border_width(widget->obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(widget->obj, 0, LV_PART_MAIN);
    
    // ====== TOP ROW: WiFi Icon, Date, Battery ======
    lv_obj_t *top_row = lv_obj_create(widget->obj);
    lv_obj_set_size(top_row, 128, 20);
    lv_obj_set_pos(top_row, 0, 0);
    lv_obj_set_style_bg_opa(top_row, LV_OPA_TRANSP, LV_PART_MAIN);
    lv_obj_set_style_border_width(top_row, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(top_row, 2, LV_PART_MAIN);
    
    // WiFi Icon (left)
    widget->connection_icon = lv_label_create(top_row);
    lv_obj_align(widget->connection_icon, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_set_style_text_color(widget->connection_icon, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(widget->connection_icon, WIFI_ICON);
    
    // Date (center)
    widget->date_label = lv_label_create(top_row);
    lv_obj_align(widget->date_label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(widget->date_label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(widget->date_label, "27/10/2025");
    
    // Battery Icon (right)
    widget->battery_icon = lv_label_create(top_row);
    lv_obj_align(widget->battery_icon, LV_ALIGN_RIGHT_MID, 0, 0);
    lv_obj_set_style_text_color(widget->battery_icon, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(widget->battery_icon, LV_SYMBOL_BATTERY_FULL);
    
    // ====== MAIN TIME DISPLAY ======
    widget->time_label = lv_label_create(widget->obj);
    lv_obj_align(widget->time_label, LV_ALIGN_CENTER, 0, 5);
    lv_obj_set_style_text_color(widget->time_label, lv_color_white(), LV_PART_MAIN);
    
    // Set large font for time
    static lv_style_t time_style;
    lv_style_init(&time_style);
    lv_style_set_text_font(&time_style, &lv_font_montserrat_48);
    lv_obj_add_style(widget->time_label, &time_style, LV_PART_MAIN);
    
    lv_label_set_text(widget->time_label, "20:20");
    
    // Initialize values
    update_connection_icon(widget->connection_icon);
    update_battery_icon(widget->battery_icon);
    update_time_display(widget->time_label);
    
    return 0;
}

lv_obj_t *zmk_widget_status_obj(struct zmk_widget_status *widget) {
    return widget->obj;
}
