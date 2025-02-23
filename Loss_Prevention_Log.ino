#include <Wire.h>

#include <M5CoreS3.h>
#include "lv_conf.h"
#include <lvgl.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <WiFi.h>

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

// Expanded color options for different clothing types
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

// Styles
static lv_style_t style_screen, style_btn, style_btn_pressed, style_title, style_text;

void initStyles() {
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_hex(0x1E1E1E));
    lv_style_set_text_color(&style_screen, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_screen, &lv_font_montserrat_14);  // Default screen font

    lv_style_init(&style_btn);
    lv_style_set_radius(&style_btn, 10);
    lv_style_set_bg_color(&style_btn, lv_color_hex(0x00C4B4));
    lv_style_set_bg_grad_color(&style_btn, lv_color_hex(0xFF00FF));
    lv_style_set_bg_grad_dir(&style_btn, LV_GRAD_DIR_HOR);
    lv_style_set_border_width(&style_btn, 0);
    lv_style_set_text_color(&style_btn, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_btn, &lv_font_montserrat_16);  // Button font
    lv_style_set_pad_all(&style_btn, 10);

    lv_style_init(&style_btn_pressed);
    lv_style_set_bg_color(&style_btn_pressed, lv_color_hex(0xFF00FF));
    lv_style_set_bg_grad_color(&style_btn_pressed, lv_color_hex(0x00C4B4));
    lv_style_set_bg_grad_dir(&style_btn_pressed, LV_GRAD_DIR_HOR);

    lv_style_init(&style_title);
    lv_style_set_text_color(&style_title, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_title, &lv_font_montserrat_20);  // Title font
    
    lv_style_init(&style_text);
    lv_style_set_text_color(&style_text, lv_color_hex(0xFFFFFF));
    lv_style_set_text_font(&style_text, &lv_font_montserrat_14);  // Regular text font
}

WiFiConnectionHandler ArduinoIoTPreferredConnection(SSID, PASS);

void setup() {
    // Initialize Serial for debugging
    Serial.begin(115200);
    while (!Serial) delay(10);
    DEBUG_PRINT("Starting Loss Prevention Log...");
    
    // Initialize CoreS3
    auto cfg = M5.config();
    CoreS3.begin(cfg);
    DEBUG_PRINT("CoreS3 initialized");
    
    // Set display brightness and clear screen
    CoreS3.Display.setBrightness(100);
    CoreS3.Display.clear();
    CoreS3.Display.setRotation(1);  // Landscape mode
    DEBUG_PRINT("Display configured");
    
    // Initialize LVGL
    lv_init();
    DEBUG_PRINT("LVGL initialized");
    
    // Initialize the display buffer
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * 20);
    DEBUG_PRINT("Display buffer initialized");
    
    // Initialize the display
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = CoreS3.Display.width();
    disp_drv.ver_res = CoreS3.Display.height();
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    DEBUG_PRINT("Display driver registered");

    // Initialize the touchpad
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    DEBUG_PRINT("Touch input driver registered");

    // Initialize Arduino Cloud
    DEBUG_PRINT("Initializing Arduino Cloud connection...");
    ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
    ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
    ArduinoCloud.addProperty(logEntry, READWRITE, ON_CHANGE);
    ArduinoCloud.begin(ArduinoIoTPreferredConnection, false);
    setDebugMessageLevel(2);
    DEBUG_PRINT("Arduino Cloud initialized");

    // Initialize styles and create main menu
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

// Display flushing
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
        data->point.y = t.y;
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

void createMainMenu() {
    if (mainScreen) lv_obj_clean(mainScreen);
    else {
        mainScreen = lv_obj_create(NULL);
        lv_obj_add_style(mainScreen, &style_screen, 0);
    }
    lv_scr_load(mainScreen);
    addWifiIndicator(mainScreen);  // Add WiFi indicator

    // Header
    lv_obj_t *header = lv_obj_create(mainScreen);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Loss Prevention");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Buttons
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
    lv_obj_add_event_cb(btn_wifi, [](lv_event_t *e) { createWifiSettingsScreen(); }, LV_EVENT_CLICKED, NULL);
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

void disconnectWifi(lv_event_t *e) {
    Serial.println("Disconnecting...");
    cloudPaused = true;
    WiFi.disconnect(true);
    delay(500);
    WiFi.mode(WIFI_OFF);
    Serial.println("Disconnected");
    createWifiSettingsScreen();
}

void createGenderMenu() {
    if (genderMenu) lv_obj_clean(genderMenu);
    else {
        genderMenu = lv_obj_create(NULL);
        lv_obj_add_style(genderMenu, &style_screen, 0);
    }
    lv_scr_load(genderMenu);
    addWifiIndicator(genderMenu);  // Add WiFi indicator

    // Header
    lv_obj_t *header = lv_obj_create(genderMenu);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Select Gender");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Buttons
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
            createColorMenuShirt();
        }, LV_EVENT_CLICKED, NULL);
    }
}

