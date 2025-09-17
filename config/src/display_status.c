#include <zmk/display/status_screen.h>
#include <zmk/events/usb_conn_state_changed.h>
#include <zmk/event_manager.h>
#include <zmk/usb.h>
#include <zmk/ble.h>
#include <zmk/endpoints.h>
#include <lvgl.h>

static lv_obj_t *status_screen;
static lv_obj_t *title_label;
static lv_obj_t *status_label;
static lv_obj_t *bt_label;

static void update_status_display() {
    if (!status_label) return;
    
    // Update connection status
    if (zmk_usb_is_powered()) {
        lv_label_set_text(status_label, "USB Connected");
    } else if (zmk_ble_active_profile_is_connected()) {
        lv_label_set_text(status_label, "BLE Connected");
    } else {
        lv_label_set_text(status_label, "Disconnected");
    }
    
    // Update BT profile info
    if (bt_label) {
        char bt_text[32];
        snprintf(bt_text, sizeof(bt_text), "Profile: %d", zmk_ble_active_profile_index());
        lv_label_set_text(bt_label, bt_text);
    }
}

lv_obj_t *zmk_display_status_screen() {
    // Create the main screen
    status_screen = lv_obj_create(NULL);
    
    // Title label
    title_label = lv_label_create(status_screen);
    lv_label_set_text(title_label, "ARION NUMPAD");
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 5);
    
    // Status label
    status_label = lv_label_create(status_screen);
    lv_obj_align(status_label, LV_ALIGN_CENTER, 0, -5);
    
    // Bluetooth profile label
    bt_label = lv_label_create(status_screen);
    lv_obj_align(bt_label, LV_ALIGN_BOTTOM_MID, 0, -5);
    
    // Initial status update
    update_status_display();
    
    return status_screen;
}

static int display_status_listener(const zmk_event_t *eh) {
    update_status_display();
    return ZMK_EV_EVENT_BUBBLE;
}

ZMK_LISTENER(display_status, display_status_listener);
ZMK_SUBSCRIPTION(display_status, zmk_usb_conn_state_changed);
