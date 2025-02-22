#include <Wire.h> // Added as requested
#include <M5Unified.h>
#include <M5GFX.h>
#include "lv_conf.h"
#include <lvgl.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>

// Declare the Unscii 16 font
LV_FONT_DECLARE(lv_font_unscii_16);

// Define screen dimensions for CoreS3
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_WIDTH * 10];

// Display and input drivers
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

// Arduino Cloud credentials (replace with your own)
const char DEVICE_LOGIN_NAME[] = "45444c7a-9c6a-42ed-8c94-413e098a2b3d";
const char SSID[] = "Wack House";
const char PASS[] = "justice69";
const char DEVICE_KEY[] = "mIB9Ezs8tfWtqe8Mx2qhQgYxT";

// Cloud variables
String logEntry;

// Menu options
const char* genders[] = {"Male", "Female"};
const char* colors[] = {"White", "Black", "Blue", "Yellow", "Green", "Orange", "Pink", "Red", "Grey"};
const char* items[] = {"Jewelry", "Women's Shoes", "Men's Shoes", "Cosmetics", "Fragrances", "Home", "Kids"};

// LVGL objects
lv_obj_t *mainScreen, *menuScreen, *genderMenu, *shirtMenu, *pantsMenu, *shoeMenu, *itemMenu, *confirmScreen;
lv_obj_t *wifiIndicator;
String currentEntry = "";

// Style for buttons
static lv_style_t btn_style;
void initButtonStyle() {
    lv_style_init(&btn_style);
    lv_style_set_bg_color(&btn_style, lv_color_hex(0x404040));
    lv_style_set_border_color(&btn_style, lv_color_hex(0xFFFFFF));
    lv_style_set_border_width(&btn_style, 2);
    lv_style_set_radius(&btn_style, 5);
    lv_style_set_text_color(&btn_style, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&btn_style, &lv_font_unscii_16);
    lv_style_set_pad_all(&btn_style, 5);
}

// Function prototypes
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void my_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
void createMainMenu();
void updateWifiIndicator();
void createGenderMenu();
void createColorMenu(const char* title, lv_obj_t** menu, void (*nextMenuFunc)());
void colorMenuCallback(lv_event_t *e);
void createItemMenu();
void createConfirmScreen();
String getFormattedEntry(const String& entry);
String getTimestamp();
void saveEntry(const String& entry);

void initProperties() {
    ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
    ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
    ArduinoCloud.addProperty(logEntry, READWRITE, ON_CHANGE);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg);
    M5.Display.setColorDepth(16);
    Serial.begin(115200);
    delay(100);
    Serial.println("M5Unified initialized");

    lv_init();
    Serial.println("LVGL initialized");

    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * 10);
    Serial.println("Display buffer initialized");

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    Serial.println("Display driver registered");

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_drv_register(&indev_drv);
    Serial.println("Touch driver registered");

    initProperties();
    ArduinoCloud.begin(ArduinoIoTPreferredConnection, false);
    setDebugMessageLevel(2);
    Serial.println("Arduino Cloud initialization started");

    // Initialize button style and UI after cloud setup
    initButtonStyle();
    Serial.println("Button style initialized");
    createMainMenu();
    Serial.println("Main menu displayed");
}

void loop() {
    M5.update();
    ArduinoCloud.update();
    updateWifiIndicator();
    lv_timer_handler();
    delay(5);
}

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    M5.Display.startWrite();
    M5.Display.setAddrWindow(area->x1, area->y1, w, h);
    M5.Display.pushColors((uint16_t *)&color_p->full, w * h, true);
    M5.Display.endWrite();
    lv_disp_flush_ready(disp);
}

void my_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    static int16_t last_x = 0;
    static int16_t last_y = 0;

    if (M5.Touch.getCount() > 0) {
        auto t = M5.Touch.getDetail();
        if (t.state == m5::touch_state_t::touch || 
            t.state == m5::touch_state_t::hold || 
            t.state == m5::touch_state_t::drag) {
            last_x = t.x;
            last_y = t.y;
            data->point.x = last_x;
            data->point.y = last_y;
            data->state = LV_INDEV_STATE_PR;
            Serial.printf("Touch at: %d, %d, State: %d\n", last_x, last_y, (int)t.state);
        }
    } else {
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
    }
}

