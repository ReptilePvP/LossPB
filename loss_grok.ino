#include "lv_conf.h"
#include <M5Unified.h>
#include <lvgl.h>
#include <Preferences.h>
#include <SD.h>
#include <WiFi.h>
#include <time.h>

// Declare the Unscii 16 font
LV_FONT_DECLARE(lv_font_unscii_16);

// Define screen dimensions for CoreS3
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Display buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf1[SCREEN_WIDTH * 10]; // Buffer for 10 lines, as per your working project

// Display driver
static lv_disp_drv_t disp_drv;

// Global variables
Preferences prefs;
String currentEntry = "";
bool wifiConnected = false;
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = -5 * 3600; // EST offset
const int daylightOffset_sec = 0;

// Menu options
const char* genders[] = {"Male", "Female"};
const char* colors[] = {"White", "Black", "Blue", "Yellow", "Green", "Orange", "Pink", "Red", "Grey"};
const char* items[] = {"Jewelry", "Women's Shoes", "Men's Shoes", "Cosmetics", "Fragrances", "Home", "Kids"};

// LVGL objects
lv_obj_t *mainScreen, *menuScreen, *genderMenu, *shirtMenu, *pantsMenu, *shoeMenu, *itemMenu, *confirmScreen;

// Function prototypes
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void createMainMenu();
void createGenderMenu();
void createColorMenu(const char* title, lv_obj_t** menu, void (*nextMenuFunc)());
void colorMenuCallback(lv_event_t *e);
void createItemMenu();
void createConfirmScreen();
String getTimestamp();
void saveEntry(String entry);

void setup() {
  M5.begin(); // Initialize M5Stack hardware, including LCD
  M5.Lcd.setColorDepth(16); // Set to 16-bit RGB565
  
  // Initialize LVGL
  lv_init();
  
  // Initialize display buffer (single buffer for 8.4.0)
  lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * 10);
  
  // Initialize display driver
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = SCREEN_WIDTH; // 320
  disp_drv.ver_res = SCREEN_HEIGHT; // 240
  disp_drv.flush_cb = my_disp_flush; // Use your working flush function
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // Initialize SD card
  if (!SD.begin()) {
    Serial.println("SD Card Mount Failed");
  }

  // Connect to Wi-Fi (replace with your credentials)
  WiFi.begin("SSID", "PASSWORD");
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }

  // Setup UI
  createMainMenu();
}

void loop() {
  M5.update();
  lv_timer_handler();
  delay(5);
}

// Display flush function from your working project
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);
  M5.Lcd.startWrite();
  M5.Lcd.setAddrWindow(area->x1, area->y1, w, h);
  M5.Lcd.pushColors((uint16_t *)&color_p->full, w * h, true); // Swap bytes for RGB565
  M5.Lcd.endWrite();
  lv_disp_flush_ready(disp);
}

// Main Menu
void createMainMenu() {
  mainScreen = lv_obj_create(NULL);
  lv_scr_load(mainScreen);

  lv_obj_t *btnNew = lv_btn_create(mainScreen);
  lv_obj_align(btnNew, LV_ALIGN_CENTER, 0, -20);
  lv_obj_t *labelNew = lv_label_create(btnNew);
  lv_label_set_text(labelNew, "Create New Entry");
  lv_obj_set_style_text_font(labelNew, &lv_font_unscii_16, 0); // Use Unscii 16
  lv_obj_add_event_cb(btnNew, [](lv_event_t *e) { createGenderMenu(); }, LV_EVENT_CLICKED, NULL);

  lv_obj_t *btnView = lv_btn_create(mainScreen);
  lv_obj_align(btnView, LV_ALIGN_CENTER, 0, 20);
  lv_obj_t *labelView = lv_label_create(btnView);
  lv_label_set_text(labelView, "View Latest Entries");
  lv_obj_set_style_text_font(labelView, &lv_font_unscii_16, 0); // Use Unscii 16
}

// Gender Menu
void createGenderMenu() {
  genderMenu = lv_obj_create(NULL);
  lv_scr_load(genderMenu);

  for (int i = 0; i < 2; i++) {
    lv_obj_t *btn = lv_btn_create(genderMenu);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, i * 60 + 20);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, genders[i]);
    lv_obj_set_style_text_font(label, &lv_font_unscii_16, 0); // Use Unscii 16
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      currentEntry = String(lv_label_get_text(lv_obj_get_child(lv_event_get_target(e), 0))) + ",";
      createColorMenu("Shirt Color", &shirtMenu, [](){ createColorMenu("Pants Color", &pantsMenu, [](){ createColorMenu("Shoe Color", &shoeMenu, createItemMenu); }); });
    }, LV_EVENT_CLICKED, NULL);
  }
}

