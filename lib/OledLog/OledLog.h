#include <Arduino.h>
#include <SPI.h>
#include <SSD1306.h>

class OledLog {

  #define OLED_ADDRESS 0x3c
  #define OLED_SDA 4  // GPIO4
  #define OLED_SCL 15 // GPIO15
  #define OLED_RST 16 // GPIO16
  #define max_lines 5

  private:
    String log_array[max_lines];
    int log_length = 0;
    int new_logs = false;
    SSD1306 display;


  public:
    OledLog() : display(OLED_ADDRESS, OLED_SDA, OLED_SCL) {

      pinMode(OLED_RST, OUTPUT);
      digitalWrite(OLED_RST, LOW); // low to reset OLED
      delay(50);
      digitalWrite(OLED_RST, HIGH); // must be high to turn on OLED

      display.init();
      display.flipScreenVertically();
      display.setTextAlignment(TEXT_ALIGN_LEFT);
      display.setFont(ArialMT_Plain_10); // 10, 16
    }

    void log(String text)
    {   
            
      // only increment till screen is full
        if (log_length < max_lines)
            log_length++;
        else {
            // move up one line
            for (int i = 0; i < (log_length-1); i++)
                log_array[i] = log_array[i+1];
        }
        log_array[log_length-1] = text;
      new_logs = true;
    }
    void print_log()
    {   if (new_logs) {
        int font_height = 10;
        for (int i = 0; i < log_length; i++)
          display.drawString(0, i * font_height, String(log_array[i]));
        display.display();
        new_logs = false;
      }
    }
};
