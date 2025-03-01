// DOWNGRADE TO LVGL 8.4 BEFORE RUNNIGN
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
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_WIDTH * 20];  // Increased buffer size

// Display and input drivers
static lv_disp_drv_t disp_drv;
static lv_indev_drv_t indev_drv;

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

// Global next button pointers
lv_obj_t* shirt_next_btn = nullptr;
lv_obj_t* pants_next_btn = nullptr;
lv_obj_t* shoes_next_btn = nullptr;

const char* shirtColors[] = {
    "Red", "Orange", "Yellow", "Green", "Blue", "Purple", 
    "Black", "White"
};

const char* pantsColors[] = {
    "Black", "Blue", "Grey", "Khaki", "Brown", "Navy",
    "White", "Beige"
};

const char* shoeColors[] = {
    "Black", "Brown", "White", "Grey", "Navy", "Red",
    "Blue", "Green"
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
// Add this new style for keyboard buttons
static lv_style_t style_keyboard_btn;

// Add these global variables and keyboard definitions
static int keyboard_page_index = 0;

// Control map for button matrix
const lv_btnmatrix_ctrl_t keyboard_ctrl_map[] = {
    4, 4, 4, 
    4, 4, 4, 
    4, 4, 4, 
    3, 7, 7, 7  // Changed last row controls for special keys
};

// Keyboard maps for different pages
const char *btnm_mapplus[11][23] = {
    { "a", "b", "c", "\n",
      "d", "e", "f", "\n",
      "g", "h", "i", "\n",
      LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "j", "k", "l", "\n",
      "m", "n", "o", "\n",
      "p", "q", "r", "\n",
      LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "s", "t", "u", "\n",
      "v", "w", "x", "\n",
      "y", "z", " ", "\n",
      LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "A", "B", "C", "\n",
      "D", "E", "F", "\n",
      "G", "H", "I", "\n",
      LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "J", "K", "L", "\n",
      "N", "M", "O", "\n",
      "P", "Q", "R", "\n",
      LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "S", "T", "U", "\n",
      "V", "W", "X", "\n",
      "Y", "Z", " ", "\n",
      LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "1", "2", "3", "\n",
      "4", "5", "6", "\n",
      "7", "8", "9", "\n",
      LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "0", "+", "-", "\n",
      "/", "*", "=", "\n",
      "!", "?", " ", "\n",
      LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "<", ">", "@", "\n",
      "%", "$", "(", "\n",
      ")", "{", "}", "\n",
      LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "[", "]", ";", "\n",
      "\"", "'", ".", "\n",
      ",", ":", " ", "\n",
      LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" },
    { "\\", "_", "~", "\n",
      "|", "&", "^", "\n",
      "`", "#", " ", "\n",
      LV_SYMBOL_OK, LV_SYMBOL_BACKSPACE, LV_SYMBOL_LEFT, LV_SYMBOL_RIGHT, "" }
  };

// Number of keyboard pages
const int NUM_KEYBOARD_PAGES = sizeof(btnm_mapplus) / sizeof(btnm_mapplus[0]);

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
    
    // Initialize keyboard button style
    lv_style_init(&style_keyboard_btn);
    lv_style_set_radius(&style_keyboard_btn, 8);  // Rounded corners
    lv_style_set_border_width(&style_keyboard_btn, 2);  // Border width
    lv_style_set_border_color(&style_keyboard_btn, lv_color_hex(0x888888));  // Border color
    lv_style_set_pad_all(&style_keyboard_btn, 5);  // Inner padding inside the button
    lv_style_set_bg_color(&style_keyboard_btn, lv_color_hex(0x333333));  // Button background color
    lv_style_set_text_color(&style_keyboard_btn, lv_color_hex(0xFFFFFF));  // Text color
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

void setup() {
    Serial.begin(115200);
    while (!Serial) delay(10);
    DEBUG_PRINT("Starting Loss Prevention Log...");
    
    auto cfg = M5.config();
    cfg.output_power = false;
    CoreS3.begin(cfg);
    CoreS3.Power.begin();
    CoreS3.Power.setBatteryCharge(true);
    CoreS3.Power.setChargeCurrent(600);
    DEBUG_PRINT("CoreS3 initialized");
    
    CoreS3.Display.setBrightness(255);
    CoreS3.Display.clear();
    DEBUG_PRINT("Display configured");

    // Add battery status check
    DEBUG_PRINTF("Battery Voltage: %f V\n", CoreS3.Power.getBatteryVoltage());
    DEBUG_PRINTF("Is Charging: %d\n", CoreS3.Power.isCharging());
    DEBUG_PRINTF("Battery Level: %d%%\n", CoreS3.Power.getBatteryLevel());

    lv_init();
    DEBUG_PRINT("LVGL initialized");
    
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * 20);
    DEBUG_PRINT("Display buffer initialized");
    
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = CoreS3.Display.width();
    disp_drv.ver_res = CoreS3.Display.height();
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    DEBUG_PRINT("Display driver registered");

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
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

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    CoreS3.Display.startWrite();
    CoreS3.Display.setAddrWindow(area->x1, area->y1, w, h);
    CoreS3.Display.pushColors((uint16_t *)color_p, w * h);
    CoreS3.Display.endWrite();
    lv_disp_flush_ready(disp);
}

void my_touchpad_read(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    auto t = CoreS3.Touch.getDetail();
    if (t.state == m5::touch_state_t::touch) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = t.x;
        data->point.y = t.y; // Apply vertical offset to fix alignment
        
        // Debug the coordinates if needed
        DEBUG_PRINTF("Touch: (%d, %d) → adjusted to (%d, %d)\n", 
            t.x, t.y, data->point.x, data->point.y);
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void createMainMenu() {
    if (mainScreen) {
        DEBUG_PRINT("Cleaning existing main screen");
        lv_obj_del(mainScreen);
        mainScreen = nullptr;
    }
    mainScreen = lv_obj_create(NULL);
    lv_obj_add_style(mainScreen, &style_screen, 0);
    lv_scr_load(mainScreen);
    DEBUG_PRINTF("Main screen created: %p\n", mainScreen);
    addWifiIndicator(mainScreen);

    lv_obj_t *header = lv_obj_create(mainScreen);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Loss Prevention");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    lv_obj_t *btn_new = lv_btn_create(mainScreen);
    lv_obj_set_size(btn_new, 280, 60);
    lv_obj_align(btn_new, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_add_style(btn_new, &style_btn, 0);
    lv_obj_add_style(btn_new, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *label_new = lv_label_create(btn_new);
    lv_label_set_text(label_new, "New Entry");
    lv_obj_center(label_new);
    lv_obj_add_event_cb(btn_new, [](lv_event_t *e) { createGenderMenu(); }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_wifi = lv_btn_create(mainScreen);
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
        lv_obj_del(wifiIndicator);
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
        lv_obj_del(genderMenu);
        genderMenu = nullptr;
    }
    genderMenu = lv_obj_create(NULL);
    lv_obj_add_style(genderMenu, &style_screen, 0);
    lv_scr_load(genderMenu);
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
        lv_obj_t *btn = lv_btn_create(genderMenu);
        lv_obj_set_size(btn, 280, 80);
        lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 60 + i * 90);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, genders[i]);
        lv_obj_center(label);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            currentEntry = String(lv_label_get_text(lv_obj_get_child(lv_event_get_target(e), 0))) + ",";
            delay(100); // Small delay for stability
            createColorMenuShirt();
        }, LV_EVENT_CLICKED, NULL);
    }

    // Add Back button
    lv_obj_t *back_btn = lv_btn_create(genderMenu);
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
        if (genderMenu && genderMenu != lv_scr_act()) {
            DEBUG_PRINTF("Cleaning old gender menu: %p\n", genderMenu);
            lv_obj_del_async(genderMenu);
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

    // Header
    lv_obj_t *header = lv_obj_create(newMenu);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Select Shirt Color");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Create a container for the color buttons
    lv_obj_t *container = lv_obj_create(newMenu);
    lv_obj_set_size(container, 300, 160);
    lv_obj_align(container, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_pad_all(container, 5, 0);
    lv_obj_set_style_pad_row(container, 5, 0);
    lv_obj_set_style_pad_column(container, 5, 0);

    // Set up grid layout
    static lv_coord_t col_dsc[] = {90, 90, 90, LV_GRID_TEMPLATE_LAST};
    static lv_coord_t row_dsc[] = {40, 40, 40, LV_GRID_TEMPLATE_LAST};
    lv_obj_set_grid_dsc_array(container, col_dsc, row_dsc);
    
    lv_obj_set_scroll_dir(container, LV_DIR_VER);
    lv_obj_set_scrollbar_mode(container, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLLABLE);
    current_scroll_obj = container;

    // Static variable for drag tracking
    static lv_point_t last_point = {0, 0};

    // Drag-to-scroll handlers
    lv_obj_add_event_cb(container, [](lv_event_t *e) {
        lv_indev_t *indev = lv_indev_get_act();
        lv_indev_get_point(indev, &last_point);
    }, LV_EVENT_PRESSED, NULL);

    lv_obj_add_event_cb(container, [](lv_event_t *e) {
        lv_obj_t *container = (lv_obj_t*)lv_event_get_user_data(e);
        lv_indev_t *indev = lv_indev_get_act();
        lv_point_t curr_point;
        lv_indev_get_point(indev, &curr_point);
        lv_coord_t delta_y = last_point.y - curr_point.y;
        lv_obj_scroll_by(container, 0, delta_y, LV_ANIM_OFF);
        last_point = curr_point;
    }, LV_EVENT_PRESSING, container);

    int numShirtColors = sizeof(shirtColors) / sizeof(shirtColors[0]);
    DEBUG_PRINTF("Creating %d shirt color buttons\n", numShirtColors);

    String selectedColors = "";

    for (int i = 0; i < numShirtColors; i++) {
        lv_obj_t *btn = lv_btn_create(container);
        lv_obj_set_size(btn, 90, 40);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_set_grid_cell(btn, LV_GRID_ALIGN_STRETCH, i % 3, 1, LV_GRID_ALIGN_STRETCH, i / 3, 1);
        lv_obj_set_user_data(btn, (void*)i);
        
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, shirtColors[i]);
        lv_obj_center(label);

        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t *btn = lv_event_get_target(e);
            int idx = (int)lv_obj_get_user_data(btn);
            bool is_selected = lv_obj_has_state(btn, LV_STATE_USER_1);

            static String selectedColors;

            if (!is_selected) {
                lv_obj_add_state(btn, LV_STATE_USER_1);
                lv_obj_set_style_border_width(btn, 3, LV_PART_MAIN);
                lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFF00), LV_PART_MAIN);
                if (!selectedColors.isEmpty()) selectedColors += "+";
                selectedColors += shirtColors[idx];
            } else {
                lv_obj_clear_state(btn, LV_STATE_USER_1);
                lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
                lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
                int pos = selectedColors.indexOf(shirtColors[idx]);
                if (pos >= 0) {
                    int nextPlus = selectedColors.indexOf("+", pos);
                    if (nextPlus >= 0) {
                        selectedColors.remove(pos, nextPlus - pos + 1);
                    } else {
                        if (pos > 0) selectedColors.remove(pos - 1, String(shirtColors[idx]).length() + 1);
                        else selectedColors.remove(pos, String(shirtColors[idx]).length());
                    }
                }
            }
            
            DEBUG_PRINTF("Selected shirt colors: %s\n", selectedColors.c_str());
            
            if (!selectedColors.isEmpty()) {
                if (shirt_next_btn == nullptr) {
                    shirt_next_btn = lv_btn_create(lv_obj_get_parent(lv_obj_get_parent(btn)));
                    lv_obj_align(shirt_next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
                    lv_obj_set_size(shirt_next_btn, 100, 40);
                    lv_obj_add_style(shirt_next_btn, &style_btn, 0);
                    lv_obj_add_style(shirt_next_btn, &style_btn_pressed, LV_STATE_PRESSED);
                    
                    lv_obj_t* next_label = lv_label_create(shirt_next_btn);
                    lv_label_set_text(next_label, "Next " LV_SYMBOL_RIGHT);
                    lv_obj_center(next_label);
                    
                    String* selectedColorsPtr = &selectedColors;
                    
                    lv_obj_add_event_cb(shirt_next_btn, [](lv_event_t* e) {
                        String* selectedColorsPtr = (String*)lv_event_get_user_data(e);
                        
                        currentEntry += *selectedColorsPtr + ",";
                        DEBUG_PRINTF("Current entry: %s\n", currentEntry.c_str());
                        delay(100);
                        DEBUG_PRINT("Transitioning to pants menu");
                        createColorMenuPants();
                        if (colorMenu && colorMenu != lv_scr_act()) {
                            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
                            lv_obj_del_async(colorMenu);
                            colorMenu = nullptr;
                        }
                        DEBUG_PRINT("Shirt menu transition complete");
                    }, LV_EVENT_CLICKED, selectedColorsPtr);
                }
            }
        }, LV_EVENT_CLICKED, NULL);
    }

    // Set button colors based on their names
    for (int i = 0; i < numShirtColors; i++) {
        lv_obj_t *btn = lv_obj_get_child(container, i);
        if (btn && lv_obj_check_type(btn, &lv_btn_class)) {
            // Set button background color to match the actual color it represents
            uint32_t color_hex = 0x4A4A4A; // Default gray
            if (strcmp(shirtColors[i], "White") == 0) color_hex = 0xFFFFFF;
            else if (strcmp(shirtColors[i], "Black") == 0) color_hex = 0x000000;
            else if (strcmp(shirtColors[i], "Red") == 0) color_hex = 0xFF0000;
            else if (strcmp(shirtColors[i], "Orange") == 0) color_hex = 0xFFA500;
            else if (strcmp(shirtColors[i], "Yellow") == 0) color_hex = 0xFFFF00;
            else if (strcmp(shirtColors[i], "Green") == 0) color_hex = 0x00FF00;
            else if (strcmp(shirtColors[i], "Blue") == 0) color_hex = 0x0000FF;
            else if (strcmp(shirtColors[i], "Purple") == 0) color_hex = 0x800080;
            
            lv_obj_set_style_bg_color(btn, lv_color_hex(color_hex), 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(color_hex), LV_STATE_PRESSED);
            
            // Adjust text color for visibility based on background
            uint32_t text_color = 0xFFFFFF; // Default white text
            if (strcmp(shirtColors[i], "White") == 0 || 
                strcmp(shirtColors[i], "Yellow") == 0) {
                text_color = 0x000000; // Black text for light backgrounds
            }
            
            lv_obj_t *label = lv_obj_get_child(btn, 0);
            if (label && lv_obj_check_type(label, &lv_label_class)) {
                lv_obj_set_style_text_color(label, lv_color_hex(text_color), 0);
            }
        }
    }

    lv_obj_t *back_btn = lv_btn_create(newMenu);
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
        
        // Reset static next button pointers
        shirt_next_btn = nullptr;
        pants_next_btn = nullptr;
        shoes_next_btn = nullptr;
        
        createGenderMenu();
        if (colorMenu && colorMenu != lv_scr_act()) {
            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
            lv_obj_del_async(colorMenu);
            colorMenu = nullptr;
        }
    }, LV_EVENT_CLICKED, NULL);
    
    lv_scr_load(newMenu);
    
    if (colorMenu && colorMenu != newMenu) {
        DEBUG_PRINTF("Cleaning existing color menu: %p\n", colorMenu);
        lv_obj_del_async(colorMenu);
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
    lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLLABLE);
    current_scroll_obj = container;

    // Static variable for drag tracking
    static lv_point_t last_point = {0, 0};

    // Drag-to-scroll handlers
    lv_obj_add_event_cb(container, [](lv_event_t *e) {
        lv_indev_t *indev = lv_indev_get_act();
        lv_indev_get_point(indev, &last_point);
    }, LV_EVENT_PRESSED, NULL);

    lv_obj_add_event_cb(container, [](lv_event_t *e) {
        lv_obj_t *container = (lv_obj_t*)lv_event_get_user_data(e);
        lv_indev_t *indev = lv_indev_get_act();
        lv_point_t curr_point;
        lv_indev_get_point(indev, &curr_point);
        lv_coord_t delta_y = last_point.y - curr_point.y;
        lv_obj_scroll_by(container, 0, delta_y, LV_ANIM_OFF);
        last_point = curr_point;
    }, LV_EVENT_PRESSING, container);

    int numPantsColors = sizeof(pantsColors) / sizeof(pantsColors[0]);
    DEBUG_PRINTF("Creating %d pants color buttons\n", numPantsColors);

    String selectedColors = "";

    for (int i = 0; i < numPantsColors; i++) {
        lv_obj_t *btn = lv_btn_create(container);
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
            int idx = (int)lv_obj_get_user_data(btn);
            bool is_selected = lv_obj_has_state(btn, LV_STATE_USER_1);

            static String selectedColors;

            if (!is_selected) {
                lv_obj_add_state(btn, LV_STATE_USER_1);
                lv_obj_set_style_border_width(btn, 3, LV_PART_MAIN);
                lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFF00), LV_PART_MAIN);
                if (!selectedColors.isEmpty()) selectedColors += "+";
                selectedColors += pantsColors[idx];
            } else {
                lv_obj_clear_state(btn, LV_STATE_USER_1);
                lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
                lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
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
                if (pants_next_btn == nullptr) {
                    pants_next_btn = lv_btn_create(lv_obj_get_parent(lv_obj_get_parent(btn)));
                    lv_obj_align(pants_next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
                    lv_obj_set_size(pants_next_btn, 100, 40);
                    lv_obj_add_style(pants_next_btn, &style_btn, 0);
                    lv_obj_add_style(pants_next_btn, &style_btn_pressed, LV_STATE_PRESSED);
                    
                    lv_obj_t* next_label = lv_label_create(pants_next_btn);
                    lv_label_set_text(next_label, "Next " LV_SYMBOL_RIGHT);
                    lv_obj_center(next_label);
                    
                    String* selectedColorsPtr = &selectedColors;
                    
                    lv_obj_add_event_cb(pants_next_btn, [](lv_event_t* e) {
                        String* selectedColorsPtr = (String*)lv_event_get_user_data(e);
                        
                        currentEntry += *selectedColorsPtr + ",";
                        DEBUG_PRINTF("Current entry: %s\n", currentEntry.c_str());
                        delay(100);
                        DEBUG_PRINT("Transitioning to shoes menu");
                        createColorMenuShoes();
                        if (colorMenu && colorMenu != lv_scr_act()) {
                            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
                            lv_obj_del_async(colorMenu);
                            colorMenu = nullptr;
                        }
                        DEBUG_PRINT("Pants menu transition complete");
                    }, LV_EVENT_CLICKED, selectedColorsPtr);
                }
            }
        }, LV_EVENT_CLICKED, NULL);
    }

    // Set button colors based on their names
    for (int i = 0; i < numPantsColors; i++) {
        lv_obj_t *btn = lv_obj_get_child(container, i);
        if (btn && lv_obj_check_type(btn, &lv_btn_class)) {
            // Set button background color to match the actual color it represents
            uint32_t color_hex = 0x4A4A4A; // Default gray
            if (strcmp(pantsColors[i], "White") == 0) color_hex = 0xFFFFFF;
            else if (strcmp(pantsColors[i], "Black") == 0) color_hex = 0x000000;
            else if (strcmp(pantsColors[i], "Red") == 0) color_hex = 0xFF0000;
            else if (strcmp(pantsColors[i], "Orange") == 0) color_hex = 0xFFA500;
            else if (strcmp(pantsColors[i], "Yellow") == 0) color_hex = 0xFFFF00;
            else if (strcmp(pantsColors[i], "Green") == 0) color_hex = 0x00FF00;
            else if (strcmp(pantsColors[i], "Blue") == 0) color_hex = 0x0000FF;
            else if (strcmp(pantsColors[i], "Purple") == 0) color_hex = 0x800080;
            else if (strcmp(pantsColors[i], "Brown") == 0) color_hex = 0x8B4513;
            else if (strcmp(pantsColors[i], "Grey") == 0) color_hex = 0x808080;
            else if (strcmp(pantsColors[i], "Tan") == 0) color_hex = 0xD2B48C;
            
            lv_obj_set_style_bg_color(btn, lv_color_hex(color_hex), 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(color_hex), LV_STATE_PRESSED);
            
            // Adjust text color for visibility based on background
            uint32_t text_color = 0xFFFFFF; // Default white text
            if (strcmp(pantsColors[i], "White") == 0 || 
                strcmp(pantsColors[i], "Yellow") == 0 ||
                strcmp(pantsColors[i], "Tan") == 0) {
                text_color = 0x000000; // Black text for light backgrounds
            }
            
            lv_obj_t *label = lv_obj_get_child(btn, 0);
            if (label && lv_obj_check_type(label, &lv_label_class)) {
                lv_obj_set_style_text_color(label, lv_color_hex(text_color), 0);
            }
        }
    }

    lv_obj_t *back_btn = lv_btn_create(newMenu);
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
        
        // Reset next button pointers
        pants_next_btn = nullptr;
        shoes_next_btn = nullptr;
        
        createColorMenuShirt();
        if (colorMenu && colorMenu != lv_scr_act()) {
            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
            lv_obj_del_async(colorMenu);
            colorMenu = nullptr;
        }
    }, LV_EVENT_CLICKED, NULL);
    
    lv_scr_load(newMenu);
    
    if (colorMenu && colorMenu != newMenu) {
        DEBUG_PRINTF("Cleaning existing color menu: %p\n", colorMenu);
        lv_obj_del_async(colorMenu);
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
    lv_obj_add_flag(container, LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLLABLE);
    current_scroll_obj = container;

    // Static variable for drag tracking
    static lv_point_t last_point = {0, 0};

    // Drag-to-scroll handlers
    lv_obj_add_event_cb(container, [](lv_event_t *e) {
        lv_indev_t *indev = lv_indev_get_act();
        lv_indev_get_point(indev, &last_point);
    }, LV_EVENT_PRESSED, NULL);

    lv_obj_add_event_cb(container, [](lv_event_t *e) {
        lv_obj_t *container = (lv_obj_t*)lv_event_get_user_data(e);
        lv_indev_t *indev = lv_indev_get_act();
        lv_point_t curr_point;
        lv_indev_get_point(indev, &curr_point);
        lv_coord_t delta_y = last_point.y - curr_point.y;
        lv_obj_scroll_by(container, 0, delta_y, LV_ANIM_OFF);
        last_point = curr_point;
    }, LV_EVENT_PRESSING, container);

    int numShoeColors = sizeof(shoeColors) / sizeof(shoeColors[0]);
    DEBUG_PRINTF("Creating %d shoe color buttons\n", numShoeColors);

    String selectedColors = "";

    for (int i = 0; i < numShoeColors; i++) {
        lv_obj_t *btn = lv_btn_create(container);
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
            int idx = (int)lv_obj_get_user_data(btn);
            bool is_selected = lv_obj_has_state(btn, LV_STATE_USER_1);

            static String selectedColors;

            if (!is_selected) {
                lv_obj_add_state(btn, LV_STATE_USER_1);
                lv_obj_set_style_border_width(btn, 3, LV_PART_MAIN);
                lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFF00), LV_PART_MAIN);
                if (!selectedColors.isEmpty()) selectedColors += "+";
                selectedColors += shoeColors[idx];
            } else {
                lv_obj_clear_state(btn, LV_STATE_USER_1);
                lv_obj_set_style_border_width(btn, 1, LV_PART_MAIN);
                lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
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
                if (shoes_next_btn == nullptr) {
                    shoes_next_btn = lv_btn_create(lv_obj_get_parent(lv_obj_get_parent(btn)));
                    lv_obj_align(shoes_next_btn, LV_ALIGN_BOTTOM_RIGHT, -10, -10);
                    lv_obj_set_size(shoes_next_btn, 100, 40);
                    lv_obj_add_style(shoes_next_btn, &style_btn, 0);
                    lv_obj_add_style(shoes_next_btn, &style_btn_pressed, LV_STATE_PRESSED);
                    
                    lv_obj_t* next_label = lv_label_create(shoes_next_btn);
                    lv_label_set_text(next_label, "Next " LV_SYMBOL_RIGHT);
                    lv_obj_center(next_label);
                    
                    String* selectedColorsPtr = &selectedColors;
                    
                    lv_obj_add_event_cb(shoes_next_btn, [](lv_event_t* e) {
                        String* selectedColorsPtr = (String*)lv_event_get_user_data(e);
                        
                        currentEntry += *selectedColorsPtr + ",";
                        DEBUG_PRINTF("Current entry: %s\n", currentEntry.c_str());
                        delay(100);
                        DEBUG_PRINT("Transitioning to item menu");
                        createItemMenu();
                        if (colorMenu && colorMenu != lv_scr_act()) {
                            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
                            lv_obj_del_async(colorMenu);
                            colorMenu = nullptr;
                        }
                        DEBUG_PRINT("Shoes menu transition complete");
                    }, LV_EVENT_CLICKED, selectedColorsPtr);
                }
            }
        }, LV_EVENT_CLICKED, NULL);
    }

    // Set button colors based on their names
    for (int i = 0; i < numShoeColors; i++) {
        lv_obj_t *btn = lv_obj_get_child(container, i);
        if (btn && lv_obj_check_type(btn, &lv_btn_class)) {
            // Set button background color to match the actual color it represents
            uint32_t color_hex = 0x4A4A4A; // Default gray
            if (strcmp(shoeColors[i], "White") == 0) color_hex = 0xFFFFFF;
            else if (strcmp(shoeColors[i], "Black") == 0) color_hex = 0x000000;
            else if (strcmp(shoeColors[i], "Red") == 0) color_hex = 0xFF0000;
            else if (strcmp(shoeColors[i], "Orange") == 0) color_hex = 0xFFA500;
            else if (strcmp(shoeColors[i], "Yellow") == 0) color_hex = 0xFFFF00;
            else if (strcmp(shoeColors[i], "Green") == 0) color_hex = 0x00FF00;
            else if (strcmp(shoeColors[i], "Blue") == 0) color_hex = 0x0000FF;
            else if (strcmp(shoeColors[i], "Purple") == 0) color_hex = 0x800080;
            else if (strcmp(shoeColors[i], "Brown") == 0) color_hex = 0x8B4513;
            else if (strcmp(shoeColors[i], "Grey") == 0) color_hex = 0x808080;
            else if (strcmp(shoeColors[i], "Tan") == 0) color_hex = 0xD2B48C;
            
            lv_obj_set_style_bg_color(btn, lv_color_hex(color_hex), 0);
            lv_obj_set_style_bg_color(btn, lv_color_hex(color_hex), LV_STATE_PRESSED);
            
            // Adjust text color for visibility based on background
            uint32_t text_color = 0xFFFFFF; // Default white text
            if (strcmp(shoeColors[i], "White") == 0 || 
                strcmp(shoeColors[i], "Yellow") == 0 ||
                strcmp(shoeColors[i], "Tan") == 0) {
                text_color = 0x000000; // Black text for light backgrounds
            }
            
            lv_obj_t *label = lv_obj_get_child(btn, 0);
            if (label && lv_obj_check_type(label, &lv_label_class)) {
                lv_obj_set_style_text_color(label, lv_color_hex(text_color), 0);
            }
        }
    }

    lv_obj_t *back_btn = lv_btn_create(newMenu);
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
        
        // Reset next button pointer
        shoes_next_btn = nullptr;
        
        createColorMenuPants();
        if (colorMenu && colorMenu != lv_scr_act()) {
            DEBUG_PRINTF("Cleaning old color menu: %p\n", colorMenu);
            lv_obj_del_async(colorMenu);
            colorMenu = nullptr;
        }
    }, LV_EVENT_CLICKED, NULL);
    
    lv_scr_load(newMenu);
    
    if (colorMenu && colorMenu != newMenu) {
        DEBUG_PRINTF("Cleaning existing color menu: %p\n", colorMenu);
        lv_obj_del_async(colorMenu);
    }
    
    colorMenu = newMenu;
}

