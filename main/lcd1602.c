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

static void lcd_write_byte(uint8_t addr, uint8_t value, uint8_t mode) {
    lcd_send_nibble(addr, value >> 4, mode);  // Send high nibble
    lcd_send_nibble(addr, value & 0x0F, mode); // Send low nibble
}

void lcd1602_init(uint8_t addr) {
    vTaskDelay(pdMS_TO_TICKS(50)); // Initial delay
    lcd_send_nibble(addr, 0x03, COMMAND); // Send initialization command
    vTaskDelay(pdMS_TO_TICKS(5));         // Wait for a bit
    lcd_send_nibble(addr, 0x03, COMMAND); // Repeat initialization
    vTaskDelay(pdMS_TO_TICKS(5));         // Wait for a bit
    lcd_send_nibble(addr, 0x03, COMMAND); // Repeat initialization
    vTaskDelay(pdMS_TO_TICKS(5));         // Wait for a bit
    lcd_send_nibble(addr, 0x02, COMMAND); // Set 4-bit mode

    lcd_write_byte(addr, 0x28, COMMAND); // Set 2-line display
    lcd_write_byte(addr, 0x0C, COMMAND); // Display ON, cursor OFF
    lcd_write_byte(addr, 0x06, COMMAND); // Entry mode
    lcd_write_byte(addr, 0x01, COMMAND); // Clear display
    vTaskDelay(pdMS_TO_TICKS(2));        // Wait for clear display to finish
}

void lcd1602_clear(uint8_t addr) {
    lcd_write_byte(addr, 0x01, COMMAND);  // Send clear display command
    vTaskDelay(pdMS_TO_TICKS(2));         // Wait for the command to finish
}

void lcd1602_gotoxy(uint8_t addr, uint8_t col, uint8_t row) {
    const uint8_t row_offsets[] = {0x00, 0x40}; // Row addresses
    lcd_write_byte(addr, 0x80 | (col + row_offsets[row]), COMMAND); // Set cursor position
}

void lcd1602_write_string(uint8_t addr, const char *str) {
    while (*str) {
        lcd_write_byte(addr, *str++, DATA); // Write each character
    }
}
