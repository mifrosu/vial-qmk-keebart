#include QMK_KEYBOARD_H

#ifdef OLED_ENABLE
#include "bitmaps.h"
#endif

enum layer_names {
    _BASE   = 0,
    _LOWER  = 1,
    _RAISE  = 2,
    _ADJUST = 3
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT_split_3x5_3(
        //,--------------------------------------------.  ,--------------------------------------------.
               KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,       KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,
        //|--------+--------+--------+--------+--------|  |--------+--------+--------+--------+--------|
               KC_A,    KC_S,    KC_D,    KC_F,    KC_G,       KC_H,    KC_J,    KC_K,    KC_L, KC_SCLN,
        //|--------+--------+--------+--------+--------'  `--------+--------+--------+--------+--------|
               KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,       KC_N,    KC_M, KC_COMM,  KC_DOT, KC_SLSH,
        //|--------+--------+--------+--------+--------.  ,--------+--------+--------+--------+--------|
                              KC_LGUI, TL_LOWR,  KC_SPC,     KC_ENT, TL_UPPR, KC_RGUI
                          //`--------------------------'  `--------------------------'
    ),

    [_LOWER] = LAYOUT_split_3x5_3(
        //,--------------------------------------------.  ,--------------------------------------------.
               KC_1,    KC_2,    KC_3,    KC_4,    KC_5,       KC_6,    KC_7,    KC_8,    KC_9,    KC_0,
        //|--------+--------+--------+--------+--------|  |--------+--------+--------+--------+--------|
            KC_LCTL, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,    KC_LEFT, KC_DOWN,   KC_UP,KC_RIGHT, XXXXXXX,
        //|--------+--------+--------+--------+--------'  `--------+--------+--------+--------+--------|
            KC_LSFT, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,    XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
        //|--------+--------+--------+--------+--------.  ,--------+--------+--------+--------+--------|
                              KC_LGUI, _______,  KC_SPC,     KC_ENT, _______, KC_RGUI
                          //`--------------------------'  `--------------------------'
    ),

    [_RAISE] = LAYOUT_split_3x5_3(
        //,--------------------------------------------.  ,--------------------------------------------.
            KC_EXLM,   KC_AT, KC_HASH,  KC_DLR, KC_PERC,    KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN,
        //|--------+--------+--------+--------+--------|  |--------+--------+--------+--------+--------|
            KC_LCTL, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,    KC_MINS,  KC_EQL, KC_LBRC, KC_RBRC, KC_BSLS,
        //|--------+--------+--------+--------+--------'  `--------+--------+--------+--------+--------|
            KC_LSFT, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,    KC_UNDS, KC_PLUS, KC_LCBR, KC_RCBR, KC_PIPE,
        //|--------+--------+--------+--------+--------.  ,--------+--------+--------+--------+--------|
                              KC_LGUI, _______,  KC_SPC,     KC_ENT, _______, KC_RGUI
                          //`--------------------------'  `--------------------------'
    ),

    [_ADJUST] = LAYOUT_split_3x5_3(
        //,--------------------------------------------.  ,--------------------------------------------.
            QK_BOOT, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,    XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
        //|--------+--------+--------+--------+--------|  |--------+--------+--------+--------+--------|
            RGB_TOG, RGB_HUI, RGB_SAI, RGB_VAI, XXXXXXX,    XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
        //|--------+--------+--------+--------+--------'  `--------+--------+--------+--------+--------|
            RGB_MOD, RGB_HUD, RGB_SAD, RGB_VAD, XXXXXXX,    XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX, XXXXXXX,
        //|--------+--------+--------+--------+--------.  ,--------+--------+--------+--------+--------|
                              KC_LGUI, _______,  KC_SPC,     KC_ENT, _______, KC_RGUI
                          //`--------------------------'  `--------------------------'
    ),
};

#ifdef OLED_ENABLE

#include "transactions.h"

static const uint8_t OLED_WIDTH = OLED_DISPLAY_HEIGHT;
static const uint16_t SPLASH_DURATION_MS = 2500;

typedef struct {
    bool oled_on;
} oled_state_m2s_t;

typedef struct {
    uint32_t left;
    uint32_t right;
} presses_m2s_t;

static bool g_oled_init_done = false;
static uint8_t g_oled_max_char;
static uint8_t g_oled_max_line;
static bool g_splash_active = true;
static bool g_splash_rendered = false;
static uint32_t g_splash_start_ms = 0;
static uint32_t g_user_ontime = 0;
static uint32_t g_press_left = 0;
static uint32_t g_press_right = 0;
static oled_state_m2s_t g_remote_oled_state = { false };
static presses_m2s_t g_remote_presses = {0, 0};

static inline pin_t get_charge_pump_enable_pin(void) {
    if (is_keyboard_left()) {
        return GP5;
    } else {
        return GP25;
    }
}

