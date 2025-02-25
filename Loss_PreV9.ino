#include <Wire.h>
#include <M5CoreS3.h>
#include "lv_conf.h"
#include <lvgl.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <WiFi.h>
#include <Preferences.h>

// Debug flag - set to true to enable debug output
#define DEBUG_ENABLED true

// Debug macros
#define DEBUG_PRINT(x) if(DEBUG_ENABLED) { Serial.print(millis()); Serial.print(": "); Serial.println(x); }
#define DEBUG_PRINTF(x, ...) if(DEBUG_ENABLED) { Serial.print(millis()); Serial.print(": "); Serial.printf(x, __VA_ARGS__); }

// Pin Definitions
#define KEY_PIN 8  // M5Stack Key UNIT

// Global variables for scrolling
lv_obj_t *current_scroll_obj = nullptr;
const int SCROLL_AMOUNT = 40;  // Pixels to scroll per button press

// Declare the Montserrat font
LV_FONT_DECLARE(lv_font_montserrat_14);
LV_FONT_DECLARE(lv_font_montserrat_16);
LV_FONT_DECLARE(lv_font_montserrat_20);

// Screen dimensions for CoreS3
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Display buffer
static lv_color_t buf1[SCREEN_WIDTH * 20];  // Increased buffer size

// Arduino Cloud credentials
const char DEVICE_LOGIN_NAME[] = "45444c7a-9c6a-42ed-8c94-413e098a2b3d";
const char SSID[] = "Wack House";
const char PASS[] = "justice69";
const char DEVICE_KEY[] = "mIB9Ezs8tfWtqe8Mx2qhQgYxT";

// Cloud variables
String logEntry;
bool cloudPaused = false;
String currentEntry = "";

#define MAX_NETWORKS 10
static char savedSSIDs[MAX_NETWORKS][32];
static int numNetworks = 0;

// Menu options
const char* genders[] = {"Male", "Female"};

const char* shirtColors[] = {
    "White", "Black", "Blue", "Red", "Green", "Yellow", 
    "Pink", "Purple", "Orange", "Grey", "Brown", "Navy",
    "Maroon", "Teal", "Beige", "Burgundy", "Olive", "Lavender"
};

const char* pantsColors[] = {
    "Black", "Blue", "Grey", "Khaki", "Brown", "Navy",
    "White", "Beige", "Olive", "Burgundy", "Tan", "Charcoal",
    "Dark Blue", "Light Grey", "Stone", "Cream"
};

const char* shoeColors[] = {
    "Black", "Brown", "White", "Grey", "Navy", "Tan",
    "Red", "Blue", "Multi-Color", "Gold", "Silver", "Bronze",
    "Beige", "Burgundy", "Pink", "Green"
};

const char* items[] = {"Jewelry", "Women's Shoes", "Men's Shoes", "Cosmetics", "Fragrances", "Home", "Kids"};

// LVGL objects
lv_obj_t *mainScreen = nullptr, *genderMenu = nullptr, *colorMenu = nullptr, *itemMenu = nullptr, *confirmScreen = nullptr;
lv_obj_t *wifiIndicator = nullptr;
lv_obj_t *wifiSettingsScreen = nullptr;

// WiFi UI components
static lv_obj_t* wifi_screen = nullptr;
static lv_obj_t* wifi_list = nullptr;
static lv_obj_t* wifi_keyboard = nullptr;
static lv_obj_t* wifi_status_label = nullptr;
static char selected_ssid[33] = ""; // Max SSID length is 32 characters + null terminator
static char wifi_password[65] = ""; // Max WPA2 password length is 64 characters + null terminator
static Preferences preferences;

// Styles
static lv_style_t style_screen, style_btn, style_btn_pressed, style_title, style_text;

