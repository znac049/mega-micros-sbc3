/* 
MIT License

Copyright (c) 2026 Bob Green

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <stdbool.h>

// Set your specific hardware delay for the desired clock speed (e.g., 4µs for 100kHz)
void i2c_delay(void) {
    // Example: Use a hardware timer delay or software loop
    for (volatile int i = 0; i < 10; i++); 
}

// Drive SDA Low (Output mode)
void sda_low(void) {
    // Set pin direction to OUTPUT. Output register must already be 0.
}

// Release SDA High (Input mode, pulled up by external resistor)
void sda_high(void) {
    // Set pin direction to INPUT.
}

// Drive SCL Low (Output mode)
void scl_low(void) {
    // Set pin direction to OUTPUT. Output register must already be 0.
}

// Release SCL High (Input mode, pulled up by external resistor)
void scl_high(void) {
    // Set pin direction to INPUT.
}

// Read the physical state of the SDA pin
bool read_sda(void) {
    // Return true if pin is high, false if low.
    return true; 
}

void i2c_init(void) {
    // Ensure data registers are cleared to 0 so toggling DDR controls the state
    // Set pins as INPUT initially to let the bus stay high
    sda_high();
    scl_high();
    i2c_delay();
}

void i2c_start(void) {
    sda_high();
    scl_high();
    i2c_delay();
    sda_low();   // SDA falls while SCL is high
    i2c_delay();
    scl_low();   // Hold SCL low to prepare for data transfer
}

void i2c_stop(void) {
    sda_low();
    i2c_delay();
    scl_high();  // SCL goes high
    i2c_delay();
    sda_high();  // SDA rises while SCL is high
    i2c_delay();
}

// Transmit a byte and return true if ACK received, false if NACK
bool i2c_write_byte(unsigned char byte) {
    // Transmit 8 data bits
    for (int i = 0; i < 8; i++) {
        if (byte & 0x80) {
            sda_high();
        } else {
            sda_low();
        }
        byte <<= 1;
        i2c_delay();
        
        scl_high();   // Target samples data on rising edge
        i2c_delay();
        scl_low();
    }
    
    // Read ACK/NACK bit (9th clock)
    sda_high();       // Release SDA so slave can pull it low
    i2c_delay();
    scl_high();       // Raise clock for reading
    i2c_delay();
    
    bool ack = (read_sda() == 0); // Slave pulls down for ACK
    
    scl_low();
    return ack; 
}

// Read a byte from the bus and send ACK (true) or NACK (false)
unsigned char i2c_read_byte(bool send_ack) {
    unsigned char byte = 0;
    sda_high();       // Release the line for the slave to write
    
    for (int i = 0; i < 8; i++) {
        i2c_delay();
        scl_high();
        i2c_delay();
        
        byte <<= 1;
        if (read_sda()) {
            byte |= 0x01;
        }
        
        scl_low();
    }
    
    // Send ACK/NACK bit (9th clock)
    if (send_ack) {
        sda_low();    // Master pulls down to acknowledge
    } else {
        sda_high();   // Master releases to signal NACK (end of read sequence)
    }
    i2c_delay();
    scl_high();
    i2c_delay();
    scl_low();
    sda_high();       // Release data line
    
    return byte;
}

// Transmit a byte and return true if ACK received, false if NACK
bool i2c_write_byte(unsigned char byte) {
    // Transmit 8 data bits
    for (int i = 0; i < 8; i++) {
        if (byte & 0x80) {
            sda_high();
        } else {
            sda_low();
        }
        byte <<= 1;
        i2c_delay();
        
        scl_high();   // Target samples data on rising edge
        i2c_delay();
        scl_low();
    }
    
    // Read ACK/NACK bit (9th clock)
    sda_high();       // Release SDA so slave can pull it low
    i2c_delay();
    scl_high();       // Raise clock for reading
    i2c_delay();
    
    bool ack = (read_sda() == 0); // Slave pulls down for ACK
    
    scl_low();
    return ack; 
}

// Read a byte from the bus and send ACK (true) or NACK (false)
unsigned char i2c_read_byte(bool send_ack) {
    unsigned char byte = 0;
    sda_high();       // Release the line for the slave to write
    
    for (int i = 0; i < 8; i++) {
        i2c_delay();
        scl_high();
        i2c_delay();
        
        byte <<= 1;
        if (read_sda()) {
            byte |= 0x01;
        }
        
        scl_low();
    }
    
    // Send ACK/NACK bit (9th clock)
    if (send_ack) {
        sda_low();    // Master pulls down to acknowledge
    } else {
        sda_high();   // Master releases to signal NACK (end of read sequence)
    }
    i2c_delay();
    scl_high();
    i2c_delay();
    scl_low();
    sda_high();       // Release data line
    
    return byte;
}

void write_register(unsigned char slave_addr, unsigned char reg, unsigned char data) {
    unsigned char write_addr = (slave_addr << 1) | 0; // Write bit is 0
    
    i2c_start();
    if (i2c_write_byte(write_addr)) {
        if (i2c_write_byte(reg)) {
            i2c_write_byte(data);
        }
    }
    i2c_stop();
}