void oled_blit_16x16_P(const char *icon, uint8_t x, uint8_t page) {
    for (uint8_t i = 0; i < 16; i++) {
        char top = pgm_read_byte(&icon[i]);         // column i, top 8 pixels
        char bot = pgm_read_byte(&icon[16 + i]);    // column i, bottom 8 pixels

        oled_write_raw_byte(top, page       * OLED_WIDTH + x + i);
        oled_write_raw_byte(bot, (page + 1) * OLED_WIDTH + x + i);
    }
}

void oled_blit_24x24_P(const char *icon, uint8_t x, uint8_t page) {
    for (uint8_t i = 0; i < 24; i++) {
        char top = pgm_read_byte(&icon[i]);         // column i, top 8 pixels
        char mid = pgm_read_byte(&icon[24 + i]);    // column i, middle 8 pixels
        char bot = pgm_read_byte(&icon[48 + i]);    // column i, bottom 8 pixels

        oled_write_raw_byte(top, page       * OLED_WIDTH + x + i);
        oled_write_raw_byte(mid, (page + 1) * OLED_WIDTH + x + i);
        oled_write_raw_byte(bot, (page + 2) * OLED_WIDTH + x + i);
    }
}

void oled_blit_P(const char *img, uint8_t w, uint8_t pages, uint8_t x, uint8_t page) {
    for (uint8_t p = 0; p < pages; p++) {
        for (uint8_t i = 0; i < w; i++) {
            oled_write_raw_byte(pgm_read_byte(&img[p * w + i]), (page + p) * OLED_WIDTH + x + i);
        }
    }
}

// WPM history bar chart: one bar per sample, scrolling left as new samples arrive
#define WPM_HIST_LEN 32
#define WPM_BAR_W 2
#define WPM_CHART_PAGES 4
#define WPM_CHART_H (WPM_CHART_PAGES * 8)
#define WPM_SAMPLE_MS 2000
#define WPM_MIN_SCALE 40
static uint8_t g_wpm_hist[WPM_HIST_LEN];
static uint32_t g_wpm_sample_ms = 0;

void wpm_hist_update(void) {
    if (timer_elapsed32(g_wpm_sample_ms) < WPM_SAMPLE_MS) {
        return;
    }
    g_wpm_sample_ms = timer_read32();
    memmove(g_wpm_hist, g_wpm_hist + 1, WPM_HIST_LEN - 1);
    const uint8_t wpm = get_current_wpm();
    g_wpm_hist[WPM_HIST_LEN - 1] = wpm;
}

void render_wpm_chart(uint8_t page) {
    uint8_t scale = WPM_MIN_SCALE;
    for (uint8_t i = 0; i < WPM_HIST_LEN; i++) {
        if (g_wpm_hist[i] > scale) {
            scale = g_wpm_hist[i];
        }
    }
    for (uint8_t i = 0; i < WPM_HIST_LEN; i++) {
        uint8_t h = (uint16_t)g_wpm_hist[i] * WPM_CHART_H / scale;
        if (h < 1) {
            h = 1; // baseline
        }
        const uint8_t start = WPM_CHART_H - h; // first filled row from the top
        for (uint8_t p = 0; p < WPM_CHART_PAGES; p++) {
            const uint8_t y0 = p * 8;
            uint8_t mask;
            if (start >= y0 + 8) {
                mask = 0x00;
            } else if (start <= y0) {
                mask = 0xFF;
            } else {
                mask = (uint8_t)(0xFF << (start - y0));
            }
            for (uint8_t w = 0; w < WPM_BAR_W; w++) {
                oled_write_raw_byte(mask, (page + p) * OLED_WIDTH + i * WPM_BAR_W + w);
            }
        }
    }
}

void render_dog(uint8_t x, uint8_t page) {
    const uint8_t wpm = get_current_wpm();
    uint8_t frame;
    if (wpm < 5) {
        frame = (timer_read32() / 600u) & 1u;           // sit, tail wag / blink
    } else if (wpm < 40) {
        frame = 2 + ((timer_read32() / 250u) & 1u);     // walk
    } else {
        frame = 4 + ((timer_read32() / 120u) & 1u);     // run
    }
    oled_blit_P(DOG_FRAMES[frame], DOG_SPRITE_W, DOG_SPRITE_PAGES, x, page);
}

uint16_t get_current_dwpm(void) {
    const uint8_t wpm = get_current_wpm();
    uint16_t dwpm = (uint16_t)wpm * 10u;
    return dwpm;
}

uint8_t round_percentage(float x) {
    float f = x + 0.5f;
    uint8_t r = (uint8_t)f;
    if ((f - (float)r) == 0.0f && (r & 1)) {
        r--; // round half to even
    }
    return r;
}