void createItemMenu() {
    if (itemMenu) {
        DEBUG_PRINT("Cleaning existing item menu");
        lv_obj_del(itemMenu);
        itemMenu = nullptr;
    }
    itemMenu = lv_obj_create(NULL);
    lv_obj_add_style(itemMenu, &style_screen, 0);
    lv_scr_load(itemMenu);
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
    lv_obj_add_flag(list_cont, LV_OBJ_FLAG_SCROLL_MOMENTUM | LV_OBJ_FLAG_SCROLLABLE);

    for (int i = 0; i < 7; i++) {
        lv_obj_t *btn = lv_btn_create(list_cont);
        lv_obj_set_width(btn, lv_pct(100));
        lv_obj_set_height(btn, 40);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, items[i]);
        lv_obj_center(label);
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            currentEntry += String(lv_label_get_text(lv_obj_get_child(lv_event_get_target(e), 0)));
            delay(100);
            createConfirmScreen();
        }, LV_EVENT_CLICKED, NULL);
    }
}

void createConfirmScreen() {
    if (confirmScreen) {
        DEBUG_PRINT("Cleaning existing confirm screen");
        lv_obj_del(confirmScreen);
        confirmScreen = nullptr;
    }
    confirmScreen = lv_obj_create(NULL);
    lv_obj_add_style(confirmScreen, &style_screen, 0);
    lv_scr_load(confirmScreen);
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

    lv_obj_t *btn_confirm = lv_btn_create(btn_container);
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

    lv_obj_t *btn_cancel = lv_btn_create(btn_container);
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
    
    // Add a spinner while scanning
    lv_obj_t* spinner = lv_spinner_create(wifi_screen, 1000, 60);
    lv_obj_set_size(spinner, 40, 40);
    lv_obj_align(spinner, LV_ALIGN_TOP_RIGHT, -10, 10);
    
    lv_timer_handler(); // Force UI update
    
    int n = WiFi.scanNetworks();
    DEBUG_PRINTF("Found %d networks\n", n);
    
    // Remove spinner
    lv_obj_del(spinner);
    
    if (wifi_list) {
        lv_obj_clean(wifi_list);
        
        if (n == 0) {
            lv_label_set_text(wifi_status_label, "No networks found");
        } else {
            lv_label_set_text(wifi_status_label, "Select a network:");
            
            for (int i = 0; i < n; ++i) {
                String ssid = WiFi.SSID(i);
                int rssi = WiFi.RSSI(i);
                
                // Create signal strength indicator based on RSSI
                String signal_indicator;
                if (rssi > -60) {
                    signal_indicator = "●●●";
                } else if (rssi > -70) {
                    signal_indicator = "●●○";
                } else {
                    signal_indicator = "●○○";
                }
                
                // Security indicator
                String security = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "" : "*";
                
                // Create list item with SSID and signal strength
                char list_text[50];
                snprintf(list_text, sizeof(list_text), "%s %s %s", 
                    ssid.c_str(), signal_indicator.c_str(), security.c_str());
                
                lv_obj_t* btn = lv_list_add_btn(wifi_list, LV_SYMBOL_WIFI, list_text);
                lv_obj_add_style(btn, &style_btn, 0);
                lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
                
                // Highlight the currently selected network
                if (strcmp(selected_ssid, ssid.c_str()) == 0) {
                    lv_obj_set_style_bg_color(btn, lv_color_hex(0x3E6E6E), 0); // Teal highlight
                }
                
                // Store SSID in user data
                char* btn_ssid = (char*)lv_mem_alloc(33);
                strncpy(btn_ssid, ssid.c_str(), 32);
                btn_ssid[32] = '\0';
                lv_obj_set_user_data(btn, btn_ssid);
                
                // Add click event
                lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                    lv_obj_t* btn = lv_event_get_target(e);
                    char* ssid = (char*)lv_obj_get_user_data(btn);
                    if (ssid) {
                        strncpy(selected_ssid, ssid, 32);
                        selected_ssid[32] = '\0';
                        DEBUG_PRINTF("Selected SSID: %s\n", selected_ssid);
                        
                        // Refresh the list to show the selected network
                        scanNetworks();
                        
                        // Show keyboard after selection is updated
                        showWiFiKeyboard();
                    }
                }, LV_EVENT_CLICKED, NULL);
            }
        }
    }
}

