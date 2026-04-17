#include <stdio.h>
#include <machine.h>

#define DISP_A0 0x01
#define DISP_A1 0x02
#define DISP_WR 0x04
#define DISP_CE_LEFT 0x08
#define DISP_CE_RIGHT 0x10

#define DISP_CLR 0x80

#define ASSERT_WR() clear_bits_b(DISP_WR); waitabit()
#define DEASSERT_WR() set_bits_b(DISP_WR); waitabit()

#define ASSERT_CLR() clear_bits_b(DISP_CLR); waitabit()
#define DEASSERT_CLR() set_bits_b(DISP_CLR); waitabit()

#define UNSELECT_ALL() set_bits_b(DISP_CE_LEFT | DISP_CE_RIGHT); waitabit()
#define SELECT_LEFT() clear_bits_b(DISP_CE_LEFT); waitabit()
#define SELECT_RIGHT() clear_bits_b(DISP_CE_RIGHT); waitabit()
#define SELECT_COL(col) set_bits_b((~col)&0x03); waitabit()

static uint8_t port_a = 0;
static uint8_t port_b = 0;

static void waitabit(void) {
    for (int i=0; i<10000; i++) {
        __asm("nop");
    }
}

static void set_a(uint8_t val) {
    port_a = val;

    *pit_padr = port_a;

    printf("set A -> %02x\n", port_a);
}
static void set_bits_a(uint8_t bits) {
    port_a = port_a | bits;

    *pit_padr = port_a;

    printf("set bits A %02x -> %02x\n", bits, port_b);
}

static void clear_bits_a(uint8_t bits) {
    port_a = port_a & (~bits);

    *pit_padr = port_a;

    printf("clear bits A %02x -> %02x\n", bits, port_b);
}

static void set_b(uint8_t val) {
    port_b = val;

    *pit_pbdr = port_b;

    printf("set B -> %02x\n", port_b);
}
static void set_bits_b(uint8_t bits) {
    port_b = port_b | bits;

    *pit_pbdr = port_b;

    printf("set bits B %02x -> %02x\n", bits, port_b);
}

static void clear_bits_b(uint8_t bits) {
    port_b = port_b & (~bits);

    *pit_pbdr = port_b;

    printf("clear bits B %02x -> %02x\n", bits, port_b);
}

static void display(char ch, uint8_t pos) {
    set_bits_b(DISP_CE_LEFT | DISP_CE_RIGHT | DISP_WR);
    if (pos < 4) {
        printf("LEFT Bank\n");

        SELECT_LEFT();
    }
    else {
        printf("RIGHT Bank\n");

        SELECT_RIGHT();
    }

    SELECT_COL(pos);
    ASSERT_WR();

    set_a(ch);

    DEASSERT_WR();
    UNSELECT_ALL();
}

void main(void) {
    printf("Configuring Ports A and B...\n");

    *pit_paddr = 0xff;  // All outputs
    *pit_pbddr = 0xff;  // All outputs

    ASSERT_CLR();
    DEASSERT_CLR();

    UNSELECT_ALL();

    printf("Displaying characters...\n");

    // display('A', 0);
    // display('B', 1);
    // display('C', 2);
    // display('D', 3);

    display('e', 4);
    display('f', 5);
    display('g', 6);
    display('h', 7);

    /*
    *pit_padr = 0x41 | 0x80;     // 'A'
    *pit_pbdr = 0x04;
    *pit_pbdr = 0;
    *pit_pbdr = 0x04;

    *pit_pbdr = 0x05;
    *pit_pbdr = 0x01;
    *pit_pbdr = 0x05;
    */

    printf("Has it worked, A?\n");
    while (!char_available()) {
        ;
    }
    getchar();
}