// Generic Color Menu
void createColorMenu(const char* title, lv_obj_t** menu, void (*nextMenuFunc)()) {
  *menu = lv_obj_create(NULL);
  lv_scr_load(*menu);

  lv_obj_t *label = lv_label_create(*menu);
  lv_label_set_text(label, title);
  lv_obj_set_style_text_font(label, &lv_font_unscii_16, 0); // Use Unscii 16
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

  for (int i = 0; i < 9; i++) {
    lv_obj_t *btn = lv_btn_create(*menu);
    lv_obj_set_pos(btn, (i % 3) * 100 + 10, (i / 3) * 60 + 50);
    lv_obj_t *lbl = lv_label_create(btn);
    lv_label_set_text(lbl, colors[i]);
    lv_obj_set_style_text_font(lbl, &lv_font_unscii_16, 0); // Use Unscii 16
    lv_obj_add_event_cb(btn, colorMenuCallback, LV_EVENT_CLICKED, (void*)nextMenuFunc);
  }
}

// Color Menu Callback
void colorMenuCallback(lv_event_t *e) {
  currentEntry += String(lv_label_get_text(lv_obj_get_child(lv_event_get_target(e), 0))) + ",";
  void (*nextMenuFunc)() = (void (*)())lv_event_get_user_data(e);
  if (nextMenuFunc) nextMenuFunc();
}

// Item Menu
void createItemMenu() {
  itemMenu = lv_obj_create(NULL);
  lv_scr_load(itemMenu);

  for (int i = 0; i < 7; i++) {
    lv_obj_t *btn = lv_btn_create(itemMenu);
    lv_obj_set_pos(btn, (i % 2) * 150 + 10, (i / 2) * 60 + 20);
    lv_obj_t *label = lv_label_create(btn);
    lv_label_set_text(label, items[i]);
    lv_obj_set_style_text_font(label, &lv_font_unscii_16, 0); // Use Unscii 16
    lv_obj_add_event_cb(btn, [](lv_event_t *e) {
      currentEntry += String(lv_label_get_text(lv_obj_get_child(lv_event_get_target(e), 0)));
      createConfirmScreen();
    }, LV_EVENT_CLICKED, NULL);
  }
}

// Confirm Screen
void createConfirmScreen() {
  confirmScreen = lv_obj_create(NULL);
  lv_scr_load(confirmScreen);

  lv_obj_t *label = lv_label_create(confirmScreen);
  lv_label_set_text(label, ("Entry: " + getTimestamp() + "," + currentEntry).c_str());
  lv_obj_set_style_text_font(label, &lv_font_unscii_16, 0); // Use Unscii 16
  lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

  lv_obj_t *btnConfirm = lv_btn_create(confirmScreen);
  lv_obj_align(btnConfirm, LV_ALIGN_BOTTOM_LEFT, 20, -20);
  lv_obj_t *lblConfirm = lv_label_create(btnConfirm);
  lv_label_set_text(lblConfirm, "Confirm");
  lv_obj_set_style_text_font(lblConfirm, &lv_font_unscii_16, 0); // Use Unscii 16
  lv_obj_add_event_cb(btnConfirm, [](lv_event_t *e) {
    saveEntry(getTimestamp() + "," + currentEntry);
    createMainMenu();
  }, LV_EVENT_CLICKED, NULL);

  lv_obj_t *btnCancel = lv_btn_create(confirmScreen);
  lv_obj_align(btnCancel, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
  lv_obj_t *lblCancel = lv_label_create(btnCancel);
  lv_label_set_text(lblCancel, "Cancel");
  lv_obj_set_style_text_font(lblCancel, &lv_font_unscii_16, 0); // Use Unscii 16
  lv_obj_add_event_cb(btnCancel, [](lv_event_t *e) { createMainMenu(); }, LV_EVENT_CLICKED, NULL);
}

// Timestamp Function
String getTimestamp() {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  char buffer[20];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

// Save Entry
void saveEntry(String entry) {
  if (wifiConnected) {
    Serial.println("Uploading to database: " + entry);
  } else {
    File file = SD.open("/logs.csv", FILE_APPEND);
    if (file) {
      file.println(entry);
      file.close();
      Serial.println("Saved to SD: " + entry);
    }
  }
  currentEntry = "";
}