void showWiFiKeyboard() {
    if (wifi_keyboard == nullptr) {
        // Reset keyboard page index
        keyboard_page_index = 0;
        memset(wifi_password, 0, sizeof(wifi_password));
        
        // Create keyboard container
        wifi_keyboard = lv_obj_create(wifi_screen);
        lv_obj_set_size(wifi_keyboard, 320, 240);
        lv_obj_align(wifi_keyboard, LV_ALIGN_CENTER, 0, 0);
        lv_obj_set_style_bg_color(wifi_keyboard, lv_color_hex(0x1E1E1E), 0);
        
        // Add password text area
        lv_obj_t* ta = lv_textarea_create(wifi_keyboard);
        lv_obj_set_size(ta, 260, 40);
        lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 15);
        lv_textarea_set_password_mode(ta, true);
        lv_textarea_set_max_length(ta, 64);
        lv_textarea_set_placeholder_text(ta, "Password");
        lv_obj_add_event_cb(ta, [](lv_event_t* e) {
            lv_obj_t* ta = lv_event_get_target(e);
            const char* password = lv_textarea_get_text(ta);
            strncpy(wifi_password, password, 64);
            wifi_password[64] = '\0';
        }, LV_EVENT_VALUE_CHANGED, NULL);
        
        // Add cancel button (X) in top right corner
        lv_obj_t* close_btn = lv_btn_create(wifi_keyboard);
        lv_obj_set_size(close_btn, 40, 40);
        lv_obj_align(close_btn, LV_ALIGN_TOP_RIGHT, -5, 5);
        lv_obj_set_style_bg_color(close_btn, lv_color_hex(0xFF0000), 0);
        lv_obj_set_style_radius(close_btn, 20, 0);
        
        lv_obj_t* close_label = lv_label_create(close_btn);
        lv_label_set_text(close_label, LV_SYMBOL_CLOSE);
        lv_obj_center(close_label);
        
        // Create a custom button matrix instead of keyboard
        lv_obj_t* kb = lv_btnmatrix_create(wifi_keyboard);
        lv_obj_set_size(kb, 300, 150);
        lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, -5);
        
        // Set map and control map
        lv_btnmatrix_set_map(kb, btnm_mapplus[keyboard_page_index]);
        lv_btnmatrix_set_ctrl_map(kb, keyboard_ctrl_map);
        
        // Add event handler for the button matrix
        lv_obj_add_event_cb(kb, [](lv_event_t* e) {
            lv_event_code_t code = lv_event_get_code(e);
            lv_obj_t* btnm = lv_event_get_target(e);
            
            if (code == LV_EVENT_VALUE_CHANGED) {
                uint32_t btn_id = lv_btnmatrix_get_selected_btn(btnm);
                const char* txt = lv_btnmatrix_get_btn_text(btnm, btn_id);
                
                if (txt) {
                    DEBUG_PRINTF("Button matrix pressed: %s\n", txt);
                    
                    if (strcmp(txt, LV_SYMBOL_RIGHT) == 0) {
                        // Move to next keyboard page
                        keyboard_page_index = (keyboard_page_index + 1) % NUM_KEYBOARD_PAGES;
                        DEBUG_PRINTF("Changing to keyboard page: %d\n", keyboard_page_index);
                        // Original (incorrect): lv_btnmatrix_set_map(btnm, keyboard_maps[keyboard_page_index]);
                        lv_btnmatrix_set_map(btnm, btnm_mapplus[keyboard_page_index]);
                    } else if (strcmp(txt, LV_SYMBOL_LEFT) == 0) {
                        // Move to previous keyboard page
                        keyboard_page_index = (keyboard_page_index - 1 + NUM_KEYBOARD_PAGES) % NUM_KEYBOARD_PAGES;
                        DEBUG_PRINTF("Changing to keyboard page: %d\n", keyboard_page_index);
                        // Original (incorrect): lv_btnmatrix_set_map(btnm, keyboard_maps[keyboard_page_index]);
                        lv_btnmatrix_set_map(btnm, btnm_mapplus[keyboard_page_index]);
                    } else if (strcmp(txt, LV_SYMBOL_BACKSPACE) == 0) {
                        // Handle backspace
                        lv_obj_t* ta = lv_obj_get_child(wifi_keyboard, 0);
                        if (ta && lv_obj_check_type(ta, &lv_textarea_class)) {
                            lv_textarea_del_char(ta);
                        }
                    } else if (strcmp(txt, LV_SYMBOL_OK) == 0) {
                        // Hide keyboard and show connect/cancel buttons
                        if (wifi_keyboard) {
                            // Delete the keyboard but keep the container
                            lv_obj_clean(wifi_keyboard);
                            
                            // Add a label showing the password (as asterisks)
                            lv_obj_t* pwd_label = lv_label_create(wifi_keyboard);
                            lv_obj_align(pwd_label, LV_ALIGN_TOP_MID, 0, 30);
                            
                            char asterisks[65] = {0};
                            size_t len = strlen(wifi_password);
                            if (len > 64) len = 64;
                            for (size_t i = 0; i < len; i++) {
                                asterisks[i] = '*';
                            }
                            asterisks[len] = '\0';
                            
                            char buffer[100];
                            snprintf(buffer, sizeof(buffer), "Password: %s", asterisks);
                            lv_label_set_text(pwd_label, buffer);
                            
                            // Add connect button
                            lv_obj_t* connect_btn = lv_btn_create(wifi_keyboard);
                            lv_obj_set_size(connect_btn, 140, 50);
                            lv_obj_align(connect_btn, LV_ALIGN_CENTER, -75, 50);
                            lv_obj_add_style(connect_btn, &style_btn, 0);
                            lv_obj_add_style(connect_btn, &style_btn_pressed, LV_STATE_PRESSED);
                            
                            lv_obj_t* connect_label = lv_label_create(connect_btn);
                            lv_label_set_text(connect_label, "Connect");
                            lv_obj_center(connect_label);
                            
                            // Add cancel button
                            lv_obj_t* cancel_btn = lv_btn_create(wifi_keyboard);
                            lv_obj_set_size(cancel_btn, 140, 50);
                            lv_obj_align(cancel_btn, LV_ALIGN_CENTER, 75, 50);
                            lv_obj_add_style(cancel_btn, &style_btn, 0);
                            lv_obj_add_style(cancel_btn, &style_btn_pressed, LV_STATE_PRESSED);
                            
                            lv_obj_t* cancel_label = lv_label_create(cancel_btn);
                            lv_label_set_text(cancel_label, "Cancel");
                            lv_obj_center(cancel_label);
                            
                            // Add event callbacks
                            lv_obj_add_event_cb(connect_btn, [](lv_event_t* e) {
                                connectToWiFi();
                            }, LV_EVENT_CLICKED, NULL);
                            
                            lv_obj_add_event_cb(cancel_btn, [](lv_event_t* e) {
                                if (wifi_keyboard) {
                                    lv_obj_del(wifi_keyboard);
                                    wifi_keyboard = nullptr;
                                }
                            }, LV_EVENT_CLICKED, NULL);
                        }
                    } else {
                        // Regular key, add to textarea
                        lv_obj_t* ta = lv_obj_get_child(wifi_keyboard, 0);
                        if (ta && lv_obj_check_type(ta, &lv_textarea_class)) {
                            lv_textarea_add_text(ta, txt);
                        }
                    }
                }
            }
        }, LV_EVENT_VALUE_CHANGED, NULL);
        
        // Increase spacing between keys
        lv_obj_set_style_pad_row(kb, 15, 0);     
        lv_obj_set_style_pad_column(kb, 15, 0);  
        
        // Add border to keyboard buttons
        lv_obj_add_style(kb, &style_keyboard_btn, LV_PART_ITEMS);
        
        // Add event callback for close button
        lv_obj_add_event_cb(close_btn, [](lv_event_t* e) {
            if (wifi_keyboard) {
                lv_obj_del(wifi_keyboard);
                wifi_keyboard = nullptr;
            }
        }, LV_EVENT_CLICKED, NULL);
    }
}

