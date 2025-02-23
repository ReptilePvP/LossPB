#include <Wire.h>
#include <M5Unified.h>
#include <M5GFX.h>
#include "lv_conf.h"
#include <lvgl.h>
#include <ArduinoIoTCloud.h>
#include <Arduino_ConnectionHandler.h>
#include <WiFi.h>

// Declare the Unscii 16 font
LV_FONT_DECLARE(lv_font_unscii_16);

// Define screen dimensions for CoreS3
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define LV_TICK_PERIOD_MS 1

// Display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_WIDTH * 10];

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

#define MAX_NETWORKS 50
static String* savedSSIDs = nullptr;

// Menu options
const char* genders[] = {"Male", "Female"};
const char* colors[] = {"White", "Black", "Blue", "Yellow", "Green", "Orange", "Pink", "Red", "Grey"};
const char* items[] = {"Jewelry", "Women's Shoes", "Men's Shoes", "Cosmetics", "Fragrances", "Home", "Kids"};

// LVGL objects
lv_obj_t *mainScreen, *menuScreen, *genderMenu, *shirtMenu, *pantsMenu, *shoeMenu, *itemMenu, *confirmScreen;
lv_obj_t *wifiIndicator;
lv_obj_t *wifiScreen;
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
void disconnectWifi(lv_event_t *e);
void createGenderMenu();
void createColorMenu(const char* title, lv_obj_t** menu, void (*nextMenuFunc)());
void colorMenuCallback(lv_event_t *e);
void createItemMenu();
void createConfirmScreen();
void createWifiScreen();
void wifiButtonCallback(lv_event_t *e);
void wifiConnectCallback(const char* ssid);
void connectWifi(lv_event_t *e);
void cleanupWifiScreen();
void createWifiSettingsScreen();
void createWifiScanScreen();

static bool isWiFiScanning = false;
static lv_obj_t* cont_wifi = NULL;  // Global container for WiFi list
static lv_style_t style_wifi_cont;   // Style for WiFi container

static void wifi_scroll_event_cb(lv_event_t * e) {
    lv_obj_t * cont = lv_event_get_target(e);

    lv_area_t cont_a;
    lv_obj_get_coords(cont, &cont_a);
    lv_coord_t cont_y_center = cont_a.y1 + lv_area_get_height(&cont_a) / 2;

    lv_coord_t r = lv_obj_get_height(cont) * 7 / 10;
    uint32_t child_cnt = lv_obj_get_child_cnt(cont);
    for(uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t * child = lv_obj_get_child(cont, i);
        lv_area_t child_a;
        lv_obj_get_coords(child, &child_a);

        lv_coord_t child_y_center = child_a.y1 + lv_area_get_height(&child_a) / 2;
        lv_coord_t diff_y = child_y_center - cont_y_center;
        diff_y = LV_ABS(diff_y);

        lv_coord_t x;
        if(diff_y >= r) {
            x = r;
        }
        else {
            uint32_t x_sqr = r * r - diff_y * diff_y;
            lv_sqrt_res_t res;
            lv_sqrt(x_sqr, &res, 0x8000);
            x = r - res.i;
        }

        lv_obj_set_style_translate_x(child, x, 0);
        lv_opa_t opa = lv_map(x, 0, r, LV_OPA_TRANSP, LV_OPA_COVER);
        lv_obj_set_style_opa(child, LV_OPA_COVER - opa, 0);
    }
}

void cleanupWifiScreen() {
    // First delete the container if it exists
    if (cont_wifi != NULL) {
        lv_obj_del(cont_wifi);
        cont_wifi = NULL;
    }
    
    // Then clean up the WiFi scan
    if (isWiFiScanning) {
        WiFi.scanDelete();
        isWiFiScanning = false;
    }
    
    // Finally delete the screen
    if (wifiScreen != NULL) {
        lv_obj_del(wifiScreen);
        wifiScreen = NULL;
    }
}

