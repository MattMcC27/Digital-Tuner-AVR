<img width="600" height="371" alt="Time to reach stable pitch" src="https://github.com/user-attachments/assets/e1ba6300-00a8-4016-89b2-56c1064e0767" />

<img width="600" height="371" alt="Requiered dB for stable pitch detection" src="https://github.com/user-attachments/assets/a9569eb4-2bc0-46ac-8829-b77fae70472f" />

<img width="600" height="371" alt="Error in Cents over frequency" src="https://github.com/user-attachments/assets/e6980c3e-2dd3-4701-bacf-c525b01d8963" />

The tuner, when within the proper volume range, between 50hz and 1000hz (the range of most human singers along with instrumnents like trumpets, saxophones etc) has a average pitch deviation of 0.567 cents well within the desired margine of error of 3-5 cents. The tuner in the upper and lower extremes of its range to experience severe loss in accurance (upwards of 48 cents or nearly a quarter tone) albiet only in ranges bellow 50 hz or above 2000 hz which are the extremes or even outside of the range of most common instruments. In the range from 500hz to 2000hz the tuner experiences a drift in pitch detecting pitches on average 1.56cents higher than they are but is a error still within the desired 3-5 cents. 

# Digital-Tuner-AVR
A digital musical tuner on the AVR 128db48 using autocorrelation-based pitch detection and parabolic interpolation. Designed using interrupt driven ADC which can detect from 40Hz-2000hz, DSP is done using DC removal, autocorrelation peak detection, parabolic interpolation and primarily fixed point for efficient processing. 
Uses custom SSD1306 OLED driver and a bitmap font to display musical notes and intonation via I2C. Used double buffered sampling, interrupts and the AVR event system to allow low-latency and continuous processing. 

*Note: the uart.c, uart.h were written by Joerg Wunsch and the uart-avrdx.c was written by my professor John Chandy for the course ECE3411: Microprocessor Applications Laboratory 
