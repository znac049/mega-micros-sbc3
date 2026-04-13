#include <stdio.h>
#include <machine.h>

int _getchar(void) {
    return _polled_getchar();
}

int _char_available(void) {
    return _polled_char_available();
}

void _putchar(int ch) {
    _polled_putchar(ch);
}