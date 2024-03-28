#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>

#define STR_MAX_LEN 100

struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    // atexit(disable_raw_mode);
    struct termios raw = orig_termios;
    // raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    cfmakeraw(&raw);
    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void strcat_c(char* str, char c) {
    for (;*str;str++);
    *str++ = c;
    *str++ = 0;
}

void goto_position(int row, int col) {
    printf("\x1b[%d;%dH", row, col);
}

void disable_alternate_screen() {
    printf("\x1b[?1049l");
}

void enable_alternate_screen() {
    printf("\x1b[?1049h");
    // atexit(disable_alternate_screen);
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

void goto_home() {
    printf("\x1bH");
}

typedef struct character_node {
    struct character_node* prev;
    struct character_node* next;
    int row;
    int col;
    char c;
    long death;
} character_node;

struct ttysize ts;
char string[STR_MAX_LEN + 1] = "";
character_node* head = NULL;
pthread_mutex_t lock;

void draw_screen() {
    char output[ts.ts_lines * ts.ts_cols + 1];
    memset(output, ' ', (ts.ts_lines * ts.ts_cols) * sizeof(char));
    output[ts.ts_lines * ts.ts_cols] = '\0';

    character_node* current = head;
    while (current != NULL) {
        int index = (current->row - 1) * ts.ts_cols + current->col;
        output[index] = current->c;
        current = current->next;
    }

    for (int i = 0; string[i]; i++) {
        output[i + (ts.ts_lines - 1) * ts.ts_cols] = string[i];
    }

    hide_cursor();
    // goto_home();
    goto_position(1, 1);
    fputs(output, stdout);
    goto_position(ts.ts_lines, strlen(string) + 1);
    show_cursor();
    fflush(stdout);
}

void advance_character(character_node* character, long time) {
    if (character->row == 1) {
        if (head == character) {
            head = character->next;
        }

        if (character->next) {
            character->next->prev = character->prev;
        }

        if (character->prev) {
            character->prev->next = character->next;
        }

        free(character);
    } else {
        character->row--;
        character->death = time + 100000000;
    }
}

character_node* create_character(char c, int col) {
    character_node* character = (character_node*)malloc(sizeof(character_node));

    if (character == NULL) {
        printf("Memory allocation failed!\n");
        exit(EXIT_FAILURE);
    }

    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    character->row = ts.ts_lines - 1;
    character->col = col;
    character->c = c;
    character->death = spec.tv_nsec + 100000000;
    character->next = NULL;
    character->prev = NULL;

    return character;
}

void add_character(char c, int col) {
    character_node* character = create_character(c, col);

    if (head == NULL) {
        head = character;
    } else {
        character->next = head;
        head->prev = character;
        head = character;
    }
}

// void* start_loop() {
//     struct timespec ts;
//     ts.tv_nsec = 50 / 1000;
//     ts.tv_nsec = (50 % 1000) * 1000000;
//
//     while(1) {
//         nanosleep(&ts, NULL);
//
//         pthread_mutex_lock(&lock);
//
//         character_node* current = head;
//         while (current != NULL) {
//             advance_character(current);
//             current = current->next;
//             printf("exists?: %p\n", current);
//         }
//
//         pthread_mutex_unlock(&lock);
//
//         // draw_screen();
//     }
//
//     return NULL;
// }

int main() {
    enable_raw_mode();

    // Get terminal size
    ioctl(STDIN_FILENO, TIOCGSIZE, &ts);

    // Setup screen
    enable_alternate_screen();
    // clear_screen();
    goto_position(ts.ts_lines, 0);
    fflush(stdout);

    // if (pthread_mutex_init(&lock, NULL) != 0) {
    //     printf("mutex init has failed\n");
    //     return 1;
    // }
    //
    // pthread_t tid;
    // pthread_create(&tid, NULL, start_loop, NULL);

    char c;
    while (1) {
        bool dirty = false;

        if (read(STDOUT_FILENO, &c, 1) == 1) {
            if (c == 3) break;
            if (c != 0 && c > 31) {
                int len = strlen(string);

                if (c == 127 && len != 0) {
                    string[len-1] = '\0';
                } else {
                    if (len == STR_MAX_LEN) continue;
                    strcat_c(string, c);
                    if (c != ' ') add_character(c, len);
                }
                dirty = true;
            }
        }

        struct timespec spec;
        clock_gettime(CLOCK_REALTIME, &spec);

        character_node* current = head;
        while (current != NULL) {
            // printf("death: %ld, nsec: %ld, %d\r\n", current->death, spec.tv_nsec, current->death < spec.tv_nsec);
            if (current->death < spec.tv_nsec) {
                advance_character(current, spec.tv_nsec);
                dirty = true;
            }
            current = current->next;
        }
        if (dirty) {
            draw_screen();
        }
    }

    disable_alternate_screen();
    disable_raw_mode();

    // printf("Width: %d, height: %d\n", ts.ts_cols, ts.ts_lines);

    return 0;
}