void initStyles() {
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_hex(0x1E1E1E));
    lv_style_set_text_color(&style_screen, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_screen, &lv_font_montserrat_14);

    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 10);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0x00C4B4));
    lv_style_set_bg_grad_color(&style_btn, lv_color_hex(0xFF00FF));
    lv_style_set_bg_grad_dir(&style_btn, LV_GRAD_DIR_HOR);
    lv_style_set_border_width(&style_btn, 0);
    lv_style_set_text_color(&style_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_btn, &lv_font_montserrat_16);
    lv_style_set_pad_all(&style_btn, 10);

    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0xFF00FF));
    lv_style_set_bg_grad_color(&style_btn_pressed, lv_color_hex(0x00C4B4));
    lv_style_set_bg_grad_dir(&style_btn_pressed, LV_GRAD_DIR_HOR);

    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_title, &lv_font_montserrat_20);
    
    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_text, &lv_font_montserrat_14);
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    DEBUG_PRINT("Starting Loss Prevention Log...");
    
    auto cfg = M5.config();
    CoreS3.begin(cfg);
    DEBUG_PRINT("CoreS3 initialized");
    
    CoreS3.Display.setBrightness(255);
    CoreS3.Display.clear();
    CoreS3.Display.setRotation(1);
    CoreS3.Power.begin();
    CoreS3.Power.setChargeCurrent(1000);
    DEBUG_PRINT("Display configured");

    DEBUG_PRINTF("Battery Voltage: %f V\n", CoreS3.Power.getBatteryVoltage());
    DEBUG_PRINTF("Is Charging: %d\n", CoreS3.Power.isCharging());
    DEBUG_PRINTF("Battery Level: %d%%\n", CoreS3.Power.getBatteryLevel());

    lv_init();
    DEBUG_PRINT("LVGL initialized");

    lv_display_t *disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_display_set_buffers(disp, buf1, NULL, SCREEN_WIDTH * 20 * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(disp, my_disp_flush);
    DEBUG_PRINT("Display driver registered");

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, my_touchpad_read);
    DEBUG_PRINT("Touch input driver registered");

    DEBUG_PRINT("Initializing Arduino Cloud connection...");
    ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
    ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
    ArduinoCloud.addProperty(logEntry, READWRITE, ON_CHANGE);
    ArduinoCloud.begin(ArduinoIoTPreferredConnection, false);
    setDebugMessageLevel(2);
    DEBUG_PRINT("Arduino Cloud initialized");

    initStyles();
    DEBUG_PRINT("UI styles initialized");
    createMainMenu();
    DEBUG_PRINT("Main menu created");
    
    DEBUG_PRINTF("Free heap after setup: %u bytes\n", ESP.getFreeHeap());
    DEBUG_PRINT("Setup complete!");
}

void loop() {
    CoreS3.update();
    if (!cloudPaused) ArduinoCloud.update();
    updateWifiIndicator();
    lv_timer_handler();
    delay(5);
}

void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    CoreS3.Display.startWrite();
    CoreS3.Display.setAddrWindow(area->x1, area->y1, w, h);
    CoreS3.Display.pushColors((uint16_t *)color_p, w * h);
    CoreS3.Display.endWrite();
    lv_display_flush_ready(disp);
}