void createColorMenuShirt() {
    DEBUG_PRINT("Creating Shirt Color Menu");
    if (colorMenu) {
        DEBUG_PRINT("Cleaning existing color menu");
        lv_obj_clean(colorMenu);
    } else {
        DEBUG_PRINT("Creating new color menu");
        colorMenu = lv_obj_create(NULL);
        lv_obj_add_style(colorMenu, &style_screen, 0);
    }
    lv_scr_load(colorMenu);
    addWifiIndicator(colorMenu);

    // Header
    lv_obj_t *header = lv_obj_create(colorMenu);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Select Shirt Color");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Color grid with scrolling
    lv_obj_t *grid = lv_obj_create(colorMenu);
    lv_obj_set_size(grid, 280, 180);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(grid, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_pad_all(grid, 5, 0);
    
    // Enable scrolling
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_row(grid, 5, 0);
    lv_obj_set_style_pad_column(grid, 5, 0);
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(grid, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_ACTIVE);
    
    // Set this as the current scroll object
    current_scroll_obj = grid;
    DEBUG_PRINT("Shirt Color Menu grid created and set as scroll object");

    // Calculate array size for shirt colors
    int numShirtColors = sizeof(shirtColors) / sizeof(shirtColors[0]);
    DEBUG_PRINTF("Creating %d shirt color buttons\n", numShirtColors);
    
    for (int i = 0; i < numShirtColors; i++) {
        lv_obj_t *btn = lv_btn_create(grid);
        lv_obj_set_size(btn, 90, 50);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, shirtColors[i]);
        lv_obj_center(label);
        
        // Store the color index in the button's user data
        int* colorIndex = (int*)lv_mem_alloc(sizeof(int));
        *colorIndex = i;
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t *btn = lv_event_get_target(e);
            int* idx = (int*)lv_obj_get_user_data(btn);
            if (idx != nullptr) {
                const char* selectedColor = shirtColors[*idx];
                DEBUG_PRINTF("Selected shirt color: %s\n", selectedColor);
                currentEntry += String(selectedColor) + ",";
                DEBUG_PRINTF("Current entry: %s\n", currentEntry.c_str());
                
                // Free the index memory
                lv_mem_free(idx);
            }
            
            // Clear current scroll object and prep for transition
            current_scroll_obj = nullptr;
            lv_obj_t* old_screen = lv_scr_act();
            
            // Schedule the transition to prevent potential crash
            lv_async_call([](void*) {
                DEBUG_PRINT("Creating pants menu after shirt selection");
                createColorMenuPants();
            }, nullptr);
            
            // Delete the old screen after scheduling the transition
            lv_obj_del_async(old_screen);
        }, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(btn, colorIndex);
        
        DEBUG_PRINTF("Created button for color: %s\n", shirtColors[i]);
    }
}

