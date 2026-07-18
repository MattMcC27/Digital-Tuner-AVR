# Digital-Tuner-AVR
A digital musical tuner on the AVR 128db48 using autocorrelation-based pitch detection and parabolic interpolation. Designed using interrupt driven ADC which can detect from 40Hz-2000hz, DSP is done using DC removal, autocorrelation peak detection, parabolic interpolation and primarily fixed point for efficient processing. 
Uses custom SSD1306 OLED driver and a bitmap font to display musical notes and intonation via I2C. Used double buffered sampling, interrupts and the AVR event system to allow low-latency and continuous processing. 
