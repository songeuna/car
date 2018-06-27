#define I2C_DEV         "/dev/i2c-1"
#define CLOCK_FREQ      25000000.0
#define PCA_ADDR        0x40
#define LED_STEP        50

#define LEFT            68
#define RIGHT           67
#define UP              65
#define DOWN            66
#define AX	            4010
#define IN             880
#define C_MAX           510
#define C_MIN           100

// Register Addr
#define MODE1           0x00
#define MODE2           0x01

#define LED15_ON_L      0x42
#define LED15_ON_H      0x43
#define LED15_OFF_L     0x44
#define LED15_OFF_H     0x45

#define LED14_ON_L      0x3E
#define LED14_ON_H      0x3F
#define LED14_OFF_L     0x40
#define LED14_OFF_H     0x41

#define LED13_ON_L      0x3A
#define LED13_ON_H      0x3B
#define LED13_OFF_L     0x3C
#define LED13_OFF_H     0x3D

#define LED4_ON_L       0x16
#define LED4_ON_H       0x17
#define LED4_OFF_L      0x18
#define LED4_OFF_H      0x19

#define LED5_ON_L       0x1A
#define LED5_ON_H       0x1B
#define LED5_OFF_L      0x1C
#define LED5_OFF_H      0x1D

// Set GPIO
#define GPIO17          0
#define GPIO27          2

#define PRE_SCALE       0xFE