void oled_print_right_aligned(const char *text, const uint8_t width) {
    uint8_t len = strlen(text);
    uint8_t pad = (len < width) ? (width - len) : 0;
    for (uint8_t i = 0; i < pad; i++) {
        oled_write_P(PSTR(" "), false);
    }
    oled_write(text, false);
}

void print_current_layer(uint8_t row) {
    char layer_str[8];
    switch (get_highest_layer(layer_state)) {
        case _BASE:
            strcpy(layer_str, "Base");
            break;
        case _LOWER:
            strcpy(layer_str, "Lower");
            break;
        case _RAISE:
            strcpy(layer_str, "Raise");
            break;
        case _ADJUST:
            strcpy(layer_str, "Adjust");
            break;
        default:
            // TODO: consider remove snprintf
            snprintf(layer_str, sizeof(layer_str), "%d", get_highest_layer(layer_state));
    }

    oled_set_cursor(0, row);
    oled_print_right_aligned(layer_str, g_oled_max_char);
}

void print_uptime(uint8_t row) {
    uint32_t time_ms = timer_read32();
    uint32_t total_min = time_ms / 60000u;
    uint32_t hours = total_min / 60u;
    uint32_t minutes = total_min % 60u;
    if (hours > 999u) {
        hours = 999u;
        minutes = 59u;
    }

    // TODO: consider remove snprintf
    char buf[8];
    snprintf(buf, sizeof(buf), "%3luh%02lum", hours, minutes);
    oled_set_cursor(0, row);
    oled_print_right_aligned(buf, g_oled_max_char);
}

void print_wpm(uint8_t row) {
    uint16_t wpm = get_current_dwpm();
    uint16_t wpm_int = wpm / 10u;
    uint16_t wpm_frac = wpm % 10u;

    // TODO: consider remove snprintf
    char buf[11];
    snprintf(buf, sizeof(buf), "%3u.%1u WPM", wpm_int, wpm_frac);
    oled_set_cursor(0, row);
    oled_print_right_aligned(buf, g_oled_max_char);
}

void print_balance(uint8_t row, uint8_t pct) {
    // TODO: consider remove snprintf
    char buf[6];
    snprintf(buf, sizeof(buf), "%3u %%", pct);
    oled_set_cursor(0, row);
    oled_print_right_aligned(buf, g_oled_max_char);
}

void render_splash(void) {
    if (g_splash_rendered) {
        return;
    }
    oled_clear();
    oled_set_cursor(0, 0);
    oled_write_raw_P(STARTUP_BITMAP, sizeof(STARTUP_BITMAP));
    g_splash_rendered = true;
}

static void user_sync_oled_state_slave(uint8_t in_len, const void* in_data,
                                       uint8_t out_len, void* out_data) {
    if (in_len >= sizeof(oled_state_m2s_t)) {
        memcpy(&g_remote_oled_state, in_data, sizeof(oled_state_m2s_t));
    }
}

static void user_sync_presses_slave(uint8_t in_len, const void* in_data,
                                    uint8_t out_len, void* out_data) {
    if (in_len >= sizeof(presses_m2s_t)) {
        memcpy(&g_remote_presses, in_data, sizeof(presses_m2s_t));
    }
}

void keyboard_post_init_user(void) {
    pin_t dsp_pen_pin = get_charge_pump_enable_pin();
    gpio_set_pin_output(dsp_pen_pin);
    gpio_write_pin_low(dsp_pen_pin);
    wait_ms(5);

    transaction_register_rpc(USER_SYNC_OLED_STATE, user_sync_oled_state_slave);
    transaction_register_rpc(USER_SYNC_PRESSES, user_sync_presses_slave);

    if (!is_keyboard_master()) {
        wait_ms(90); // wait for master to be ready
    }
}

void housekeeping_task_user(void) {
    if (is_keyboard_master()) {
        static uint32_t last_sync = 0;
        if (timer_elapsed32(last_sync) > 500) {
            oled_state_m2s_t oled_state_pkt = { is_oled_on() };
            (void)transaction_rpc_send(
                USER_SYNC_OLED_STATE, sizeof(oled_state_pkt), &oled_state_pkt
            );
            last_sync = timer_read32();
        }
    }
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    g_user_ontime = timer_read32();

    if (record->event.pressed) {
        uint8_t row = record->event.key.row;
        if (row < MATRIX_ROWS / 2) {
            g_press_left++;
        } else {
            g_press_right++;
        }

        if (is_keyboard_master()) {
            presses_m2s_t presses_pkt = { g_press_left, g_press_right };
            (void)transaction_rpc_send(USER_SYNC_PRESSES, sizeof(presses_pkt), &presses_pkt);
        }
    }
    return true;
}

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    return OLED_ROTATION_90;
}

