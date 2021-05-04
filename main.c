#include <stdio.h>
#include "config.c"
#include "def_pinos.h"

// RAM module chip-select
#define CS P2_3

// RAM addresses
#define RAM_SIZE 1<<13

#define EEPROM_WRITE 0
#define EEPROM_READ 1

unsigned char esc_byte_cntr(unsigned char device, __bit RW) {
    STA = 1;
    SI = 0;

    while (SI == 0);
    if (SMB0STA != 0x08 && SMB0STA != 0x10) {
        return SMB0STA;
    }

    SMB0DAT = (device & 0xfe) | RW;
    STA = 0;
    SI = 0;
    while (SI == 0);

    if (RW == EEPROM_WRITE) {
        if (SMB0STA != 0x18) return SMB0STA;
    } else {
        if (SMB0STA != 0x40) return SMB0STA;
    }

    return 0;
}

unsigned char write_data_byte(unsigned char data) {
    SMB0DAT = data;

    SI = 0;
    while (SI == 0);

    if (SMB0STA != 0x28) {
        return SMB0STA;
    } else {
        return 0;
    }
}

int write_eeprom(unsigned char device, unsigned char address, unsigned char data) {
    unsigned char ret;

    ret = esc_byte_cntr(device, EEPROM_WRITE);
    if (ret != 0) return - (int) ret;

    ret = write_data_byte(address);
    if (ret != 0) return - (int) ret;

    ret = write_data_byte(data);
    if (ret != 0) return - (int) ret;

    STO = 1;
    SI = 0;
    while (STO == 1);

    while (1) {
        ret = esc_byte_cntr(device, EEPROM_WRITE);
        if (ret == 0) break;
        if (ret != 0x20) return - (int) ret;
    }

    return 0;
}

int read_eeprom(unsigned char device, unsigned char address) {
    int dado;
    unsigned char ret;

    ret = esc_byte_cntr(device, EEPROM_WRITE);
    if (ret != 0) return - (int) ret;

    ret = write_data_byte(address);
    if (ret != 0) return - (int) ret;

    ret = esc_byte_cntr(device, EEPROM_READ);
    if (ret != 0) return - (int) ret;

    AA = 0;
    SI = 0;
    while (SI == 0);

    if (SMB0STA != 0x58) {
        return - (int) SMB0STA;
    }
    dado = (int) SMB0DAT;

    STO = 1;
    SI = 0;
    while (STO == 1);

    return dado;
}

// delay execution
void delay(unsigned int ms) {
    TMOD |= 0x01;
    TMOD &= ~0x02;

    while (ms-- > 0) {
        TR0 = 0;
        TF0 = 0;
        TL0 = 0x58;
        TH0 = 0x9e;
        TR0 = 1;
        while (TF0 == 0);
    }
}

// handles printf
void putchar (unsigned char c) {
    SBUF0 = c;
    while (TI0 == 0);
    TI0 = 0;
}

void main (void) {
    unsigned int i = 0;
    unsigned char device = 0xA0;
    unsigned char address = 0;

    Init_Device();
    SFRPAGE = LEGACY_PAGE;

    i = read_eeprom(device, address);

    while (1) {
        printf_fast_f("Counter: %d\n", read_eeprom(device, address));
        write_eeprom(device, address, ++i);
        delay(500);
    }
}