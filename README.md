# smart-timed-plug
Smart timed plug for my coffee grinder. It just turns the power on for the given time.

Additional Features:
- Time can be set with 0.1 second accuracy. The value is stored in EEPROM, ensuring the previously set time is retained after a power outage.
- Pressing the Start/Stop button while the unit is running pauses the timer and turns off power. In this mode, the time setting can be adjusted.
- After a period of inactivity, the unit enters sleep mode and the display turns off. Pressing the Start/Stop button once exits sleep mode, powers the unit, and starts the timer.

Default state, when the full time appears on the screen:
<br/>
<img src="https://github.com/user-attachments/assets/956c328f-bd76-46b6-a205-6851927bef10" width="250">

There is a nice progress bar to indicate, how much time is left:
<br/>
<img src="https://github.com/user-attachments/assets/e2ce9592-6d70-4c3a-bb53-230cac80aac5" width="250">

The display is an OLED (128x64 px) based on SSD1309.
