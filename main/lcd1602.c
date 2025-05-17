#include "driver/i2c.h"
#include "freertos/task.h"

#define I2C_MASTER_NUM I2C_NUM_0  // I2C port number for the master device
#define LCD_BACKLIGHT 0x08        // Backlight bit for the LCD
#define ENABLE 0x04               // Enable pin for the LCD
#define COMMAND 0                 // Mode for sending commands to the LCD
#define DATA 1                    // Mode for sending data to the LCD

static void lcd_send_nibble(uint8_t addr, uint8_t nibble, uint8_t mode) {
    uint8_t data = nibble << 4 | LCD_BACKLIGHT | mode; // Prepare data
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();      // Create I2C command handle
    i2c_master_start(cmd);                             // Start I2C communication
    i2c_master_write_byte(cmd, addr << 1, true);       // Send address
    i2c_master_write_byte(cmd, data | ENABLE, true);   // Send data with enable pulse
    i2c_master_write_byte(cmd, data, true);            // Send data without enable pulse
    i2c_master_stop(cmd);                              // Stop I2C communication
    i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(50));  // Execute the command
    i2c_cmd_link_delete(cmd);                          // Delete command handle
    vTaskDelay(pdMS_TO_TICKS(1));                      // Small delay
}