
# Digital-Tuner-AVR
A digital musical tuner on the AVR 128db48 using autocorrelation-based pitch detection and parabolic interpolation. Designed using interrupt driven ADC which can detect from 40Hz-2000hz, DSP is done using DC removal, autocorrelation peak detection, parabolic interpolation and primarily fixed point for efficient processing. 
Uses custom SSD1306 OLED driver and a bitmap font to display musical notes and intonation via I2C. Used double buffered sampling, interrupts and the AVR event system to allow low-latency and continuous processing. 

*Note: the uart.c, uart.h were written by Joerg Wunsch and the uart-avrdx.c was written by my professor John Chandy for the course ECE3411: Microprocessor Applications Laboratory 

## Design and choices
Design Overview:
	In my project I use two new components, the MAX4466 and the SSD1306. The MAX4466 has built-in gain control ranging from 25x to 125x. I chose 125x to ensure that it is sensitive to pick up frequencies from an instrument or a voice from a distance. Having the signal amplified more also ensures that it better spans the 0 to 3.3v rails which ensures the ADC has better resolution of the sample so long as the microphone does not peak. 
	The microphone is sampled using an event system and interrupts to ensure that the time between samples are exact, if not, the computations would not provide accurate frequency data. TC01 is used to count up to a chosen PER value which is calculated depending on your desired sampling rate, when the PER is reached on a hardware level the ADC is started. The frequencies calculated depend on SAMPLE_RATE/k where k is the autocorrelation peak and so I chose a sampling rate of 8000 to ensure a good enough frequency resolution while not being over-bearing on the cpu. When the ADC is finished it causes an interrupt and samples the data into a buffer. Finally I do a DC shift on the data due to the microphone output being centered at around 1.6V.
	For processing I chose to use autocorrelation. While initially I wrote the code using an FFT, it was very slow due to the massive amount of ram needed due to the algorithm being recursive. Following autocorrelation I find the largest first peak (to ensure upper harmonics aren’t selected instead of the root note) and use a parabolic interpolation formula to get better frequency resolution. This is needed because especially at high frequencies (low k values) the peak frequency varies immensely which would make it essentially unusable otherwise at upper frequencies unless I had an extremely large sampling rate (F = SAMPLE_RATE/k so for k= 4, F =2000, k = 5, F = 1600). The parabolic interpolation compares the values to the side of the peak and approximates k value as a float instead of an int. Finally the frequency is compared using a binary search to a lookup table of known note frequencies to initialize a Note struct. 
	The OLED drivers use TWI to first initialize the screen using the start commands found in the data sheet and then using two functions SSD1306_print_char and SSD1306_print_int. Both take the strings of bytes from my font.c file which I drew using GLCD font creator (though the output was in the wrong format and I had swapped the byte order in python). The font is 16x8. The code loops through the first 12 bytes which is the upper page, moves to the page below and prints the next 12 bytes. Each byte is for the 8 bit pages. The code for print char and print int are nearly the same except print int first checks the ints size and prints every digit sequentially in order. 

Lessons and Challenges
	The majority of the challenges was performance optimization. Due to constraints of ram and CPU speed, I had to spend a lot of time altering my code so it could get usable frequency resolution and accuracy. As I mentioned before I had written code for an FFT and the performance was so bad I decided to start from scratch switch algorithms. Writing the FFT on its own along with the hann window (which reduces spectral ghosting) taught me how to properly write code in fixed point which runs significantly faster than floating point code but has the issue of being harder to debug and introduces issues of dealing with type sizes. The autocorrelation algorithm likewise had me dealing with type conversion very frequently as well due to sampling at 16bit but processing at 32bit and 64bit. A large amount of time was spent trying to avoid floats and to ensure that the type with the least amount of memory and fastest processing speed was chosen. While it was a challenge, this skill is certainly something essential that will be used in many projects in the future.



## Experimental Results of the Tuners behavior

<img width="600" height="371" alt="Time to reach stable pitch" src="https://github.com/user-attachments/assets/e1ba6300-00a8-4016-89b2-56c1064e0767" />

The average time to settle for the tuner is 1.02 seconds. The time was recorded counting 240hz slowmotion footage frame by frame and so the resulting error per attampt is likely only within the miliseconds. The data does not seem to show a trend in adjustment time depending on the tested frequency. 

**The needed volume for Proper pitch detection**

Methods: Used a decibil meter on a phone and gradually raised the volume on my speaker system playing a sine tone until the correct tone appears and is stable. It should be noted that the mobil decibal meters are not as acurate as dedicated ones and so exact numbers may deviate.

<img width="600" height="371" alt="Requiered dB for stable pitch detection" src="https://github.com/user-attachments/assets/a9569eb4-2bc0-46ac-8829-b77fae70472f" />

To reach a stable and accurate the tuner requires an average of 61 dB within the range of 50hz - 2000hz. There is a unexpected bump in required volume betewen 500hz - 1000hz but the average still remains 67 dB, bellow 70 dB which is the volume or normal conversation. The tuner becomes unusable the range above 2000hz requiring between 77 dB up to 84dB which is as loud as a diesal truck or food blender. Due to the nature of how I recorded the data it is hard to know if the bump between 500hz -1000hz is a repeatable error or simeply a result of my methods used. Regardless the trend of rapid increasing required volume about 2000 hz is evident qualitativly while recording the data myself. 



<img width="600" height="371" alt="Error in Cents over frequency" src="https://github.com/user-attachments/assets/e6980c3e-2dd3-4701-bacf-c525b01d8963" />

The tuner, when within the proper volume range, between 50hz and 1000hz (the range of most human singers along with instrumnents like trumpets, saxophones etc) has a average pitch deviation of 0.567 cents well within the desired margine of error of 3-5 cents. The tuner in the upper and lower extremes of its range to experience severe loss in accurance (upwards of 48 cents or nearly a quarter tone) albiet only in ranges bellow 50 hz or above 2000 hz which are the extremes or even outside of the range of most common instruments. In the range from 500hz to 2000hz the tuner experiences a drift in pitch detecting pitches on average 1.56cents higher than they are but is a error still within the desired 3-5 cents. 
