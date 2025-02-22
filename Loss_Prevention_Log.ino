#include <Wire.h>

#include <M5Unified.h>
#include <M5GFX.h>
#include "lv_conf.h"
#include <lvgl.h>

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

// Menu options
const char* genders[] = {"Male", "Female"};
const char* colors[] = {"White", "Black", "Blue", "Yellow", "Green", "Orange", "Pink", "Red", "Grey"};
const char* items[] = {"Jewelry", "Women's Shoes", "Men's Shoes", "Cosmetics", "Fragrances", "Home", "Kids"};

// LVGL objects
lv_obj_t *mainScreen, *menuScreen, *genderMenu, *shirtMenu, *pantsMenu, *shoeMenu, *itemMenu, *confirmScreen;
String currentEntry = "";

// Function prototypes
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p);
void my_touch_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data);
void createMainMenu();
void createGenderMenu();
void createColorMenu(const char* title, lv_obj_t** menu, void (*nextMenuFunc)());
void colorMenuCallback(lv_event_t *e);
void createItemMenu();
void createConfirmScreen();

void setup() {
    auto cfg = M5.config();
    M5.begin(cfg); // Initialize with M5Unified
    M5.Display.setColorDepth(16);
    Serial.begin(115200);
    delay(100); // Give Serial time to initialize
    Serial.println("M5Unified initialized");

    // Initialize LVGL
    lv_init();
    Serial.println("LVGL initialized");

    // Initialize display buffer
    lv_disp_draw_buf_init(&draw_buf, buf1, NULL, SCREEN_WIDTH * 10);
    Serial.println("Display buffer initialized");

    // Initialize display driver
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    Serial.println("Display driver registered");

    // Initialize touch input driver
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touch_read;
    lv_indev_drv_register(&indev_drv);
    Serial.println("Touch driver registered");

    // Setup UI
    createMainMenu();
    Serial.println("Setup completed");
}

void loop() {
    M5.update(); // Update M5Unified states (touch, etc.)
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

// Main Menu
void createMainMenu() {
    mainScreen = lv_obj_create(NULL);
    lv_scr_load(mainScreen);

    lv_obj_t *btnNew = lv_btn_create(mainScreen);
    lv_obj_align(btnNew, LV_ALIGN_CENTER, 0, -20);
    lv_obj_t *labelNew = lv_label_create(btnNew);
    lv_label_set_text(labelNew, "Create New Entry");
    lv_obj_add_event_cb(btnNew, [](lv_event_t *e) { createGenderMenu(); }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btnView = lv_btn_create(mainScreen);
    lv_obj_align(btnView, LV_ALIGN_CENTER, 0, 20);
    lv_obj_t *labelView = lv_label_create(btnView);
    lv_label_set_text(labelView, "View Latest Entries");
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
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 10);

    for (int i = 0; i < 9; i++) {
        lv_obj_t *btn = lv_btn_create(*menu);
        lv_obj_set_pos(btn, (i % 3) * 100 + 10, (i / 3) * 60 + 50);
        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, colors[i]);
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
    lv_label_set_text(label, ("Entry: " + currentEntry).c_str());
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 20);

    lv_obj_t *btnConfirm = lv_btn_create(confirmScreen);
    lv_obj_align(btnConfirm, LV_ALIGN_BOTTOM_LEFT, 20, -20);
    lv_obj_t *lblConfirm = lv_label_create(btnConfirm);
    lv_label_set_text(lblConfirm, "Confirm");
    lv_obj_add_event_cb(btnConfirm, [](lv_event_t *e) {
        createMainMenu();
    }, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btnCancel = lv_btn_create(confirmScreen);
    lv_obj_align(btnCancel, LV_ALIGN_BOTTOM_RIGHT, -20, -20);
    lv_obj_t *lblCancel = lv_label_create(btnCancel);
    lv_label_set_text(lblCancel, "Cancel");
    lv_obj_add_event_cb(btnCancel, [](lv_event_t *e) { createMainMenu(); }, LV_EVENT_CLICKED, NULL);
}