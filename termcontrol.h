#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#ifndef TERMCONTROL_H
#define TERMCONTROL_H

static inline void goto_position(int row, int col) {
    printf("\x1b[%d;%dH", row, col);
}

static inline void disable_alternate_screen() {
    printf("\x1b[?1049l");
}

static inline void enable_alternate_screen(bool disable_at_exit) {
    printf("\x1b[?1049h");
    if (disable_at_exit) atexit(disable_alternate_screen);
}

static inline void clear_screen() {
    printf("\x1b[2J");
}

static inline void hide_cursor() {
    printf("\x1b[?25l");
}

static inline void show_cursor() {
    printf("\x1b[?25h");
}

static inline void clear_after_cursor() {
    printf("\x1b[J");
}

static inline void set_foreground_color(uint8_t r, uint8_t g, uint8_t b) {
    printf("\x1b[38;2;%u;%u;%um", r, g, b);
}

static inline void set_background_color(uint8_t r, uint8_t g, uint8_t b) {
    printf("\x1b[48;2;%u;%u;%um", r, g, b);
}

#endif