bool oled_post_init(void) {
    if (!g_oled_init_done) {
        // set OLED size for characters
        g_oled_max_char = oled_max_chars();
        g_oled_max_line = oled_max_lines();

        // enable charge pump
        pin_t dsp_pen_pin = get_charge_pump_enable_pin();
        gpio_write_pin_high(dsp_pen_pin);
        wait_ms(20);
        oled_clear();

        // start timer for splash screen
        g_splash_start_ms = timer_read32();
        g_splash_active = true;

        g_oled_init_done = true;
    }
    return false;
}

bool oled_task_user(void) {
    // perform custom initialisation once
    oled_post_init();

    // render splash screen
    if (g_splash_active) {
        if (timer_elapsed32(g_splash_start_ms) > SPLASH_DURATION_MS) {
            g_splash_active = false;
            oled_clear();
            g_user_ontime = timer_read32();
        } else {
            render_splash();
        }
        return false;
    }

    // sync key presses
    uint32_t local_presses_left, local_presses_right;
    if (is_keyboard_master()) {
        local_presses_left = g_press_left;
        local_presses_right = g_press_right;
    } else {
        local_presses_left = g_remote_presses.left;
        local_presses_right = g_remote_presses.right;
    }

    // manage oled on/off state based on idle time
    if (is_keyboard_master()) {
        const uint32_t idle_time = timer_elapsed32(g_user_ontime);
        if (!is_oled_on()) {
            if (idle_time > OLED_TIMEOUT_USER) {
                return false; // stay off
            } else {
                oled_on();
                oled_state_m2s_t oled_state_pkt = { true };
                (void)transaction_rpc_send(
                    USER_SYNC_OLED_STATE, sizeof(oled_state_pkt), &oled_state_pkt
                );
            }
        } else {
            if (idle_time > OLED_TIMEOUT_USER) {
                oled_off();
                oled_state_m2s_t oled_state_pkt = { false };
                (void)transaction_rpc_send(
                    USER_SYNC_OLED_STATE, sizeof(oled_state_pkt), &oled_state_pkt
                );
                return false;
            } else {
                // stay on
            }
        }
    } else {
        if (g_remote_oled_state.oled_on) {
            if (!is_oled_on()) {
                oled_on();
            } else {
                // stay on
            }
        } else {
            if (is_oled_on()) {
                oled_off();
                return false;
            } else {
                return false;
                // stay off
            }
        }
    }

    uint32_t total = local_presses_left + local_presses_right;
    if (total == 0) {
        total = 1;  // avoid div by 0
    }
    uint8_t pct_left = round_percentage((100.0f * local_presses_left) / total);
    uint8_t pct_right = round_percentage((100.0f * local_presses_right) / total);

    if (is_keyboard_left()) {
        // Layer state
        oled_set_cursor(0, 0);
        oled_write_P(PSTR("Layer:"), false);
        print_current_layer(1);

        // Lock status
        led_t led_state = host_keyboard_led_state();
        if (led_state.num_lock) {
            oled_blit_16x16_P(NUM_LOCK_BITMAP, 0, 3);
        } else {
            oled_blit_16x16_P(EMPTY_BITMAP, 0, 3);
        }
        if (led_state.caps_lock) {
            oled_blit_16x16_P(CAPS_LOCK_BITMAP, 24, 3);
        } else {
            oled_blit_16x16_P(EMPTY_BITMAP, 24, 3);
        }
        if (led_state.scroll_lock) {
            oled_blit_16x16_P(SCROLL_LOCK_BITMAP, 48, 3);
        } else {
            oled_blit_16x16_P(EMPTY_BITMAP, 48, 3);
        }

        // split balance
        oled_set_cursor(0, 7);
        oled_write_P(PSTR("Left:"), false);
        print_balance(8, pct_left);

        // WPM history
        wpm_hist_update();
        char wpm_buf[12];
        snprintf(wpm_buf, sizeof(wpm_buf), "WPM %3u", get_current_wpm());
        oled_set_cursor(0, 10);
        oled_print_right_aligned(wpm_buf, g_oled_max_char);
        render_wpm_chart(11);
    } else {
        // Uptime (only if oled is on)
        oled_set_cursor(0, 0);
        oled_write_P(PSTR("Uptime:"), false);
        print_uptime(1);

        // Typing speed
        oled_set_cursor(0, 3);
        oled_write_P(PSTR("Avg Speed"), false);
        oled_set_cursor(0, 4);
        oled_write_P(PSTR("(25 s):"), false);
        print_wpm(5);

        // split balance
        oled_set_cursor(0, 7);
        oled_write_P(PSTR("Right:"), false);
        print_balance(8, pct_right);

        // Dog
        render_dog(8, 12);
    }
    return false;
}

#endif // OLED_ENABLE