void createMainMenu() {
    mainScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(mainScreen, lv_color_hex(0x1A1A1A), 0);
    lv_obj_set_style_bg_grad_color(mainScreen, lv_color_hex(0x404040), 0);
    lv_obj_set_style_bg_grad_dir(mainScreen, LV_GRAD_DIR_VER, 0);
    lv_scr_load(mainScreen);

    lv_obj_t *btnNew = lv_btn_create(mainScreen);
    lv_obj_add_style(btnNew, &btn_style, 0);
    lv_obj_set_size(btnNew, 220, 50);
    lv_obj_align(btnNew, LV_ALIGN_CENTER, 0, -40);
    lv_obj_t *labelNew = lv_label_create(btnNew);
    lv_label_set_text(labelNew, "Create New Entry");
    lv_label_set_long_mode(labelNew, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(labelNew, 180);
    lv_obj_center(labelNew);
    lv_obj_add_event_cb(btnNew, [](lv_event_t *e) { createGenderMenu(); }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btnView = lv_btn_create(mainScreen);
    lv_obj_add_style(btnView, &btn_style, 0);
    lv_obj_set_size(btnView, 220, 50);
    lv_obj_align(btnView, LV_ALIGN_CENTER, 0, 40);
    lv_obj_t *labelView = lv_label_create(btnView);
    lv_label_set_text(labelView, "View Latest Entries");
    lv_label_set_long_mode(labelView, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(labelView, 180);
    lv_obj_center(labelView);

    wifiIndicator = lv_label_create(mainScreen);
    lv_obj_set_style_text_font(wifiIndicator, &lv_font_unscii_16, 0);
    lv_obj_align(wifiIndicator, LV_ALIGN_TOP_RIGHT, -10, 10);
    updateWifiIndicator();
}

void updateWifiIndicator() {
    if (ArduinoCloud.connected()) {
        lv_label_set_text(wifiIndicator, "WiFi: ON");
        lv_obj_set_style_text_color(wifiIndicator, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(wifiIndicator, "WiFi: OFF");
        lv_obj_set_style_text_color(wifiIndicator, lv_color_hex(0xFF0000), 0);
    }
}

void createGenderMenu() {
    genderMenu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(genderMenu, lv_color_hex(0x1A1A1A), 0);
    lv_scr_load(genderMenu);

    lv_obj_t *title = lv_label_create(genderMenu);
    lv_label_set_text(title, "Select Gender");
    lv_obj_set_style_text_font(title, &lv_font_unscii_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = lv_btn_create(genderMenu);
        lv_obj_add_style(btn, &btn_style, 0);
        lv_obj_set_size(btn, 160, 50);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 40 + i * 60);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, genders[i]);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, 140);
        lv_obj_center(label);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            currentEntry = String(lv_label_get_text(lv_obj_get_child(lv_event_get_target(e), 0))) + ",";
            createColorMenu("Shirt Color", &shirtMenu, [](){ createColorMenu("Pants Color", &pantsMenu, [](){ createColorMenu("Shoe Color", &shoeMenu, createItemMenu); }); });
        }, LV_EVENT_CLICKED, NULL);
    }
}

void createColorMenu(const char* title, lv_obj_t** menu, void (*nextMenuFunc)()) {
    *menu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(*menu, lv_color_hex(0x1A1A1A), 0);
    lv_scr_load(*menu);

    lv_obj_t *lblTitle = lv_label_create(*menu);
    lv_label_set_text(lblTitle, title);
    lv_obj_set_style_text_font(lblTitle, &lv_font_unscii_16, 0);
    lv_obj_align(lblTitle, LV_ALIGN_TOP_MID, 0, 10);

    for (int i = 0; i < 9; i++) {
        lv_obj_t *btn = lv_btn_create(*menu);
        lv_obj_add_style(btn, &btn_style, 0);
        lv_obj_set_size(btn, 100, 40);
        lv_obj_set_pos(btn, (i % 3) * 105 + 5, (i / 3) * 50 + 40);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, colors[i]);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(lbl, 80);
        lv_obj_center(lbl);
        lv_obj_add_event_cb(btn, colorMenuCallback, LV_EVENT_CLICKED, (void*)nextMenuFunc);
    }
}