void my_touchpad_read(lv_indev_t *indev, lv_indev_data_t *data) {
    auto t = CoreS3.Touch.getDetail();
    if (t.state == m5::touch_state_t::touch) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = t.x;
        data->point.y = t.y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void createMainMenu() {
    if (mainScreen) {
        DEBUG_PRINT("Cleaning existing main screen");
        lv_obj_delete(mainScreen);
        mainScreen = nullptr;
    }
    mainScreen = lv_obj_create(NULL);
    lv_obj_add_style(mainScreen, &style_screen, 0);
    lv_screen_load(mainScreen);
    DEBUG_PRINTF("Main screen created: %p\n", mainScreen);
    addWifiIndicator(mainScreen);

    lv_obj_t *header = lv_obj_create(mainScreen);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Loss Prevention");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_new = lv_button_create(mainScreen);
    lv_obj_set_size(btn_new, 280, 60);
    lv_obj_align(btn_new, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_add_style(btn_new, &style_btn, 0);
    lv_obj_add_style(btn_new, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *label_new = lv_label_create(btn_new);
    lv_label_set_text(label_new, "New Entry");
    lv_obj_center(label_new);
    lv_obj_add_event_cb(btn_new, [](lv_event_t *e) { createGenderMenu(); }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_wifi = lv_button_create(mainScreen);
    lv_obj_set_size(btn_wifi, 280, 60);
    lv_obj_align(btn_wifi, LV_ALIGN_TOP_MID, 0, 130);
    lv_obj_add_style(btn_wifi, &style_btn, 0);
    lv_obj_add_style(btn_wifi, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *label_wifi = lv_label_create(btn_wifi);
    lv_label_set_text(label_wifi, "Wi-Fi Settings");
    lv_obj_center(label_wifi);
    lv_obj_add_event_cb(btn_wifi, [](lv_event_t *e) { createWiFiScreen(); }, LV_EVENT_CLICKED, NULL);
}

void updateWifiIndicator() {
    if (!wifiIndicator) return;
    if (WiFi.status() == WL_CONNECTED) {
        lv_label_set_text(wifiIndicator, LV_SYMBOL_WIFI);
        lv_obj_set_style_text_color(wifiIndicator, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(wifiIndicator, LV_SYMBOL_CLOSE);
        lv_obj_set_style_text_color(wifiIndicator, lv_color_hex(0xFF0000), 0);
    }
    lv_obj_move_foreground(wifiIndicator);
    lv_obj_invalidate(wifiIndicator);
}

void addWifiIndicator(lv_obj_t *screen) {
    if (wifiIndicator != nullptr) {
        lv_obj_delete(wifiIndicator);
        wifiIndicator = nullptr;
    }
    wifiIndicator = lv_label_create(screen);
    lv_obj_set_style_text_font(wifiIndicator, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(wifiIndicator, LV_TEXT_ALIGN_RIGHT, 0);
    lv_obj_align(wifiIndicator, LV_ALIGN_TOP_RIGHT, -10, 5);
    lv_obj_set_style_bg_opa(wifiIndicator, LV_OPA_TRANSP, 0);
    lv_obj_set_style_text_color(wifiIndicator, lv_color_hex(0x00FF00), 0);
    lv_obj_set_style_pad_all(wifiIndicator, 0, 0);
    lv_obj_set_size(wifiIndicator, 32, 32);
    updateWifiIndicator();
}

void createGenderMenu() {
    if (genderMenu) {
        DEBUG_PRINT("Cleaning existing gender menu");
        lv_obj_delete(genderMenu);
        genderMenu = nullptr;
    }
    genderMenu = lv_obj_create(NULL);
    lv_obj_add_style(genderMenu, &style_screen, 0);
    lv_screen_load(genderMenu);
    DEBUG_PRINTF("Gender menu created: %p\n", genderMenu);
    addWifiIndicator(genderMenu);

    lv_obj_t *header = lv_obj_create(genderMenu);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Select Gender");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    for (int i = 0; i < 2; i++) {
        lv_obj_t *btn = lv_button_create(genderMenu);
        lv_obj_set_size(btn, 280, 80);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 60 + i * 90);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, genders[i]);
        lv_obj_center(label);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t *target = lv_event_get_target(e);
            currentEntry = String(lv_label_get_text(lv_obj_get_child(target, 0))) + ",";
            delay(100);
            createColorMenuShirt();
        }, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_t *back_btn = lv_button_create(genderMenu);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
        delay(100);
        DEBUG_PRINT("Returning to main menu");
        createMainMenu();
        if (genderMenu && genderMenu != lv_screen_active()) {
            DEBUG_PRINTF("Cleaning old gender menu: %p\n", genderMenu);
            lv_obj_delete_async(genderMenu);
            genderMenu = nullptr;
        }
    }, LV_EVENT_CLICKED, NULL);
}

void createColorMenuShirt() {
    DEBUG_PRINT("Creating Shirt Color Menu");
    
    lv_obj_t* newMenu = lv_obj_create(NULL);
    lv_obj_add_style(newMenu, &style_screen, 0);
    DEBUG_PRINTF("New color menu created: %p\n", newMenu);
    addWifiIndicator(newMenu);

    lv_obj_t *header = lv_obj_create(newMenu);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Select Shirt Color");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *list = lv_obj_create(newMenu);
    lv_obj_set_size(list, 300, 150);
    lv_obj_align(list, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(list, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(list, 10, 0);
    lv_obj_set_style_pad_row(list, 5, 0);
    lv_obj_set_scroll_dir(list, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(list, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_flag(list, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(list, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_set_flex_flow(list, LV_FLEX_FLOW_COLUMN);
    current_scroll_obj = list;

    static lv_style_t style_chip;
    lv_style_init(&style_chip);
    lv_style_set_bg_color(&style_chip, lv_color_hex(0x4A4A4A));
    lv_style_set_border_width(&style_chip, 1);
    lv_style_set_border_color(&style_chip, lv_color_hex(0xFFFFFF));
    lv_style_set_radius(&style_chip, 5);
    lv_style_set_pad_all(&style_chip, 5);
    lv_style_set_text_font(&style_chip, &lv_font_montserrat_14);

    static lv_style_t style_selected;
    lv_style_init(&style_selected);
    lv_style_set_bg_color(&style_selected, lv_color_hex(0xFF4444));
    lv_style_set_border_color(&style_selected, lv_color_hex(0xFFFFFF));

    int numShirtColors = sizeof(shirtColors) / sizeof(shirtColors[0]);
    DEBUG_PRINTF("Creating %d shirt color chips\n", numShirtColors);

    String selectedColors = "";

    for (int i = 0; i < numShirtColors; i++) {
        lv_obj_t *chip = lv_obj_create(list);
        lv_obj_set_size(chip, 280, 35);
        lv_obj_add_style(chip, &style_chip, 0);
        lv_obj_set_user_data(chip, (void*)shirtColors[i]);
        
        lv_obj_t *label = lv_label_create(chip);
        lv_label_set_text(label, shirtColors[i]);
        lv_obj_center(label);

        lv_obj_add_event_cb(chip, [](lv_event_t *e) {
            lv_obj_t *chip = lv_event_get_target(e);
            const char* color = static_cast<const char*>(lv_obj_get_user_data(chip));
            bool is_selected = lv_obj_has_state(chip, LV_STATE_USER_1);

            static String selectedColors;

            if (!is_selected) {
                lv_obj_add_state(chip, LV_STATE_USER_1);
                lv_obj_add_style(chip, &style_selected, LV_STATE_USER_1);
                if (!selectedColors.isEmpty()) selectedColors += "+";
                selectedColors += color;
            } else {
                lv_obj_clear_state(chip, LV_STATE_USER_1);
                lv_obj_remove_style(chip, &style_selected, LV_STATE_USER_1);
                int pos = selectedColors.indexOf(color);
                if (pos >= 0) {
                    int nextPlus = selectedColors.indexOf("+", pos);
                    if (nextPlus >= 0) {
                        selectedColors.remove(pos, nextPlus - pos + 1);
                    } else {
                        if (pos > 0) selectedColors.remove(pos - 1, String(color).length() + 1);
                        else selectedColors.remove(pos, String(color).length());
                    }
                }
            }
            
            DEBUG_PRINTF("Selected shirt colors: %s\n", selectedColors.c_str());
            
            if (!selectedColors.isEmpty()) {
                static lv_obj_t* next_btn = nullptr;
                if (next_btn == nullptr) {
                    next_btn = lv_button_create(lv_obj_get_parent(lv_obj_get_parent(chip)));
                    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
                    lv_obj_set_size(next_btn, 100, 40);
                    lv_obj_add_style(next_btn, &style_btn, 0);
                    lv_obj_add_style(next_btn, &style_btn_pressed, LV_STATE_PRESSED);
                    
                    lv_obj_t* next_label = lv_label_create(next_btn);
                    lv_label_set_text(next_label, "Next " LV_SYMBOL_RIGHT);
                    lv_obj_center(next_label);
                    
                    lv_obj_add_event_cb(next_btn, [](lv_event_t *e) {
                        static String selectedColors;
                        currentEntry += selectedColors + ",";
                        DEBUG_PRINTF("Current entry: %s\n", currentEntry.c_str());
                        delay(100);
                        DEBUG_PRINT("Transitioning to pants menu");
                        createColorMenuPants();
                        if (colorMenu && colorMenu != lv_screen_active()) {
                            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
                            lv_obj_delete_async(colorMenu);
                            colorMenu = nullptr;
                        }
                        DEBUG_PRINT("Shirt menu transition complete");
                    }, LV_EVENT_CLICKED, NULL);
                }
            }
        }, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_t *back_btn = lv_button_create(newMenu);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
        delay(100);
        DEBUG_PRINT("Returning to gender menu");
        createGenderMenu();
        if (colorMenu && colorMenu != lv_screen_active()) {
            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
            lv_obj_delete_async(colorMenu);
            colorMenu = nullptr;
        }
    }, LV_EVENT_CLICKED, NULL);
    
    lv_screen_load(newMenu);
    
    if (colorMenu && colorMenu != newMenu) {
        DEBUG_PRINTF("Cleaning existing color menu: %p\n", colorMenu);
        lv_obj_delete_async(colorMenu);
    }
    
    colorMenu = newMenu;
}

void createColorMenuPants() {
    DEBUG_PRINT("Creating Pants Color Menu");
    
    lv_obj_t* newMenu = lv_obj_create(NULL);
    lv_obj_add_style(newMenu, &style_screen, 0);
    DEBUG_PRINTF("New color menu created: %p\n", newMenu);
    addWifiIndicator(newMenu);

    lv_obj_t *header = lv_obj_create(newMenu);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Select Pants Color");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *container = lv_obj_create(newMenu);
    lv_obj_set_size(container, 300, 150);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(container, 5, 0);
    lv_obj_set_style_pad_row(container, 8, 0);
    lv_obj_set_style_pad_column(container, 8, 0);
    
    static const lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    
    lv_obj_set_grid_dsc_array(container, col_dsc, row_dsc);
    lv_obj_set_layout(container, LV_LAYOUT_GRID);
    lv_obj_set_scroll_dir(container, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(container, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    current_scroll_obj = container;

    static lv_style_t style_selected;
    lv_style_init(&style_selected);
    lv_style_set_bg_color(&style_selected, lv_color_hex(0xFF4444));
    lv_style_set_bg_grad_color(&style_selected, lv_color_hex(0xFF8888));
    lv_style_set_bg_grad_dir(&style_selected, LV_GRAD_DIR_HOR);

    int numPantsColors = sizeof(pantsColors) / sizeof(pantsColors[0]);
    DEBUG_PRINTF("Creating %d pants color buttons\n", numPantsColors);

    String selectedColors = "";

    for (int i = 0; i < numPantsColors; i++) {
        lv_obj_t *btn = lv_button_create(container);
        lv_obj_set_size(btn, 90, 30);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 3, 1, LV_GRID_ALIGN_STRETCH, i / 3, 1);
        lv_obj_set_user_data(btn, (void*)i);
        
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, pantsColors[i]);
        lv_obj_center(label);

        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t *btn = lv_event_get_target(e);
            int idx = reinterpret_cast<int>(lv_obj_get_user_data(btn));
            bool is_selected = lv_obj_has_state(btn, LV_STATE_USER_1);

            static String selectedColors;

            if (!is_selected) {
                lv_obj_add_state(btn, LV_STATE_USER_1);
                lv_obj_add_style(btn, &style_selected, LV_STATE_USER_1);
                if (!selectedColors.isEmpty()) selectedColors += "+";
                selectedColors += pantsColors[idx];
            } else {
                lv_obj_clear_state(btn, LV_STATE_USER_1);
                lv_obj_remove_style(btn, &style_selected, LV_STATE_USER_1);
                int pos = selectedColors.indexOf(pantsColors[idx]);
                if (pos >= 0) {
                    int nextPlus = selectedColors.indexOf("+", pos);
                    if (nextPlus >= 0) {
                        selectedColors.remove(pos, nextPlus - pos + 1);
                    } else {
                        if (pos > 0) selectedColors.remove(pos - 1, String(pantsColors[idx]).length() + 1);
                        else selectedColors.remove(pos, String(pantsColors[idx]).length());
                    }
                }
            }
            
            DEBUG_PRINTF("Selected pants colors: %s\n", selectedColors.c_str());
            
            if (!selectedColors.isEmpty()) {
                static lv_obj_t* next_btn = nullptr;
                if (next_btn == nullptr) {
                    next_btn = lv_button_create(lv_obj_get_parent(lv_obj_get_parent(btn)));
                    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
                    lv_obj_set_size(next_btn, 100, 40);
                    lv_obj_add_style(next_btn, &style_btn, 0);
                    lv_obj_add_style(next_btn, &style_btn_pressed, LV_STATE_PRESSED);
                    
                    lv_obj_t* next_label = lv_label_create(next_btn);
                    lv_label_set_text(next_label, "Next " LV_SYMBOL_RIGHT);
                    lv_obj_center(next_label);
                    
                    lv_obj_add_event_cb(next_btn, [](lv_event_t* e) {
                        static String selectedColors;
                        currentEntry += selectedColors + ",";
                        DEBUG_PRINTF("Current entry: %s\n", currentEntry.c_str());
                        delay(100);
                        DEBUG_PRINT("Transitioning to shoes menu");
                        createColorMenuShoes();
                        if (colorMenu && colorMenu != lv_screen_active()) {
                            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
                            lv_obj_delete_async(colorMenu);
                            colorMenu = nullptr;
                        }
                        DEBUG_PRINT("Pants menu transition complete");
                    }, LV_EVENT_CLICKED, NULL);
                }
            }
        }, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_t *back_btn = lv_button_create(newMenu);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
        delay(100);
        DEBUG_PRINT("Returning to shirt menu");
        createColorMenuShirt();
        if (colorMenu && colorMenu != lv_screen_active()) {
            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
            lv_obj_delete_async(colorMenu);
            colorMenu = nullptr;
        }
    }, LV_EVENT_CLICKED, NULL);
    
    lv_screen_load(newMenu);
    
    if (colorMenu && colorMenu != newMenu) {
        DEBUG_PRINTF("Cleaning existing color menu: %p\n", colorMenu);
        lv_obj_delete_async(colorMenu);
    }
    
    colorMenu = newMenu;
}

void createColorMenuShoes() {
    DEBUG_PRINT("Creating Shoes Color Menu");
    
    lv_obj_t* newMenu = lv_obj_create(NULL);
    lv_obj_add_style(newMenu, &style_screen, 0);
    DEBUG_PRINTF("New color menu created: %p\n", newMenu);
    addWifiIndicator(newMenu);

    lv_obj_t *header = lv_obj_create(newMenu);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Select Shoes Color");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *container = lv_obj_create(newMenu);
    lv_obj_set_size(container, 300, 150);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(container, 5, 0);
    lv_obj_set_style_pad_row(container, 8, 0);
    lv_obj_set_style_pad_column(container, 8, 0);
    
    static const lv_coord_t col_dsc[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
    static const lv_coord_t row_dsc[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
    
    lv_obj_set_grid_dsc_array(container, col_dsc, row_dsc);
    lv_obj_set_layout(container, LV_LAYOUT_GRID);
    lv_obj_set_scroll_dir(container, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(container, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLLABLE);
    current_scroll_obj = container;

    static lv_style_t style_selected;
    lv_style_init(&style_selected);
    lv_style_set_bg_color(&style_selected, lv_color_hex(0xFF4444));
    lv_style_set_bg_grad_color(&style_selected, lv_color_hex(0xFF8888));
    lv_style_set_bg_grad_dir(&style_selected, LV_GRAD_DIR_HOR);

    int numShoeColors = sizeof(shoeColors) / sizeof(shoeColors[0]);
    DEBUG_PRINTF("Creating %d shoe color buttons\n", numShoeColors);

    String selectedColors = "";

    for (int i = 0; i < numShoeColors; i++) {
        lv_obj_t *btn = lv_button_create(container);
        lv_obj_set_size(btn, 90, 30);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 3, 1, LV_GRID_ALIGN_STRETCH, i / 3, 1);
        lv_obj_set_user_data(btn, (void*)i);
        
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, shoeColors[i]);
        lv_obj_center(label);

        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t *btn = lv_event_get_target(e);
            int idx = reinterpret_cast<int>(lv_obj_get_user_data(btn));
            bool is_selected = lv_obj_has_state(btn, LV_STATE_USER_1);

            static String selectedColors;

            if (!is_selected) {
                lv_obj_add_state(btn, LV_STATE_USER_1);
                lv_obj_add_style(btn, &style_selected, LV_STATE_USER_1);
                if (!selectedColors.isEmpty()) selectedColors += "+";
                selectedColors += shoeColors[idx];
            } else {
                lv_obj_clear_state(btn, LV_STATE_USER_1);
                lv_obj_remove_style(btn, &style_selected, LV_STATE_USER_1);
                int pos = selectedColors.indexOf(shoeColors[idx]);
                if (pos >= 0) {
                    int nextPlus = selectedColors.indexOf("+", pos);
                    if (nextPlus >= 0) {
                        selectedColors.remove(pos, nextPlus - pos + 1);
                    } else {
                        if (pos > 0) selectedColors.remove(pos - 1, String(shoeColors[idx]).length() + 1);
                        else selectedColors.remove(pos, String(shoeColors[idx]).length());
                    }
                }
            }
            
            DEBUG_PRINTF("Selected shoe colors: %s\n", selectedColors.c_str());
            
            if (!selectedColors.isEmpty()) {
                static lv_obj_t* next_btn = nullptr;
                if (next_btn == nullptr) {
                    next_btn = lv_button_create(lv_obj_get_parent(lv_obj_get_parent(btn)));
                    lv_obj_align(next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
                    lv_obj_set_size(next_btn, 100, 40);
                    lv_obj_add_style(next_btn, &style_btn, 0);
                    lv_obj_add_style(next_btn, &style_btn_pressed, LV_STATE_PRESSED);
                    
                    lv_obj_t* next_label = lv_label_create(next_btn);
                    lv_label_set_text(next_label, "Next " LV_SYMBOL_RIGHT);
                    lv_obj_center(next_label);
                    
                    lv_obj_add_event_cb(next_btn, [](lv_event_t* e) {
                        static String selectedColors;
                        currentEntry += selectedColors + ",";
                        DEBUG_PRINTF("Current entry: %s\n", currentEntry.c_str());
                        delay(100);
                        DEBUG_PRINT("Transitioning to item menu");
                        createItemMenu();
                        if (colorMenu && colorMenu != lv_screen_active()) {
                            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
                            lv_obj_delete_async(colorMenu);
                            colorMenu = nullptr;
                        }
                        DEBUG_PRINT("Shoes menu transition complete");
                    }, LV_EVENT_CLICKED, NULL);
                }
            }
        }, LV_EVENT_CLICKED, NULL);
    }

    lv_obj_t *back_btn = lv_button_create(newMenu);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
        delay(100);
        DEBUG_PRINT("Returning to pants menu");
        createColorMenuPants();
        if (colorMenu && colorMenu != lv_screen_active()) {
            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
            lv_obj_delete_async(colorMenu);
            colorMenu = nullptr;
        }
    }, LV_EVENT_CLICKED, NULL);
    
    lv_screen_load(newMenu);
    
    if (colorMenu && colorMenu != newMenu) {
        DEBUG_PRINTF("Cleaning existing color menu: %p\n", colorMenu);
        lv_obj_delete_async(colorMenu);
    }
    
    colorMenu = newMenu;
}

void createItemMenu() {
    if (itemMenu) {
        DEBUG_PRINT("Cleaning existing item menu");
        lv_obj_delete(itemMenu);
        itemMenu = nullptr;
    }
    itemMenu = lv_obj_create(NULL);
    lv_obj_add_style(itemMenu, &style_screen, 0);
    lv_screen_load(itemMenu);
    DEBUG_PRINTF("Item menu created: %p\n", itemMenu);
    addWifiIndicator(itemMenu);

    lv_obj_t *header = lv_obj_create(itemMenu);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Select Item");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *list_cont = lv_obj_create(itemMenu);
    lv_obj_set_size(list_cont, 280, 180);
    lv_obj_align(list_cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(list_cont, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(list_cont, 5, 0);
    lv_obj_set_scroll_dir(list_cont, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(list_cont, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(list_cont, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_clear_flag(list_cont, LV_OBJ_FLAG_SCROLL_ELASTIC);
    lv_obj_add_flag(list_cont, LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_add_flag(list_cont, LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 7; i++) {
        lv_obj_t *btn = lv_button_create(list_cont);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_set_height(btn, 40);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, items[i]);
        lv_obj_center(label);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t *target = lv_event_get_target(e);
            currentEntry += String(lv_label_get_text(lv_obj_get_child(target, 0)));
            delay(100);
            createConfirmScreen();
        }, LV_EVENT_CLICKED, NULL);
    }
}

void createConfirmScreen() {
    if (confirmScreen) {
        DEBUG_PRINT("Cleaning existing confirm screen");
        lv_obj_delete(confirmScreen);
        confirmScreen = nullptr;
    }
    confirmScreen = lv_obj_create(NULL);
    lv_obj_add_style(confirmScreen, &style_screen, 0);
    lv_screen_load(confirmScreen);
    DEBUG_PRINTF("Confirm screen created: %p\n", confirmScreen);
    addWifiIndicator(confirmScreen);

    lv_obj_t *header = lv_obj_create(confirmScreen);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Confirm Entry");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *preview = lv_label_create(confirmScreen);
    lv_label_set_text(preview, getFormattedEntry(currentEntry).c_str());
    lv_obj_add_style(preview, &style_text, 0);
    lv_obj_set_size(preview, 280, 140);
    lv_obj_align(preview, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(preview, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_pad_all(preview, 10, 0);

    lv_obj_t *btn_container = lv_obj_create(confirmScreen);
    lv_obj_remove_style_all(btn_container);
    lv_obj_set_size(btn_container, 320, 50);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    lv_obj_t *btn_confirm = lv_button_create(btn_container);
    lv_obj_set_size(btn_confirm, 130, 40);
    lv_obj_add_style(btn_confirm, &style_btn, 0);
    lv_obj_add_style(btn_confirm, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *label_confirm = lv_label_create(btn_confirm);
    lv_label_set_text(label_confirm, "Confirm");
    lv_obj_center(label_confirm);
    lv_obj_add_event_cb(btn_confirm, [](lv_event_t *e) {
        saveEntry(getFormattedEntry(currentEntry));
        currentEntry = "";
        delay(100);
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_cancel = lv_button_create(btn_container);
    lv_obj_set_size(btn_cancel, 130, 40);
    lv_obj_add_style(btn_cancel, &style_btn, 0);
    lv_obj_add_style(btn_cancel, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *label_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(label_cancel, "Cancel");
    lv_obj_center(label_cancel);
    lv_obj_add_event_cb(btn_cancel, [](lv_event_t *e) {
        currentEntry = "";
        delay(100);
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);
}

void scanNetworks() {
    if (wifi_status_label) {
        lv_label_set_text(wifi_status_label, "Scanning for networks...");
    }
    
    int n = WiFi.scanNetworks();
    DEBUG_PRINTF("Found %d networks\n", n);
    
    if (wifi_list) {
        lv_obj_clean(wifi_list);
        
        if (n == 0) {
            lv_label_set_text(wifi_status_label, "No networks found");
        } else {
            lv_label_set_text(wifi_status_label, "Select a network:");
            
            for (int i = 0; i < n; ++i) {
                String ssid = WiFi.SSID(i);
                int rssi = WiFi.RSSI(i);
                String security = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "" : " ";
                
                char list_text[50];
                snprintf(list_text, sizeof(list_text), "%s (%ddBm)%s", 
                    ssid.c_str(), rssi, security.c_str());
                
                lv_obj_t* btn = lv_list_add_button(wifi_list, LV_SYMBOL_WIFI, list_text);
                lv_obj_add_style(btn, &style_btn, 0);
                lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
                
                char* btn_ssid = static_cast<char*>(lv_malloc(33));
                strncpy(btn_ssid, ssid.c_str(), 32);
                btn_ssid[32] = '\0';
                lv_obj_set_user_data(btn, btn_ssid);
                
                lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    lv_obj_t* btn = lv_event_get_target(e);
                    char* ssid = static_cast<char*>(lv_obj_get_user_data(btn));
                    if (ssid) {
                        strncpy(selected_ssid, ssid, 32);
                        selected_ssid[32] = '\0';
                        DEBUG_PRINTF("Selected SSID: %s\n", selected_ssid);
                        showWiFiKeyboard();
                    }
                }, LV_EVENT_CLICKED, NULL);
            }
        }
    }
}

void showWiFiKeyboard() {
    if (wifi_keyboard == nullptr) {
        wifi_keyboard = lv_obj_create(wifi_screen);
        lv_obj_set_size(wifi_keyboard, 320, 240);
        lv_obj_align(wifi_keyboard, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(wifi_keyboard, lv_color_hex(0x1E1E1E), 0);
        
        lv_obj_t* header = lv_obj_create(wifi_keyboard);
        lv_obj_set_size(header, 320, 40);
        lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 0);
        lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
        
        lv_obj_t* header_label = lv_label_create(header);
        lv_label_set_text(header_label, "Enter WiFi Password");
        lv_obj_align(header_label, LV_ALIGN_CENTER, 0, 0);
        
        lv_obj_t* ta = lv_textarea_create(wifi_keyboard);
        lv_obj_set_size(ta, 280, 40);
        lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 50);
        lv_textarea_set_password_mode(ta, true);
        lv_textarea_set_max_length(ta, 64);
        lv_textarea_set_placeholder_text(ta, "Password");
        lv_obj_add_event_cb(ta, [](lv_event_t* e) {
            lv_obj_t* ta = lv_event_get_target(e);
            const char* password = lv_textarea_get_text(ta);
            strncpy(wifi_password, password, 64);
            wifi_password[64] = '\0';
        }, LV_EVENT_VALUE_CHANGED, NULL);
        
        lv_obj_t* kb = lv_keyboard_create(wifi_keyboard);
        lv_obj_set_size(kb, 320, 140);
        lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_keyboard_set_textarea(kb, ta);
        
        lv_obj_t* connect_btn = lv_button_create(wifi_keyboard);
        lv_obj_align(connect_btn, LV_ALIGN_TOP_RIGHT, -10, 50);
        lv_obj_set_size(connect_btn, 80, 40);
        lv_obj_add_style(connect_btn, &style_btn, 0);
        lv_obj_add_style(connect_btn, &style_btn_pressed, LV_STATE_PRESSED);
        
        lv_obj_t* connect_label = lv_label_create(connect_btn);
        lv_label_set_text(connect_label, "Connect");
        lv_obj_center(connect_label);
        
        lv_obj_add_event_cb(connect_btn, [](lv_event_t* e) {
            connectToWiFi();
        }, LV_EVENT_CLICKED, NULL);
        
        lv_obj_t* cancel_btn = lv_button_create(wifi_keyboard);
        lv_obj_align(cancel_btn, LV_ALIGN_TOP_LEFT, 10, 50);
        lv_obj_set_size(cancel_btn, 80, 40);
        lv_obj_add_style(cancel_btn, &style_btn, 0);
        lv_obj_add_style(cancel_btn, &style_btn_pressed, LV_STATE_PRESSED);
        
        lv_obj_t* cancel_label = lv_label_create(cancel_btn);
        lv_label_set_text(cancel_label, "Cancel");
        lv_obj_center(cancel_label);
        
        lv_obj_add_event_cb(cancel_btn, [](lv_event_t* e) {
            if (wifi_keyboard) {
                lv_obj_delete(wifi_keyboard);
                wifi_keyboard = nullptr;
            }
        }, LV_EVENT_CLICKED, NULL);
    }
}

void connectToWiFi() {
    if (strlen(selected_ssid) > 0 && strlen(wifi_password) > 0) {
        DEBUG_PRINTF("Connecting to %s\n", selected_ssid);
        lv_label_set_text(wifi_status_label, "Connecting...");
        
        WiFi.begin(selected_ssid, wifi_password);
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 20) {
            delay(500);
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            DEBUG_PRINTF("Connected to WiFi. IP: %s\n", WiFi.localIP().toString().c_str());
            lv_label_set_text(wifi_status_label, "Connected!");
            
            preferences.begin("wifi", false);
            preferences.putString("ssid", selected_ssid);
            preferences.putString("password", wifi_password);
            preferences.end();
            
            if (wifi_keyboard) {
                lv_obj_delete(wifi_keyboard);
                wifi_keyboard = nullptr;
            }
        } else {
            lv_label_set_text(wifi_status_label, "Connection failed");
            WiFi.disconnect();
        }
    }
}

void createWiFiScreen() {
    if (wifi_screen) {
        lv_obj_delete(wifi_screen);
    }
    
    wifi_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_screen, &style_screen, 0);
    
    lv_obj_t* header = lv_obj_create(wifi_screen);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "WiFi Setup");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    
    wifi_status_label = lv_label_create(wifi_screen);
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_MID, 0, 60);
    lv_label_set_text(wifi_status_label, "Initializing...");
    
    wifi_list = lv_list_create(wifi_screen);
    lv_obj_set_size(wifi_list, 300, 160);
    lv_obj_align(wifi_list, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_color(wifi_list, lv_color_hex(0x2D2D2D), 0);
    
    lv_obj_t* refresh_btn = lv_button_create(wifi_screen);
    lv_obj_align(refresh_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
    lv_obj_set_size(refresh_btn, 100, 40);
    lv_obj_add_style(refresh_btn, &style_btn, 0);
    lv_obj_add_style(refresh_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    lv_obj_t* refresh_label = lv_label_create(refresh_btn);
    lv_label_set_text(refresh_label, LV_SYMBOL_REFRESH " Scan");
    lv_obj_center(refresh_label);
    
    lv_obj_add_event_cb(refresh_btn, [](lv_event_t* e) {
        scanNetworks();
    }, LV_EVENT_CLICKED, NULL);
    
    lv_obj_t* back_btn = lv_button_create(wifi_screen);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);
    
    lv_screen_load(wifi_screen);
    scanNetworks();
}

String getFormattedEntry(const String& entry) {
    String parts[5];
    int partCount = 0, startIdx = 0;
    for (int i = 0; i < entry.length() && partCount < 5; i++) {
        if (entry.charAt(i) == ',') {
            parts[partCount++] = entry.substring(startIdx, i);
            startIdx = i + 1;
        }
    }
    if (startIdx < entry.length()) parts[partCount++] = entry.substring(startIdx);

    String formatted = "Time: " + getTimestamp() + "\n";
    formatted += "Gender: " + (partCount > 0 ? parts[0] : "N/A") + "\n";
    formatted += "Shirt: " + (partCount > 1 ? parts[1] : "N/A") + "\n";
    formatted += "Pants: " + (partCount > 2 ? parts[2] : "N/A") + "\n";
    formatted += "Shoes: " + (partCount > 3 ? parts[3] : "N/A") + "\n";
    formatted += "Item: " + (partCount > 4 ? parts[4] : "N/A");
    return formatted;
}

String getTimestamp() {
    if (WiFi.status() == WL_CONNECTED) {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        char buffer[20];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
        return String(buffer);
    }
    return "NoTime";
}

void saveEntry(const String& entry) {
    logEntry = entry;
    Serial.println("New Entry:");
    Serial.println(entry);
    if (WiFi.status() == WL_CONNECTED && ArduinoCloud.connected()) {
        Serial.println("Sending to Arduino Cloud...");
        delay(100);
    } else {
        Serial.println("Cloud not connected, logged locally only");
    }
}