void createWifiScreen() {
    // Clean up any previous instance
    cleanupWifiScreen();
    
    // Initialize style if not done
    static bool style_initialized = false;
    if (!style_initialized) {
        lv_style_init(&style_wifi_cont);
        lv_style_set_bg_color(&style_wifi_cont, lv_color_hex(0x000000));
        lv_style_set_pad_row(&style_wifi_cont, 8);
        lv_style_set_pad_all(&style_wifi_cont, 4);
        style_initialized = true;
    }
    
    // Create main screen
    wifiScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(wifiScreen, lv_color_hex(0x000000), 0);
    
    // Create title
    lv_obj_t * title = lv_label_create(wifiScreen);
    lv_label_set_text(title, "WiFi Networks");
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create scrollable container
    cont_wifi = lv_obj_create(wifiScreen);
    lv_obj_set_size(cont_wifi, 240, 160);
    lv_obj_align(cont_wifi, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_style(cont_wifi, &style_wifi_cont, 0);
    
    // Set container properties
    lv_obj_set_flex_flow(cont_wifi, LV_FLEX_FLOW_COLUMN);
    lv_obj_add_event_cb(cont_wifi, wifi_scroll_event_cb, LV_EVENT_SCROLL, NULL);
    
    // Set scroll properties
    lv_obj_set_style_radius(cont_wifi, 5, 0);
    lv_obj_set_scroll_dir(cont_wifi, LV_DIR_VER);
    lv_obj_set_scroll_snap_y(cont_wifi, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(cont_wifi, LV_SCROLLBAR_MODE_AUTO);
    
    // Start WiFi scan
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    
    isWiFiScanning = true;
    int n = WiFi.scanNetworks();
    
    if (n == 0) {
        lv_obj_t * no_networks = lv_label_create(cont_wifi);
        lv_label_set_text(no_networks, "No networks found");
        lv_obj_set_width(no_networks, lv_pct(100));
    } else {
        for (int i = 0; i < n; ++i) {
            // Create button
            lv_obj_t * btn = lv_btn_create(cont_wifi);
            lv_obj_set_width(btn, lv_pct(100));
            lv_obj_set_height(btn, 60);
            lv_obj_add_style(btn, &btn_style, 0);
            
            // Store network info
            char* ssid = strdup(WiFi.SSID(i).c_str());
            if (ssid != NULL) {
                lv_obj_add_event_cb(btn, [](lv_event_t * e) {
                    lv_obj_t * btn = lv_event_get_target(e);
                    char* ssid = (char*)lv_obj_get_user_data(btn);
                    if (ssid != NULL) {
                        WiFi.scanDelete();
                        isWiFiScanning = false;
                        wifiConnectCallback(ssid);
                        free(ssid);  // Free the SSID after use
                    }
                }, LV_EVENT_CLICKED, ssid);
                lv_obj_set_user_data(btn, ssid);
                
                // Create info container
                lv_obj_t * info_cont = lv_obj_create(btn);
                lv_obj_set_size(info_cont, lv_pct(100), LV_SIZE_CONTENT);
                lv_obj_set_style_pad_all(info_cont, 5, 0);
                lv_obj_set_style_bg_opa(info_cont, LV_OPA_TRANSP, 0);
                lv_obj_set_flex_flow(info_cont, LV_FLEX_FLOW_COLUMN);
                lv_obj_set_flex_align(info_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
                
                // Add SSID label
                lv_obj_t * ssid_label = lv_label_create(info_cont);
                lv_label_set_text(ssid_label, ssid);
                lv_obj_set_style_text_font(ssid_label, &lv_font_unscii_16, 0);
                
                // Add info label
                lv_obj_t * info_label = lv_label_create(info_cont);
                String info = String("Strength: ") + String(WiFi.RSSI(i)) + " dBm";
                info += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? " (Open)" : " (Secured)";
                lv_label_set_text(info_label, info.c_str());
            }
        }
    }
    
    // Clean up scan results
    WiFi.scanDelete();
    isWiFiScanning = false;
    
    // Create back button
    lv_obj_t * back_btn = lv_btn_create(wifiScreen);
    lv_obj_add_style(back_btn, &btn_style, 0);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    
    lv_obj_t * back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    
    // Add back button callback
    lv_obj_add_event_cb(back_btn, [](lv_event_t * e) {
        // First load the main screen
        lv_scr_load_anim(mainScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
        
        // Then schedule the cleanup after animation completes
        lv_timer_t * timer = lv_timer_create([](lv_timer_t * timer) {
            cleanupWifiScreen();
            lv_timer_del(timer);
        }, 600, NULL);
    }, LV_EVENT_CLICKED, NULL);
    
    // Enable scrolling and show the screen
    lv_obj_scroll_to_view(lv_obj_get_child(cont_wifi, 0), LV_ANIM_OFF);
    lv_scr_load_anim(wifiScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, false);
}

void connectWifi(lv_event_t *e) {
    struct ConnectData {
        const char* ssid;
        lv_obj_t* ta;
    };
    ConnectData* data = (ConnectData*)lv_event_get_user_data(e);
    const char* ssid = data->ssid;
    const char* password = lv_textarea_get_text(data->ta);
    Serial.println("Connecting to " + String(ssid) + " with password: " + String(password));
    WiFi.begin(ssid, password);
    int timeout = 10000;
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < timeout) {
        delay(500);
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to " + String(ssid));
        cloudPaused = false;
    } else {
        Serial.println("\nFailed to connect to " + String(ssid));
        cloudPaused = false;
    }
    delete data;
    createMainMenu();
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

    initButtonStyle();
    Serial.println("Button style initialized");
    createMainMenu();
    Serial.println("Main menu displayed");
}

void loop() {
    M5.update();
    if (!cloudPaused) {
        ArduinoCloud.update();
    }
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
    static bool was_pressed = false;

    if (M5.Touch.getCount() > 0) {
        auto t = M5.Touch.getDetail();
        int16_t adjusted_x = t.x;
        int16_t adjusted_y = t.y;

        if (adjusted_x < 0) adjusted_x = 0;
        if (adjusted_x >= SCREEN_WIDTH) adjusted_x = SCREEN_WIDTH - 1;
        if (adjusted_y < 0) adjusted_y = 0;
        if (adjusted_y >= SCREEN_HEIGHT) adjusted_y = SCREEN_HEIGHT - 1;

        last_x = adjusted_x;
        last_y = adjusted_y;
        data->point.x = last_x;
        data->point.y = last_y;

        if (t.state == m5::touch_state_t::touch) {
            data->state = LV_INDEV_STATE_PR;
            if (!was_pressed) {
                Serial.printf("Touch PRESS at: %d, %d, State: %d (Raw: %d, %d)\n", last_x, last_y, (int)t.state, t.x, t.y);
                was_pressed = true;
            }
        } else if (t.state == m5::touch_state_t::drag || t.state == m5::touch_state_t::hold) {
            data->state = LV_INDEV_STATE_PR;
            Serial.printf("Touch DRAG at: %d, %d, State: %d (Raw: %d, %d)\n", last_x, last_y, (int)t.state, t.x, t.y);
        }
    } else if (was_pressed) {
        data->point.x = last_x;
        data->point.y = last_y;
        data->state = LV_INDEV_STATE_REL;
        Serial.printf("Touch RELEASE at: %d, %d\n", last_x, last_y);
        was_pressed = false;
    } else {
        data->state = LV_INDEV_STATE_REL; // No position update when idle
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
    lv_obj_set_size(btnNew, 220, 40);
    lv_obj_align(btnNew, LV_ALIGN_CENTER, 0, -90);
    lv_obj_t *labelNew = lv_label_create(btnNew);
    lv_label_set_text(labelNew, "Create New Entry");
    lv_label_set_long_mode(labelNew, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(labelNew, 180);
    lv_obj_center(labelNew);
    lv_obj_add_event_cb(btnNew, [](lv_event_t *e) { createGenderMenu(); }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btnView = lv_btn_create(mainScreen);
    lv_obj_add_style(btnView, &btn_style, 0);
    lv_obj_set_size(btnView, 220, 40);
    lv_obj_align(btnView, LV_ALIGN_CENTER, 0, -30);
    lv_obj_t *labelView = lv_label_create(btnView);
    lv_label_set_text(labelView, "View Latest Entries");
    lv_label_set_long_mode(labelView, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(labelView, 180);
    lv_obj_center(labelView);

    lv_obj_t *btnWifi = lv_btn_create(mainScreen);
    lv_obj_add_style(btnWifi, &btn_style, 0);
    lv_obj_set_size(btnWifi, 220, 40);
    lv_obj_align(btnWifi, LV_ALIGN_CENTER, 0, 30);
    lv_obj_t *labelWifi = lv_label_create(btnWifi);
    lv_label_set_text(labelWifi, "Wi-Fi Settings");
    lv_label_set_long_mode(labelWifi, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(labelWifi, 180);
    lv_obj_center(labelWifi);
    lv_obj_add_event_cb(btnWifi, [](lv_event_t *e) { createWifiSettingsScreen(); }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btnDisconnect = lv_btn_create(mainScreen);
    lv_obj_add_style(btnDisconnect, &btn_style, 0);
    lv_obj_set_size(btnDisconnect, 220, 40);
    lv_obj_align(btnDisconnect, LV_ALIGN_CENTER, 0, 90);
    lv_obj_t *labelDisconnect = lv_label_create(btnDisconnect);
    lv_label_set_text(labelDisconnect, "Disconnect");
    lv_label_set_long_mode(labelDisconnect, LV_LABEL_LONG_SCROLL_CIRCULAR);
    lv_obj_set_width(labelDisconnect, 180);
    lv_obj_center(labelDisconnect);
    lv_obj_add_event_cb(btnDisconnect, disconnectWifi, LV_EVENT_CLICKED, NULL);

    wifiIndicator = lv_label_create(mainScreen);
    lv_obj_set_style_text_font(wifiIndicator, &lv_font_unscii_16, 0);
    lv_obj_set_style_text_color(wifiIndicator, lv_color_hex(0xF0EDE5), 0);
    lv_obj_align(wifiIndicator, LV_ALIGN_TOP_RIGHT, -10, 10);
    updateWifiIndicator();
}

void updateWifiIndicator() {
    if (WiFi.status() == WL_CONNECTED) {
        lv_label_set_text(wifiIndicator, "WiFi: ON");
        lv_obj_set_style_text_color(wifiIndicator, lv_color_hex(0x00FF00), 0);
    } else {
        lv_label_set_text(wifiIndicator, "WiFi: OFF");
        lv_obj_set_style_text_color(wifiIndicator, lv_color_hex(0xFF0000), 0);
    }
}

void disconnectWifi(lv_event_t *e) {
    Serial.println("Disconnecting from current network...");
    cloudPaused = true;
    WiFi.disconnect(true);
    delay(500);
    WiFi.mode(WIFI_OFF);
    Serial.println("Disconnected");
    createMainMenu();
}

void createGenderMenu() {
    genderMenu = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(genderMenu, lv_color_hex(0x1A1A1A), 0);
    lv_scr_load(genderMenu);

    lv_obj_t *title = lv_label_create(genderMenu);
    lv_label_set_text(title, "Select Gender");
    lv_obj_set_style_text_font(title, &lv_font_unscii_16, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0xF0EDE5), 0);
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
    lv_obj_set_style_text_color(lblTitle, lv_color_hex(0xF0EDE5), 0);
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
    lv_obj_set_style_text_color(title, lv_color_hex(0xF0EDE5), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);

    for (int i = 0; i < 7; i++) {
        lv_obj_t *btn = lv_btn_create(itemMenu);
        lv_obj_add_style(btn, &btn_style, 0);
        lv_obj_set_size(btn, 150, 40);
        lv_obj_set_pos(btn, (i % 2) * 155 + 5, (i / 3) * 50 + 40);
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
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10);

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

void initProperties() {
    ArduinoCloud.setBoardId(DEVICE_LOGIN_NAME);
    ArduinoCloud.setSecretDeviceKey(DEVICE_KEY);
    ArduinoCloud.addProperty(logEntry, READWRITE, ON_CHANGE);
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
    } else {
        return "NoTime";
    }
}

String getFormattedEntry(const String& entry) {
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

static lv_obj_t* wifiSettingsScreen = NULL;
static lv_obj_t* wifiScanScreen = NULL;
static lv_style_t style_wifi_info;

void createWifiSettingsScreen() {
    if (wifiSettingsScreen != NULL) {
        lv_obj_del(wifiSettingsScreen);
    }
    
    // Initialize style if not done
    static bool style_initialized = false;
    if (!style_initialized) {
        lv_style_init(&style_wifi_info);
        lv_style_set_text_font(&style_wifi_info, &lv_font_unscii_16);
        lv_style_set_text_color(&style_wifi_info, lv_color_hex(0xFFFFFF));
        style_initialized = true;
    }
    
    // Create main screen
    wifiSettingsScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(wifiSettingsScreen, lv_color_hex(0x000000), 0);
    
    // Create title
    lv_obj_t* title = lv_label_create(wifiSettingsScreen);
    lv_label_set_text(title, "WiFi Settings");
    lv_obj_add_style(title, &style_wifi_info, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create WiFi status info
    lv_obj_t* info_container = lv_obj_create(wifiSettingsScreen);
    lv_obj_set_size(info_container, 280, 80);
    lv_obj_align(info_container, LV_ALIGN_TOP_MID, 0, 40);
    lv_obj_set_style_bg_color(info_container, lv_color_hex(0x222222), 0);
    lv_obj_set_style_pad_all(info_container, 10, 0);
    
    // WiFi SSID
    lv_obj_t* wifi_label = lv_label_create(info_container);
    String wifi_text = "WiFi: ";
    wifi_text += (WiFi.status() == WL_CONNECTED) ? WiFi.SSID() : "Not Connected";
    lv_label_set_text(wifi_label, wifi_text.c_str());
    lv_obj_align(wifi_label, LV_ALIGN_TOP_LEFT, 0, 0);
    
    // IP Address
    lv_obj_t* ip_label = lv_label_create(info_container);
    String ip_text = "Local IP: ";
    ip_text += (WiFi.status() == WL_CONNECTED) ? WiFi.localIP().toString() : "---";
    lv_label_set_text(ip_label, ip_text.c_str());
    lv_obj_align(ip_label, LV_ALIGN_TOP_LEFT, 0, 30);
    
    // Create buttons container
    lv_obj_t* btn_container = lv_obj_create(wifiSettingsScreen);
    lv_obj_set_size(btn_container, 280, 160);
    lv_obj_align(btn_container, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_set_style_bg_opa(btn_container, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(btn_container, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(btn_container, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(btn_container, 10, 0);
    
    // Scan Networks button
    lv_obj_t* scan_btn = lv_btn_create(btn_container);
    lv_obj_set_size(scan_btn, 200, 40);
    lv_obj_add_style(scan_btn, &btn_style, 0);
    lv_obj_t* scan_label = lv_label_create(scan_btn);
    lv_label_set_text(scan_label, "Scan for Networks");
    lv_obj_center(scan_label);
    lv_obj_add_event_cb(scan_btn, [](lv_event_t* e) {
        createWifiScanScreen();
    }, LV_EVENT_CLICKED, NULL);
    
    // Disconnect button (only show if connected)
    if (WiFi.status() == WL_CONNECTED) {
        lv_obj_t* disconnect_btn = lv_btn_create(btn_container);
        lv_obj_set_size(disconnect_btn, 200, 40);
        lv_obj_add_style(disconnect_btn, &btn_style, 0);
        lv_obj_t* disconnect_label = lv_label_create(disconnect_btn);
        lv_label_set_text(disconnect_label, "Disconnect");
        lv_obj_center(disconnect_label);
        lv_obj_add_event_cb(disconnect_btn, [](lv_event_t* e) {
            WiFi.disconnect();
            createWifiSettingsScreen(); // Refresh the screen
        }, LV_EVENT_CLICKED, NULL);
    }
    
    // Back button
    lv_obj_t* back_btn = lv_btn_create(btn_container);
    lv_obj_set_size(back_btn, 200, 40);
    lv_obj_add_style(back_btn, &btn_style, 0);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_scr_load_anim(mainScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
        lv_timer_t* timer = lv_timer_create([](lv_timer_t* timer) {
            if (wifiSettingsScreen != NULL) {
                lv_obj_del(wifiSettingsScreen);
                wifiSettingsScreen = NULL;
            }
            lv_timer_del(timer);
        }, 600, NULL);
    }, LV_EVENT_CLICKED, NULL);
    
    lv_scr_load_anim(wifiSettingsScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, false);
}

void createWifiScanScreen() {
    if (wifiScanScreen != NULL) {
        lv_obj_del(wifiScanScreen);
    }
    
    wifiScanScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(wifiScanScreen, lv_color_hex(0x000000), 0);
    
    // Create title
    lv_obj_t* title = lv_label_create(wifiScanScreen);
    lv_label_set_text(title, "Available Networks");
    lv_obj_add_style(title, &style_wifi_info, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    // Create loading label
    lv_obj_t* loading_label = lv_label_create(wifiScanScreen);
    lv_label_set_text(loading_label, "Scanning...");
    lv_obj_add_style(loading_label, &style_wifi_info, 0);
    lv_obj_align(loading_label, LV_ALIGN_CENTER, 0, 0);
    
    // Create scrollable container for networks
    lv_obj_t* cont = lv_obj_create(wifiScanScreen);
    lv_obj_set_size(cont, 280, 180);
    lv_obj_align(cont, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(cont, 8, 0);
    lv_obj_set_scrollbar_mode(cont, LV_SCROLLBAR_MODE_ACTIVE);
    lv_obj_clear_flag(cont, LV_OBJ_FLAG_SCROLLABLE); // Will enable after scan
    
    // Back button
    lv_obj_t* back_btn = lv_btn_create(wifiScanScreen);
    lv_obj_set_size(back_btn, 100, 40);
    lv_obj_add_style(back_btn, &btn_style, 0);
    lv_obj_align(back_btn, LV_ALIGN_BOTTOM_MID, 0, -10);
    lv_obj_t* back_label = lv_label_create(back_btn);
    lv_label_set_text(back_label, "Back");
    lv_obj_center(back_label);
    lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
        lv_scr_load_anim(wifiSettingsScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
        lv_timer_t* timer = lv_timer_create([](lv_timer_t* timer) {
            if (wifiScanScreen != NULL) {
                lv_obj_del(wifiScanScreen);
                wifiScanScreen = NULL;
            }
            lv_timer_del(timer);
        }, 600, NULL);
    }, LV_EVENT_CLICKED, NULL);
    
    lv_scr_load_anim(wifiScanScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, false);
    
    // Start WiFi scan after screen is shown
    lv_timer_t* scan_timer = lv_timer_create([](lv_timer_t* timer) {
        WiFi.mode(WIFI_STA);
        WiFi.disconnect();
        delay(100);
        
        int n = WiFi.scanNetworks();
        
        // Remove loading label
        lv_obj_t* loading = lv_obj_get_child(wifiScanScreen, 1); // Index 1 should be the loading label
        if (loading != NULL) {
            lv_obj_del(loading);
        }
        
        // Get container
        lv_obj_t* cont = lv_obj_get_child(wifiScanScreen, 1); // Now the container is at index 1
        
        if (n == 0) {
            lv_obj_t* no_networks = lv_label_create(cont);
            lv_label_set_text(no_networks, "No networks found");
            lv_obj_set_width(no_networks, lv_pct(100));
            lv_obj_set_style_text_align(no_networks, LV_TEXT_ALIGN_CENTER, 0);
        } else {
            for (int i = 0; i < n; ++i) {
                lv_obj_t* btn = lv_btn_create(cont);
                lv_obj_set_width(btn, lv_pct(100));
                lv_obj_set_height(btn, 60);
                lv_obj_add_style(btn, &btn_style, 0);
                
                char* ssid = strdup(WiFi.SSID(i).c_str());
                if (ssid != NULL) {
                    lv_obj_add_event_cb(btn, [](lv_event_t* e) {
                        lv_obj_t* btn = lv_event_get_target(e);
                        char* ssid = (char*)lv_obj_get_user_data(btn);
                        if (ssid != NULL) {
                            wifiConnectCallback(ssid);
                            free(ssid);
                        }
                    }, LV_EVENT_CLICKED, ssid);
                    lv_obj_set_user_data(btn, ssid);
                    
                    lv_obj_t* info_cont = lv_obj_create(btn);
                    lv_obj_set_size(info_cont, lv_pct(100), LV_SIZE_CONTENT);
                    lv_obj_set_style_pad_all(info_cont, 5, 0);
                    lv_obj_set_style_bg_opa(info_cont, LV_OPA_TRANSP, 0);
                    lv_obj_set_flex_flow(info_cont, LV_FLEX_FLOW_COLUMN);
                    
                    lv_obj_t* ssid_label = lv_label_create(info_cont);
                    lv_label_set_text(ssid_label, ssid);
                    
                    lv_obj_t* info_label = lv_label_create(info_cont);
                    String info = String("Strength: ") + String(WiFi.RSSI(i)) + " dBm";
                    info += WiFi.encryptionType(i) == WIFI_AUTH_OPEN ? " (Open)" : " (Secured)";
                    lv_label_set_text(info_label, info.c_str());
                }
            }
        }
        
        // Enable scrolling
        lv_obj_add_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
        
        // Clean up timer
        lv_timer_del(timer);
    }, 0, NULL);
    lv_timer_set_repeat_count(scan_timer, 1);
}

void wifiConnectCallback(const char* ssid) {
    // Create password entry screen
    lv_obj_t* passScreen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(passScreen, lv_color_hex(0x000000), 0);
    
    // Title
    lv_obj_t* title = lv_label_create(passScreen);
    lv_label_set_text(title, WiFi.encryptionType(WiFi.scanComplete()) == WIFI_AUTH_OPEN ? 
                     "Connecting..." : "Enter Password");
    lv_obj_add_style(title, &style_wifi_info, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 10);
    
    if (WiFi.encryptionType(WiFi.scanComplete()) != WIFI_AUTH_OPEN) {
        // Password input
        lv_obj_t* ta = lv_textarea_create(passScreen);
        lv_obj_set_size(ta, 280, 40);
        lv_obj_align(ta, LV_ALIGN_TOP_MID, 0, 50);
        lv_textarea_set_password_mode(ta, true);
        
        // Connect button
        lv_obj_t* connect_btn = lv_btn_create(passScreen);
        lv_obj_set_size(connect_btn, 120, 40);
        lv_obj_add_style(connect_btn, &btn_style, 0);
        lv_obj_align(connect_btn, LV_ALIGN_TOP_MID, 0, 100);
        lv_obj_t* connect_label = lv_label_create(connect_btn);
        lv_label_set_text(connect_label, "Connect");
        lv_obj_center(connect_label);
        
        // Keyboard
        lv_obj_t* kb = lv_keyboard_create(passScreen);
        lv_obj_set_size(kb, SCREEN_WIDTH, 140);
        lv_obj_align(kb, LV_ALIGN_BOTTOM_MID, 0, 0);
        lv_keyboard_set_textarea(kb, ta);
        
        // Connect button callback
        struct ConnectData {
            const char* ssid;
            lv_obj_t* ta;
        };
        ConnectData* data = new ConnectData{ssid, ta};
        lv_obj_add_event_cb(connect_btn, [](lv_event_t* e) {
            ConnectData* data = (ConnectData*)lv_obj_get_user_data(lv_event_get_target(e));
            const char* password = lv_textarea_get_text(data->ta);
            
            WiFi.begin(data->ssid, password);
            lv_obj_t* status = lv_label_create(lv_scr_act());
            lv_label_set_text(status, "Connecting...");
            lv_obj_align(status, LV_ALIGN_CENTER, 0, 0);
            
            lv_timer_t* timer = lv_timer_create([](lv_timer_t* timer) {
                if (WiFi.status() == WL_CONNECTED) {
                    createWifiSettingsScreen();
                    lv_timer_del(timer);
                }
            }, 500, NULL);
            lv_timer_set_repeat_count(timer, 20); // 10 second timeout
            
            delete data;
        }, LV_EVENT_CLICKED, data);
        
        // Back button
        lv_obj_t* back_btn = lv_btn_create(passScreen);
        lv_obj_set_size(back_btn, 100, 40);
        lv_obj_add_style(back_btn, &btn_style, 0);
        lv_obj_align(back_btn, LV_ALIGN_TOP_RIGHT, -10, 10);
        lv_obj_t* back_label = lv_label_create(back_btn);
        lv_label_set_text(back_label, "Back");
        lv_obj_center(back_label);
        lv_obj_add_event_cb(back_btn, [](lv_event_t* e) {
            createWifiScanScreen();
        }, LV_EVENT_CLICKED, NULL);
        
    } else {
        // For open networks, connect directly
        WiFi.begin(ssid);
        lv_timer_t* timer = lv_timer_create([](lv_timer_t* timer) {
            if (WiFi.status() == WL_CONNECTED) {
                createWifiSettingsScreen();
                lv_timer_del(timer);
            }
        }, 500, NULL);
        lv_timer_set_repeat_count(timer, 20); // 10 second timeout
    }
    
    lv_scr_load_anim(passScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, false);
}