void colorMenuCallback(lv_event_t *e) {
    currentEntry += String(lv_label_get_text(lv_obj_get_child(lv_event_get_target(e), 0))) + ",";
    void (*nextMenuFunc)() = (void (*)())lv_event_get_user_data(e);
    if (nextMenuFunc) nextMenuFunc();
}

void createItemMenu() {
    itemMenu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(itemMenu, lv_color_hex(0x1A1A1A), 0);
    lv_scr_load(itemMenu);

    lv_obj_t *title = lv_label_create(itemMenu);
    lv_label_set_text(title, "Select Item");
    lv_obj_set_style_text_font(title, &lv_font_unscii_16, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    for (int i = 0; i < 7; i++) {
        lv_obj_t *btn = lv_btn_create(itemMenu);
        lv_obj_add_style(btn, &btn_style, 0);
        lv_obj_set_size(btn, 150, 40);
        lv_obj_set_pos(btn, (i % 2) * 155 + 5, (i / 2) * 50 + 40);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, items[i]);
        lv_label_set_long_mode(label, LV_LABEL_LONG_SCROLL_CIRCULAR);
        lv_obj_set_width(label, 130);
        lv_obj_center(label);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            currentEntry += String(lv_label_get_text(lv_obj_get_child(lv_event_get_target(e), 0)));
            createConfirmScreen();
        }, LV_EVENT_CLICKED, NULL);
    }
}

void createConfirmScreen() {
    confirmScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(confirmScreen, lv_color_hex(0x1A1A1A), 0);
    lv_scr_load(confirmScreen);

    lv_obj_t *label = lv_label_create(confirmScreen);
    lv_label_set_text(label, getFormattedEntry(currentEntry).c_str());
    lv_obj_set_style_text_font(label, &lv_font_unscii_16, 0);
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10); // Left-aligned for readability

    lv_obj_t *btnConfirm = lv_btn_create(confirmScreen);
    lv_obj_add_style(btnConfirm, &btn_style, 0);
    lv_obj_set_size(btnConfirm, 120, 40);
    lv_obj_align(btnConfirm, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_t *lblConfirm = lv_label_create(btnConfirm);
    lv_label_set_text(lblConfirm, "Confirm");
    lv_obj_center(lblConfirm);
    lv_obj_add_event_cb(btnConfirm, [](lv_event_t *e) {
        saveEntry(getFormattedEntry(currentEntry));
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btnCancel = lv_btn_create(confirmScreen);
    lv_obj_add_style(btnCancel, &btn_style, 0);
    lv_obj_set_size(btnCancel, 120, 40);
    lv_obj_align(btnCancel, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_t *lblCancel = lv_label_create(btnCancel);
    lv_label_set_text(lblCancel, "Cancel");
    lv_obj_center(lblCancel);
    lv_obj_add_event_cb(btnCancel, [](lv_event_t *e) { createMainMenu(); }, LV_EVENT_CLICKED, NULL);
}

String getTimestamp() {
    if (ArduinoCloud.connected()) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return String(buffer);
    } else {
        return "NoTime";
    }
}

String getFormattedEntry(const String& entry) {
    // Split the comma-separated entry
    String parts[5];
    int partCount = 0;
    int startIdx = 0;
    for (int i = 0; i < entry.length() && partCount < 5; i++) {
        if (entry.charAt(i) == ',') {
            parts[partCount++] = entry.substring(startIdx, i);
            startIdx = i + 1;
        }
    }
    if (startIdx < entry.length()) {
        parts[partCount++] = entry.substring(startIdx);
    }

    // Build formatted string
    String formatted = "Time/Date/2025: " + getTimestamp() + "\n";
    formatted += "Gender: " + (partCount > 0 ? parts[0] : "N/A") + "\n";
    formatted += "Shirt Color: " + (partCount > 1 ? parts[1] : "N/A") + "\n";
    formatted += "Pants Color: " + (partCount > 2 ? parts[2] : "N/A") + "\n";
    formatted += "Shoe Color: " + (partCount > 3 ? parts[3] : "N/A") + "\n";
    formatted += "What to be believe stolen: " + (partCount > 4 ? parts[4] : "N/A");
    return formatted;
}

void saveEntry(const String& entry) {
    logEntry = entry;
    Serial.println("Entry sent to Arduino Cloud:\n" + entry);
    if (!ArduinoCloud.connected()) {
        Serial.println("Cloud not connected, entry logged locally:\n" + entry);
    }
}