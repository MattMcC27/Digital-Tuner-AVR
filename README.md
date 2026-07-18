<img width="600" height="371" alt="Time to reach stable pitch" src="https://github.com/user-attachments/assets/e1ba6300-00a8-4016-89b2-56c1064e0767" />

The average time to settle for the tuner is 1.02 seconds. The time was recorded counting 240hz slowmotion footage frame by frame and so the resulting error per attampt is likely only within the miliseconds. The data does not seem to show a trend in adjustment time depending on the tested frequency. 

**The needed volume for Proper pitch detection**

Methods: Used a decibil meter on a phone and gradually raised the volume on my speaker system playing a sine tone until the correct tone appears and is stable. It should be noted that the mobil decibal meters are not as acurate as dedicated ones and so exact numbers may deviate.

<img width="600" height="371" alt="Requiered dB for stable pitch detection" src="https://github.com/user-attachments/assets/a9569eb4-2bc0-46ac-8829-b77fae70472f" />

To reach a stable and accurate the tuner requires an average of 61 dB within the range of 50hz - 2000hz. There is a unexpected bump in required volume betewen 500hz - 1000hz but the average still remains 67 dB, bellow 70 dB which is the volume or normal conversation. The tuner becomes unusable the range above 2000hz requiring between 77 dB up to 84dB which is as loud as a diesal truck or food blender. Due to the nature of how I recorded the data it is hard to know if the bump between 500hz -1000hz is a repeatable error or simeply a result of my methods used. Regardless the trend of rapid increasing required volume about 2000 hz is evident qualitativly while recording the data myself. 



<img width="600" height="371" alt="Error in Cents over frequency" src="https://github.com/user-attachments/assets/e6980c3e-2dd3-4701-bacf-c525b01d8963" />

The tuner, when within the proper volume range, between 50hz and 1000hz (the range of most human singers along with instrumnents like trumpets, saxophones etc) has a average pitch deviation of 0.567 cents well within the desired margine of error of 3-5 cents. The tuner in the upper and lower extremes of its range to experience severe loss in accurance (upwards of 48 cents or nearly a quarter tone) albiet only in ranges bellow 50 hz or above 2000 hz which are the extremes or even outside of the range of most common instruments. In the range from 500hz to 2000hz the tuner experiences a drift in pitch detecting pitches on average 1.56cents higher than they are but is a error still within the desired 3-5 cents. 

# Digital-Tuner-AVR
A digital musical tuner on the AVR 128db48 using autocorrelation-based pitch detection and parabolic interpolation. Designed using interrupt driven ADC which can detect from 40Hz-2000hz, DSP is done using DC removal, autocorrelation peak detection, parabolic interpolation and primarily fixed point for efficient processing. 
Uses custom SSD1306 OLED driver and a bitmap font to display musical notes and intonation via I2C. Used double buffered sampling, interrupts and the AVR event system to allow low-latency and continuous processing. 

*Note: the uart.c, uart.h were written by Joerg Wunsch and the uart-avrdx.c was written by my professor John Chandy for the course ECE3411: Microprocessor Applications Laboratory 
