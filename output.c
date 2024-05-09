#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

struct termios orig_termios;

void disable_raw_mode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_raw_mode(bool disable_at_exit) {
    tcgetattr(STDIN_FILENO, &orig_termios);

    struct termios raw = orig_termios;
    cfmakeraw(&raw);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);

    if (disable_at_exit) atexit(disable_raw_mode);
}

int main() {
    enable_raw_mode(true);

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 3) break;
        printf("%d ", c);
    }
    printf("\r\n");

    return 0;
}
