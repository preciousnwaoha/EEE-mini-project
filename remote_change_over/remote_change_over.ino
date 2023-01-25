#include <LiquidCrystal_I2C.h>

#define relay_1 11
#define relay_2 10
#define relay_3 9

#define led_red 8
#define led_yellow 7
#define led_blue 6

#define AC_1 A3
#define AC_2 A1
#define AC_3 A2

#define threshold_voltage 50.0

LiquidCrystal_I2C lcd(0x27, 16, 2);

String data = "", Buffer = "";

struct AC_Selector
{
  float phase_voltage[3];
  bool phase_state[3];

  byte relay[3] = {relay_1, relay_2, relay_3};
  byte phase[3] = {AC_1, AC_2, AC_3};
  byte led[3] = {led_red, led_yellow, led_blue};

  uint32_t i, last_selected_phase = 0, selected_phase = 0, last_disp, last_blink, refresh_rate = 1000;

  void init(void)
  {
    for (i = 0; i < 3; i++)
    {
      pinMode(relay[i], 1);
      pinMode(led[i], 1);
      pinMode(phase[i], 0);

      phase_voltage[i] = 0.0;
      phase_state[i] = false;

    }
    for (i = 0; i < 3; i++)
    {
      digitalWrite(led[i], 1);
      delay(200);
    }

    delay(1500);
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("wChangeOver     ");
    lcd.setCursor(0, 1);
    lcd.print("Initializing... ");

    for (i = 0; i < 3; i++)
    {
      digitalWrite(led[i], 0);
      delay(200);
    }

    delay(500);
    lcd.clear();

    Serial.begin(9600);

  }
  void run(void)
  {
    measureVoltage();
    display(0);
    select_phase();
    if (Serial.available())
    {
      data = Serial.readStringUntil(';');
      if (data.length() > 0)
      {
        if (data == "+set1")
        {
          selected_phase = 1;
        }
        else if (data == "+set2")
        {
          selected_phase = 2;
        }
        else if (data == "+set3")
        {
          selected_phase = 3;
        }
        else if (data == "+unset")
        {
          selected_phase = 0;
        }
        else if (data == "+read")
        {
          load_buffer();
          Serial.println(Buffer);
        }
        data = "";
      }
    }
  }
  void blink_selected_phase(void)
  {
    if ((millis() - last_blink) >= 1000)
    {
      for (i = 0; i < 3; i++)
      {
        if (selected_phase == (i + 1))
        {
          if (phase_state[i] == true)
          {
            digitalWrite(led[i], !phase_state[i]);
            phase_state[i] = !phase_state[i];
          }
          else {
            digitalWrite(led[i], !phase_state[i]);
            phase_state[i] = !phase_state[i];
          }
        }
        else {
          if (phase_voltage[i] >= threshold_voltage)
          {
            digitalWrite(led[i], 1);
            phase_state[i] = true;
          }
          else {
            digitalWrite(led[i], 0);
            phase_state[i] = false;
          }
        }
      }
      last_blink = millis();
    }
  }
  void select_phase(void)
  {
    if (selected_phase == 0)
    {
      for (i = 0; i < 3; i++)
      {
        digitalWrite(relay[i], 0);
      }
      blink_selected_phase();
    }
    else {
      if (selected_phase != last_selected_phase)
      {
        for (i = 0; i < 3; i++)
        {
          digitalWrite(relay[i], 0);
        }
        last_selected_phase = selected_phase;
      }
      for (i = 0; i < 3; i++)
      {
        if ((i + 1) == selected_phase)
        {
          digitalWrite(relay[i], 1);
        }
        else {
          digitalWrite(relay[i], 0);
        }
      }
      blink_selected_phase();
    }
  }
  void load_buffer(void)
  {
    Buffer = "";
    Buffer.concat("{");
    Buffer.concat(phase_voltage[0]);
    Buffer.concat(",");
    Buffer.concat(phase_voltage[1]);
    Buffer.concat(",");
    Buffer.concat(phase_voltage[2]);
    Buffer.concat("}");
  }
  void measureVoltage(void)
  {
    float val0 = 0, val1 = 0, val2 = 0;
    float maxpk0 = 0, maxpk1 = 0, maxpk2 = 0;
    unsigned long Time = millis(), sampleTime = 1500;
    while (millis() - Time <= sampleTime)
    {
      for (int i = 0; i < 300; ++i)
      {
        val0 += analogRead(phase[0]);
        val1 += analogRead(phase[1]);
        val2 += analogRead(phase[2]);
      }
      val0 /= 300;
      val1 /= 300;
      val2 /= 300;

      if (val0 <= 0)
      {
        maxpk0 = 0;
      }
      else
      {
        if (val0 > maxpk0)
        {
          maxpk0 = val0;
        }
      }

      if (val1 <= 0)
      {
        maxpk1 = 0;
      }
      else
      {
        if (val1 > maxpk1)
        {
          maxpk1 = val1;
        }
      }

      if (val2 <= 0)
      {
        maxpk2 = 0;
      }
      else
      {
        if (val2 > maxpk2)
        {
          maxpk2 = val2;
        }
      }
    }
    maxpk0 = (maxpk0 * 505.0) / 1024.0;
    maxpk1 = (maxpk1 * 505.0) / 1024.0;
    maxpk2 = (maxpk2 * 505.0) / 1024.0;
    phase_voltage[0] = maxpk0 * 0.707;
    phase_voltage[1] = maxpk1 * 0.707;
    phase_voltage[2] = maxpk2 * 0.707;
  }
  void display(byte slide)
  {
    if ((millis() - last_disp) >= refresh_rate)
    {
      if (slide == 0)
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(" AC1  AC2  AC3  ");
        lcd.setCursor(0, 1);
        lcd.print(String(phase_voltage[0], 0) + "V, " + String(phase_voltage[1], 0) + "V, " + String(phase_voltage[2], 0) + "V");
      }
      last_disp = millis();
    }
  }
} ac_selector;

void setup() {
  ac_selector.init();
}

void loop() {
  ac_selector.run();
}