void connectToWiFi() {
    if (strlen(wifi_password) < 1) {
        Serial.println("Password too short");
        return;
    }
    
    // Create a loading indicator
    lv_obj_t* loading_container = lv_obj_create(wifi_screen);
    lv_obj_set_size(loading_container, 200, 150);
    lv_obj_align(loading_container, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(loading_container, lv_color_hex(0x1E1E1E), 0);
    lv_obj_set_style_border_width(loading_container, 2, 0);
    lv_obj_set_style_border_color(loading_container, lv_color_hex(0x555555), 0);
    
    // Add a spinner
    lv_obj_t* spinner = lv_spinner_create(loading_container, 1000, 60);
    lv_obj_set_size(spinner, 80, 80);
    lv_obj_align(spinner, LV_ALIGN_TOP_MID, 0, 15);
    
    // Add connecting text
    lv_obj_t* connect_label = lv_label_create(loading_container);
    lv_label_set_text(connect_label, "Connecting to WiFi...");
    lv_obj_align(connect_label, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    // Force UI update
    lv_timer_handler();
    
    // Close the keyboard dialog if open
    if (wifi_keyboard) {
        lv_obj_del(wifi_keyboard);
        wifi_keyboard = nullptr;
    }
    
    // Try to connect
    Serial.println("Connecting to WiFi...");
    Serial.printf("SSID: %s, Password: %s\n", selected_ssid, wifi_password);
    
    WiFi.begin(selected_ssid, wifi_password);
    
    int attempt = 0;
    while (WiFi.status() != WL_CONNECTED && attempt < 20) {
        delay(500);
        Serial.print(".");
        lv_timer_handler();
        attempt++;
    }
    
    // Remove loading indicator
    lv_obj_del(loading_container);
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        
        // Save WiFi credentials
        preferences.begin("wifi", false);
        preferences.putString("ssid", selected_ssid);
        preferences.putString("password", wifi_password);
        preferences.end();
        
        // Update WiFi indicator
        updateWifiIndicator();
        
        // Refresh the network list
        scanNetworks();
    } else {
        Serial.println("\nFailed to connect to WiFi.");
        
        // Show error dialog
        lv_obj_t* error_box = lv_msgbox_create(wifi_screen, "Connection Failed", 
            "Failed to connect to the WiFi network.\nPlease check your password and try again.", 
            NULL, true);
        lv_obj_set_size(error_box, 280, 150);
        lv_obj_center(error_box);
    }
}

void createWiFiScreen() {
    if (wifi_screen) {
        lv_obj_del(wifi_screen);
    }
    
    wifi_screen = lv_obj_create(NULL);
    lv_obj_add_style(wifi_screen, &style_screen, 0);
    
    // Add header
    lv_obj_t* header = lv_obj_create(wifi_screen);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    
    lv_obj_t* title = lv_label_create(header);
    lv_label_set_text(title, "WiFi Setup");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);
    
    // Add status label
    wifi_status_label = lv_label_create(wifi_screen);
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_MID, 0, 60);
    lv_label_set_text(wifi_status_label, "Initializing...");
    
    // Add network list
    wifi_list = lv_list_create(wifi_screen);
    lv_obj_set_size(wifi_list, 300, 160);
    lv_obj_align(wifi_list, LV_ALIGN_TOP_MID, 0, 80);
    lv_obj_set_style_bg_color(wifi_list, lv_color_hex(0x2D2D2D), 0);
    
    // Add refresh button
    lv_obj_t* refresh_btn = lv_btn_create(wifi_screen);
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
    
    // Add back button
    lv_obj_t* back_btn = lv_btn_create(wifi_screen);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_LEFT, 10, -10);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT " Back");
    lv_obj_center(back_label);
    
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);
    
    // Load screen and start scan
    lv_scr_load(wifi_screen);
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