void createColorMenuPants() {
    DEBUG_PRINT("Creating Pants Color Menu");
    if (colorMenu) {
        DEBUG_PRINT("Cleaning existing color menu");
        lv_obj_clean(colorMenu);
    } else {
        DEBUG_PRINT("Creating new color menu");
        colorMenu = lv_obj_create(NULL);
        lv_obj_add_style(colorMenu, &style_screen, 0);
    }
    lv_scr_load(colorMenu);
    addWifiIndicator(colorMenu);  // Add WiFi indicator

    // Header
    lv_obj_t *header = lv_obj_create(colorMenu);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Select Pants Color");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Color grid with scrolling
    lv_obj_t *grid = lv_obj_create(colorMenu);
    lv_obj_set_size(grid, 280, 180);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_all(grid, 5, 0);
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(grid, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_ACTIVE);
    
    // Set this as the current scroll object
    current_scroll_obj = grid;
    DEBUG_PRINT("Pants Color Menu grid created and set as scroll object");

    // Calculate array size for pants colors
    int numPantsColors = sizeof(pantsColors) / sizeof(pantsColors[0]);
    
    for (int i = 0; i < numPantsColors; i++) {
        lv_obj_t *btn = lv_btn_create(grid);
        lv_obj_set_size(btn, 90, 50);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, pantsColors[i]);
        lv_obj_center(label);
        
        // Store the color index in the button's user data
        int* colorIndex = (int*)lv_mem_alloc(sizeof(int));
        *colorIndex = i;
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t *btn = lv_event_get_target(e);
            int* idx = (int*)lv_obj_get_user_data(btn);
            if (idx != nullptr) {
                const char* selectedColor = pantsColors[*idx];
                DEBUG_PRINTF("Selected pants color: %s\n", selectedColor);
                currentEntry += String(selectedColor) + ",";
                DEBUG_PRINTF("Current entry: %s\n", currentEntry.c_str());
                
                // Free the index memory
                lv_mem_free(idx);
            }
            
            // Clear current scroll object and prep for transition
            current_scroll_obj = nullptr;
            lv_obj_t* old_screen = lv_scr_act();
            
            // Schedule the transition to prevent potential crash
            lv_async_call([](void*) {
                DEBUG_PRINT("Creating shoes menu after pants selection");
                createColorMenuShoes();
            }, nullptr);
            
            // Delete the old screen after scheduling the transition
            lv_obj_del_async(old_screen);
        }, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(btn, colorIndex);
        
        DEBUG_PRINTF("Created button for color: %s\n", pantsColors[i]);
    }
}

