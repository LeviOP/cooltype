#include <pthread.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <termios.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/ioctl.h>
#include "xterm.h"

#define STR_MAX_LEN 1000
#define DIRTY_DELTA 100

#define TS_TO_MSEC(ts) ((ts).tv_sec * 1000 + (ts).tv_nsec / 1000000)
#define CTRL_KEY(k) ((k) & 0x1f)

struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode(bool disable_at_exit) {
    tcgetattr(STDIN_FILENO, &orig_termios);

    struct termios raw = orig_termios;
    cfmakeraw(&raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    if (disable_at_exit) atexit(disable_raw_mode);
}

typedef struct character_node {
    struct character_node* next;
    struct character_node* prev;
    int y;
    int x;
    char c;
    long dirty_time;
} character_node;

character_node* head = NULL;
struct timespec ts;
struct ttysize tsize;

typedef struct {
    char c;
} sky_cell;

sky_cell** last_sky = NULL;
int sky_height;
int sky_width;
bool sky_dirty = false;

char string[STR_MAX_LEN+1] = "";

void add_character(char c, int col) {
    clock_gettime(CLOCK_REALTIME, &ts);
    character_node* character = (character_node*)malloc(sizeof(character_node));

    if (head == NULL) {
        character->next = NULL;
    } else {
        character->next = head;
        head->prev = character;
    }

    character->prev = NULL;
    character->c = c;
    character->y = 1;
    character->x = col;
    character->dirty_time = TS_TO_MSEC(ts) + DIRTY_DELTA;

    head = character;
}

void remove_character(character_node* character) {
    if (character == head) {
        head = character->next;
    }

    if (character->next) {
        character->next->prev = character->prev;
    }

    if (character->prev) {
        character->prev->next = character->next;
    }

    free(character);
}

void advance_character(character_node* character, long time) {
    if (character->y >= sky_height - 1) {
        remove_character(character);
    } else {
        character->y++;

        int dir = rand() % 3;
        switch(dir) {
            case 1:
                character->x--;
                break;
            case 2:
                character->x++;
                break;
        }

        character->dirty_time = time + DIRTY_DELTA;
    }
}

void setup_screen_background() {
    int len = tsize.ts_lines * tsize.ts_cols;
    char output[len + 1];
    memset(output, ' ', len * sizeof(char));
    output[len] = '\0';

    goto_position(1, 1);
    set_background_color(0, 0, 0);
    fputs(output, stdout);
    fflush(stdout);
}

void create_sky(sky_cell*** s, int height, int width) {
    *s = (sky_cell**)malloc(height*sizeof(sky_cell*));
    for (int i = 0; i < height; i++) {
        (*s)[i] = (sky_cell*)malloc(width*sizeof(sky_cell));
        for (int j = 0; j < width; j++) {
            (*s)[i][j].c = 0;
        }
    }
}

void destory_sky(sky_cell*** s, int height) {
    for (int i = 0; i < height; i++)
        free((*s)[i]);
    free(*s);
}

void draw_sky() {
    sky_cell** sky;
    create_sky(&sky, sky_height, sky_width);

    for (character_node* current = head; current != NULL; current = current->next) {
        int x = current->x;
        if (x < 0) continue;

        sky[current->y][x].c = current->c;
    }

    hide_cursor();

    for (int y = 0; y < tsize.ts_lines; y++) {
        int b = ((double)(tsize.ts_lines - y) / tsize.ts_lines) * 255;
        for (int x = 0; x < tsize.ts_cols; x++) {
            char c = sky[y][x].c;
            if (last_sky[y][x].c == c) continue;
            goto_position(tsize.ts_lines - y, x + 1);
            if (c) {
                set_foreground_color(b, b, b);
                printf("%c", c);
            } else {
                printf(" ");
            }
        }
    }

    goto_position(tsize.ts_lines, strlen(string) + 1);
    show_cursor();
    fflush(stdout);

    destory_sky(&last_sky, sky_height);
    last_sky = sky;
}

void draw_input_line() {
    hide_cursor();
    goto_position(tsize.ts_lines, 1);
    set_foreground_color(255, 255, 255);
    fputs(string, stdout);
    show_cursor();
    clear_after_cursor();
    fflush(stdout);
}

void delete_word(char* str) {
    char* till = str;
    while (*str != '\0') {
        if (*str == ' ') till = str;
        str++;
    }
    while (str != till) {
        str--;
        *str = '\0';
    }
}

void strcat_c(char* str, char c) {
    for (;*str;str++);
    *str++ = c;
    *str++ = 0;
}

void read_keys() {
    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == CTRL_KEY('c')) exit(0);

        if (c == CTRL_KEY('w')) {
            delete_word(string);
            goto draw;
        }

        if (c < 32) continue;

        int len = strlen(string);

        if (c == 127) {
            if (len != 0) string[len-1] = '\0';
        } else {
            if (len == STR_MAX_LEN) continue;
            strcat_c(string, c);
            if (c != ' ') {
                add_character(c, len);
                sky_dirty = true;
            }
        }
draw:
        draw_input_line();
    }
}

void process_characters() {
    clock_gettime(CLOCK_REALTIME, &ts);

    character_node* current = head;
    while (current != NULL) {
        character_node* next = current->next;

        long msec = TS_TO_MSEC(ts);
        if (current->dirty_time < msec) {
            advance_character(current, msec);
            sky_dirty = true;
        }
        current = next;
    }
}

void screen_reset() {
    hide_cursor();

    setup_screen_background();
    draw_input_line();
    draw_sky();
}

void updateScreenSize() {
    ioctl(STDIN_FILENO, TIOCGSIZE, &tsize);

    // Only create a new sky if the old sky was too small
    if (tsize.ts_lines > sky_height || tsize.ts_cols > sky_width) {
        sky_cell** sky;
        create_sky(&sky, tsize.ts_lines, tsize.ts_cols);

        destory_sky(&last_sky, sky_height);
        last_sky = sky;

        sky_height = tsize.ts_lines;
        sky_width = tsize.ts_cols;
    }

    screen_reset();
}

void sky_init() {
    sky_height = tsize.ts_lines;
    sky_width = tsize.ts_cols;

    create_sky(&last_sky, sky_height, sky_width);
}

void terminal_init() {
    enable_raw_mode(true);
    enable_alternate_screen(true);

    ioctl(STDIN_FILENO, TIOCGSIZE, &tsize);
}

int main() {
    srand(time(NULL));

    terminal_init();
    sky_init();
    screen_reset();

    signal(SIGWINCH, updateScreenSize);

    while (1) {
        read_keys();
        process_characters();
        if (sky_dirty) {
            draw_sky();
            sky_dirty = false;
        }
    }
}
