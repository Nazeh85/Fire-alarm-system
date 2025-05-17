#include "driver/i2c.h"
#include "freertos/task.h"

#define I2C_MASTER_NUM I2C_NUM_0  // I2C port number for the master device
#define LCD_BACKLIGHT 0x08        // Backlight bit for the LCD
#define ENABLE 0x04               // Enable pin for the LCD
#define COMMAND 0                 // Mode for sending commands to the LCD
#define DATA 1                    // Mode for sending data to the LCD