#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef XTERM_H
#define XTERM_H

void goto_position(int row, int col) {
    printf("\x1b[%d;%dH", row, col);
}

void disable_alternate_screen() {
    printf("\x1b[?1049l");
}

void enable_alternate_screen() {
    printf("\x1b[?1049h");
    atexit(disable_alternate_screen);
}

void clear_screen() {
    printf("\x1b[2J");
}

void hide_cursor() {
    printf("\x1b[?25l");
}

void show_cursor() {
    printf("\x1b[?25h");
}

void clear_after_cursor() {
    printf("\x1b[J");
}

void set_foreground_color(uint8_t r, uint8_t g, uint8_t b) {
    printf("\x1b[38;2;%u;%u;%um", r, g, b);
}

void set_background_color(uint8_t r, uint8_t g, uint8_t b) {
    printf("\x1b[48;2;%u;%u;%um", r, g, b);
}

#endif