void createColorMenuShoes() {
    DEBUG_PRINT("Creating Shoes Color Menu");
    if (colorMenu) {
        DEBUG_PRINT("Cleaning existing color menu");
        lv_obj_clean(colorMenu);
    } else {
        DEBUG_PRINT("Creating new color menu");
        colorMenu = lv_obj_create(NULL);
        lv_obj_add_style(colorMenu, &style_screen, 0);
    }
    lv_scr_load(colorMenu);
    addWifiIndicator(colorMenu);  // Add WiFi indicator

    // Header
    lv_obj_t *header = lv_obj_create(colorMenu);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Select Shoes Color");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Color grid with scrolling
    lv_obj_t *grid = lv_obj_create(colorMenu);
    lv_obj_set_size(grid, 280, 180);
    lv_obj_align(grid, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_opa(grid, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
    lv_obj_set_style_pad_all(grid, 5, 0);
    lv_obj_set_scroll_dir(grid, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(grid, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(grid, LV_SCROLLBAR_MODE_ACTIVE);
    
    // Set this as the current scroll object
    current_scroll_obj = grid;
    DEBUG_PRINT("Shoes Color Menu grid created and set as scroll object");

    // Calculate array size for shoe colors
    int numShoeColors = sizeof(shoeColors) / sizeof(shoeColors[0]);
    DEBUG_PRINTF("Creating %d shoe color buttons\n", numShoeColors);
    
    for (int i = 0; i < numShoeColors; i++) {
        lv_obj_t *btn = lv_btn_create(grid);
        lv_obj_set_size(btn, 90, 50);
        lv_obj_add_style(btn, &style_btn, 0);
        lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
        lv_obj_t *label = lv_label_create(btn);
        lv_label_set_text(label, shoeColors[i]);
        lv_obj_center(label);
        
        // Store the color index in the button's user data
        int* colorIndex = (int*)lv_mem_alloc(sizeof(int));
        *colorIndex = i;
        lv_obj_add_event_cb(btn, [](lv_event_t *e) {
            lv_obj_t *btn = lv_event_get_target(e);
            int* idx = (int*)lv_obj_get_user_data(btn);
            if (idx != nullptr) {
                const char* selectedColor = shoeColors[*idx];
                DEBUG_PRINTF("Selected shoe color: %s\n", selectedColor);
                currentEntry += String(selectedColor) + ",";
                DEBUG_PRINTF("Current entry: %s\n", currentEntry.c_str());
                
                // Free the index memory
                lv_mem_free(idx);
            }
            
            // Clear current scroll object and prep for transition
            current_scroll_obj = nullptr;
            lv_obj_t* old_screen = lv_scr_act();
            
            // Schedule the transition to prevent potential crash
            lv_async_call([](void*) {
                DEBUG_PRINT("Creating item menu after shoe selection");
                createItemMenu();
            }, nullptr);
            
            // Delete the old screen after scheduling the transition
            lv_obj_del_async(old_screen);
        }, LV_EVENT_CLICKED, NULL);
        lv_obj_set_user_data(btn, colorIndex);
        
        DEBUG_PRINTF("Created button for color: %s\n", shoeColors[i]);
    }
}

void createItemMenu() {
    if (itemMenu) lv_obj_clean(itemMenu);
    else {
        itemMenu = lv_obj_create(NULL);
        lv_obj_add_style(itemMenu, &style_screen, 0);
    }
    lv_scr_load(itemMenu);
    addWifiIndicator(itemMenu);  // Add WiFi indicator

    // Header
    lv_obj_t *header = lv_obj_create(itemMenu);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Select Item");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Create scrollable container
    lv_obj_t *list_cont = lv_obj_create(itemMenu);
    lv_obj_set_size(list_cont, 280, 180);
    lv_obj_align(list_cont, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(list_cont, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_pad_all(list_cont, 5, 0);
    
    // Enable scrolling
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(list_cont, 5, 0);
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
            createConfirmScreen();
        }, LV_EVENT_CLICKED, NULL);
    }
}

void createConfirmScreen() {
    if (confirmScreen) lv_obj_clean(confirmScreen);
    else {
        confirmScreen = lv_obj_create(NULL);
        lv_obj_add_style(confirmScreen, &style_screen, 0);
    }
    lv_scr_load(confirmScreen);
    addWifiIndicator(confirmScreen);  // Add WiFi indicator

    // Header
    lv_obj_t *header = lv_obj_create(confirmScreen);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Confirm Entry");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Entry preview
    lv_obj_t *preview = lv_label_create(confirmScreen);
    lv_label_set_text(preview, getFormattedEntry(currentEntry).c_str());
    lv_obj_add_style(preview, &style_text, 0);
    lv_obj_set_size(preview, 280, 140);
    lv_obj_align(preview, LV_ALIGN_TOP_MID, 0, 60);
    lv_obj_set_style_bg_color(preview, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_style_pad_all(preview, 10, 0);

    // Buttons container
    lv_obj_t *btn_container = lv_obj_create(confirmScreen);
    lv_obj_remove_style_all(btn_container);
    lv_obj_set_size(btn_container, 320, 50);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // Confirm button
    lv_obj_t *btn_confirm = lv_btn_create(btn_container);
    lv_obj_set_size(btn_confirm, 130, 40);
    lv_obj_add_style(btn_confirm, &style_btn, 0);
    lv_obj_add_style(btn_confirm, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *label_confirm = lv_label_create(btn_confirm);
    lv_label_set_text(label_confirm, "Confirm");
    lv_obj_center(label_confirm);
    lv_obj_add_event_cb(btn_confirm, [](lv_event_t *e) {
        // Save the entry
        saveEntry(getFormattedEntry(currentEntry));
        
        // Clear the current entry
        currentEntry = "";
        
        // Add a small delay before screen transition
        delay(100);
        
        // Return to main menu
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);

    // Cancel button
    lv_obj_t *btn_cancel = lv_btn_create(btn_container);
    lv_obj_set_size(btn_cancel, 130, 40);
    lv_obj_add_style(btn_cancel, &style_btn, 0);
    lv_obj_add_style(btn_cancel, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *label_cancel = lv_label_create(btn_cancel);
    lv_label_set_text(label_cancel, "Cancel");
    lv_obj_center(label_cancel);
    lv_obj_add_event_cb(btn_cancel, [](lv_event_t *e) {
        currentEntry = "";
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);
}

void scanWifiCallback(lv_event_t *e) {
    lv_obj_t *list_cont = (lv_obj_t*)lv_event_get_user_data(e);
    lv_obj_clean(list_cont);

    lv_obj_t *loading = lv_label_create(list_cont);
    lv_label_set_text(loading, "Scanning...");
    lv_obj_add_style(loading, &style_text, 0);
    lv_obj_align(loading, LV_ALIGN_CENTER, 0, 0);

    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    int n = WiFi.scanNetworks();
    lv_obj_del(loading);

    numNetworks = (n > 0) ? min(n, MAX_NETWORKS) : 0;
    if (numNetworks == 0) {
        lv_obj_t *no_networks = lv_label_create(list_cont);
        lv_label_set_text(no_networks, "No networks found");
        lv_obj_add_style(no_networks, &style_text, 0);
        lv_obj_set_width(no_networks, lv_pct(100));
        lv_obj_set_style_text_align(no_networks, LV_TEXT_ALIGN_CENTER, 0);
    } else {
        for (int i = 0; i < numNetworks; i++) {
            strncpy(savedSSIDs[i], WiFi.SSID(i).c_str(), 31);
            savedSSIDs[i][31] = '\0';
            lv_obj_t *btn = lv_btn_create(list_cont);
            lv_obj_set_width(btn, lv_pct(100));
            lv_obj_set_height(btn, 40);
            lv_obj_add_style(btn, &style_btn, 0);
            lv_obj_add_style(btn, &style_btn_pressed, LV_STATE_PRESSED);
            lv_obj_t *ssid_label = lv_label_create(btn);
            lv_label_set_text(ssid_label, savedSSIDs[i]);
            lv_obj_add_style(ssid_label, &style_text, 0);
            lv_obj_align(ssid_label, LV_ALIGN_LEFT_MID, 5, 0);
            lv_obj_add_event_cb(btn, wifiButtonCallback, LV_EVENT_CLICKED, (void*)i);
            Serial.printf("Added network button %d: %s, Parent: %p\n", i, savedSSIDs[i], lv_obj_get_parent(btn));
        }
    }
    WiFi.scanDelete();
    lv_obj_invalidate(list_cont);
    lv_refr_now(NULL);
    Serial.printf("List content height: %d, Scroll position: %d\n", lv_obj_get_content_height(list_cont), lv_obj_get_scroll_y(list_cont));
    Serial.printf("Free heap after scan: %u bytes\n", ESP.getFreeHeap());
}

void createWifiSettingsScreen() {
    if (wifiSettingsScreen) lv_obj_clean(wifiSettingsScreen);
    else {
        wifiSettingsScreen = lv_obj_create(NULL);
        lv_obj_add_style(wifiSettingsScreen, &style_screen, 0);
    }
    lv_scr_load(wifiSettingsScreen);
    addWifiIndicator(wifiSettingsScreen);  // Add WiFi indicator

    cloudPaused = true;

    // Header
    lv_obj_t *header = lv_obj_create(wifiSettingsScreen);
    lv_obj_set_size(header, 320, 50);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x333333), 0);
    lv_obj_t *title = lv_label_create(header);
    lv_label_set_text(title, "Wi-Fi Settings");
    lv_obj_add_style(title, &style_title, 0);
    lv_obj_align(title, LV_ALIGN_CENTER, 0, 0);

    // Status
    lv_obj_t *status = lv_label_create(wifiSettingsScreen);
    String wifi_text = "Status: " + String(WiFi.status() == WL_CONNECTED ? WiFi.SSID() : "Not Connected");
    lv_label_set_text(status, wifi_text.c_str());
    lv_obj_add_style(status, &style_text, 0);
    lv_obj_align(status, LV_ALIGN_TOP_MID, 0, 60);

    // Network list
    lv_obj_t *list_cont = lv_obj_create(wifiSettingsScreen);
    lv_obj_set_size(list_cont, 280, 140);
    lv_obj_align(list_cont, LV_ALIGN_TOP_MID, 0, 90);
    lv_obj_set_style_bg_color(list_cont, lv_color_hex(0x4A4A4A), 0);
    lv_obj_set_flex_flow(list_cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_scrollbar_mode(list_cont, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_add_flag(list_cont, LV_OBJ_FLAG_SCROLLABLE);

    // Scan button
    lv_obj_t *scan_btn = lv_btn_create(wifiSettingsScreen);
    lv_obj_set_size(scan_btn, 130, 40);
    lv_obj_align(scan_btn, LV_ALIGN_BOTTOM_LEFT, 20, -10);
    lv_obj_add_style(scan_btn, &style_btn, 0);
    lv_obj_add_style(scan_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, "Scan");
    lv_obj_center(scan_label);
    lv_obj_add_event_cb(scan_btn, scanWifiCallback, LV_EVENT_CLICKED, list_cont);

    // Disconnect button
    lv_obj_t *disconnect_btn = lv_btn_create(wifiSettingsScreen);
    lv_obj_set_size(disconnect_btn, 130, 40);
    lv_obj_align(disconnect_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_add_style(disconnect_btn, &style_btn, 0);
    lv_obj_add_style(disconnect_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *disconnect_label = lv_label_create(disconnect_btn);
    lv_label_set_text(disconnect_label, "Disconnect");
    lv_obj_center(disconnect_label);
    lv_obj_add_event_cb(disconnect_btn, disconnectWifi, LV_EVENT_CLICKED, NULL);

    // Back button
    lv_obj_t *back_btn = lv_btn_create(wifiSettingsScreen);
    lv_obj_set_size(back_btn, 130, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_RIGHT, -20, -10);
    lv_obj_add_style(back_btn, &style_btn, 0);
    lv_obj_add_style(back_btn, &style_btn_pressed, LV_STATE_PRESSED);
    lv_obj_t *back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t *e) {
        cloudPaused = false;
        lv_scr_load(mainScreen);
    }, LV_EVENT_CLICKED, NULL);

    lv_scr_load(wifiSettingsScreen);
    Serial.printf("Free heap after WiFi settings: %u bytes\n", ESP.getFreeHeap());
}

// Struct to hold connection data
struct ConnectData {
    int idx;
    lv_obj_t *ta;
    lv_obj_t *modal;
};

// Callback function for connecting to Wi-Fi
void connectWifiCallback(lv_event_t *e) {
    ConnectData *data = (ConnectData*)lv_event_get_user_data(e);
    const char* password = lv_textarea_get_text(data->ta);
    WiFi.begin(savedSSIDs[data->idx], password);
    int timeout = 10000;
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
        delay(500);
    }
    lv_obj_del(data->modal);
    cloudPaused = false;
    createWifiSettingsScreen();
    delete data;
}

void wifiButtonCallback(lv_event_t *e) {
    if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
        int idx = (int)lv_event_get_user_data(e);
        if (idx >= 0 && idx < numNetworks) {
            lv_obj_t *modal = lv_obj_create(lv_scr_act());
            lv_obj_set_size(modal, 300, 200);
            lv_obj_center(modal);
            lv_obj_set_style_bg_color(modal, lv_color_hex(0x4A4A4A), 0);
            lv_obj_set_style_border_color(modal, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_border_width(modal, 2, 0);

            lv_obj_t *title = lv_label_create(modal);
            lv_label_set_text(title, "Enter Password");
            lv_obj_add_style(title, &style_title, 0);
            lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

            lv_obj_t *ta = lv_textarea_create(modal);
            lv_obj_set_size(ta, 260, 40);
            lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 50);
            lv_textarea_set_password_mode(ta, true);

            lv_obj_t *kb = lv_keyboard_create(modal);
            lv_obj_set_size(kb, 300, 100);
            lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
            lv_keyboard_set_textarea(kb, ta);

            lv_obj_t *connect_btn = lv_btn_create(modal);
            lv_obj_set_size(connect_btn, 100, 40);
            lv_obj_align(connect_btn, LV_ALIGN_TOP_MID, 0, 100);
            lv_obj_add_style(connect_btn, &style_btn, 0);
            lv_obj_add_style(connect_btn, &style_btn_pressed, LV_STATE_PRESSED);
            lv_obj_t *connect_label = lv_label_create(connect_btn);
            lv_label_set_text(connect_label, "Connect");
            lv_obj_center(connect_label);

            ConnectData *data = new ConnectData{idx, ta, modal};
            lv_obj_add_event_cb(connect_btn, connectWifiCallback, LV_EVENT_CLICKED, data);
        }
    }
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
    // Save to logEntry variable
    logEntry = entry;
    
    // Log to Serial for debugging
    Serial.println("New Entry:");
    Serial.println(entry);
    
    // Try to send to cloud if connected
    if (WiFi.status() == WL_CONNECTED && ArduinoCloud.connected()) {
        Serial.println("Sending to Arduino Cloud...");
        // Add a small delay to ensure proper handling
        delay(100);
    } else {
        Serial.println("Cloud not connected, logged locally only");
    }
}