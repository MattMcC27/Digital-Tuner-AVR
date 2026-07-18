/*
 * Tuner.c
 *
 * Created: 4/22/2026 11:30:39 AM
 * Author : matth
 */ 

#include <avr/io.h>
#include <avr/interrupt.h>
#include "uart.h"
#include "font.h"
#include <stdlib.h>
#include <math.h>
#include <util/twi.h>
#include <util/delay.h>

void TWI_Host_Initialize(void);
int TWI_Address(uint8_t Address, uint8_t mode);
int  TWI_Transmit_Data(uint8_t data);
uint8_t TWI_Receive_Data(void);
void TWI_Stop(void);

#define F_CPU 16000000UL
#define SAMPLE_RATE 8000
#define F_LOW 40
#define F_HIGH 2000
#define BUFF_SIZE 512
#define PI 3.1415927410
#define COMMAND 0x00
#define DATA 0x40
#define SSD1306_ADDR 0x3C
#define NOISE_TOLERANCE 45000000

volatile int16_t buffer1[BUFF_SIZE];
volatile int16_t buffer2[BUFF_SIZE];
volatile int16_t* volatile write_buffer = buffer1;
volatile int16_t* volatile read_buffer = buffer2;


int last_best_f = 0;
volatile int s_index = 0;
volatile int timer = 0;
volatile uint8_t process_ready = 0;

typedef struct {
	int cents;  //1200log2(N1/N2)
	int letter_sharp; //If the note is sharp or not
	char letter; //Name of not like  A, B etc
} Note;

Note note_Find(int f);


int main(void)
{
	init_clock();
	init_adc();
	init_TCA0();
	EVEN_init();
	uart_init(3, 9600, NULL);
	
	sei();
	TWI_Host_Initialize();
	SSD1306_init();
	SSD1306_clearPattern();

	
	int k_low  = SAMPLE_RATE / F_HIGH; 
	int k_high = SAMPLE_RATE / F_LOW;   

	while (1)
	{
		//when buffer is full
		if (process_ready) {
			process_ready = 0;

			int16_t x[BUFF_SIZE]; //samples
			int64_t R[BUFF_SIZE]; //autocorrelation function
			
			for (int i =0; i<BUFF_SIZE; i++) {
				x[i]= read_buffer[i];
			}
			//mic sits from 0 to 3.3 so dc of 1.6 needs to be removed
			DC_Shift(x);

			AutoCor(x, R, k_low, k_high);

 
			int best_k = -1;         
			double refined_k =0.0;
			int freq;
			
			//Finds only first peak in autocorrelation function to prevent upper octaves from being selected
			//Selects only if past expermental noise tolerance level
			for (int i = k_low +1; i < k_high - 1; i++) {
				if (R[i] > NOISE_TOLERANCE && R[i] > R[i-1] && R[i] > R[i+1]) {
					best_k = i;
					break;
				}
			}
			
			//If nothing past noise tolerance ex: instrument stop keep last frequency
			if (best_k == -1) {
				freq = last_best_f;
			}
			
			//using parabolic interpolation to increase frequency resolution (prevents things like k=10 -> 800 k=11 ->727
			else {
				double delta = 0.0;
				if (best_k > k_low && best_k < k_high){
					double num = (double)(R[best_k-1]-R[best_k+1]);
					double den = (double)(2*(R[best_k-1]-2*R[best_k] + R[best_k+1]));
					if (den != 0.0) {
						delta = num / den;
					}
				}
				
				refined_k = best_k + delta;
				double freq_f = (double) SAMPLE_RATE*1.0075/ refined_k;  //sample rate was experientially off, 1.0075 is correction factor 
				freq = (int) (freq_f +0.5);
				last_best_f = freq;
			}
			
			
			if (timer > 1) {
				Note a = note_Find(freq);
				SSD1306_print_note(a, 3, 0);
				printf("%d \n", freq); 
				printf("Note: %c cents: %d, is sharp: %d \n", a.letter, a.cents, a.letter_sharp); 
				timer = 0;
			}
		}
	}
}

//Multiply signal by its self, shift and sum see when signal periods line up
void AutoCor(int16_t* x, int64_t* R, int k_low, int k_high) {
	for (int k = k_low; k <= k_high; k++) {
		int64_t sum = 0;
		for (int i = 0; i < BUFF_SIZE - k; i++) {
			sum += (int32_t)(x[i]) * (int32_t)(x[i + k]);
		}
		R[k] = sum;
	}
}

//remove average dc value in sample
void DC_Shift(int16_t* data) {
	int32_t sum = 0;
	for (int i =0; i<BUFF_SIZE; i++) {
		sum += data[i];
	}
	
	int mean = (int) (sum/BUFF_SIZE);
	
	for (int i =0; i<BUFF_SIZE; i++) {
		data[i] -=mean;
	}
}

void init_adc(void)
{
	VREF.ADC0REF = VREF_REFSEL_VDD_gc;
	ADC0.CTRLA = ADC_RESSEL_12BIT_gc | ADC_ENABLE_bm;
	ADC0.CTRLC = ADC_PRESC_DIV16_gc;
	ADC0.MUXPOS = ADC_MUXPOS_AIN21_gc;
	ADC0.CTRLD = ADC_INITDLY_DLY16_gc;
	ADC0.INTCTRL = ADC_RESRDY_bm;
	ADC0.EVCTRL = ADC_STARTEI_bm;
}

ISR(ADC0_RESRDY_vect) {
	write_buffer[s_index] = ADC0.RES;
	s_index++;
	//when full swap buffers to continue collecting and process data
	if (s_index >= BUFF_SIZE) {
		volatile int16_t* temp = write_buffer; 
		write_buffer = read_buffer;
		read_buffer  = temp;

		s_index = 0;
		process_ready = 1;
		timer++;
	}
}

//Has overflow start the adc without need for interupt
void EVEN_init(void) {
	EVSYS.CHANNEL0 = EVSYS_CHANNEL0_TCA0_OVF_LUNF_gc;
	EVSYS.USERADC0START =EVSYS_USER_CHANNEL0_gc;
}

void init_TCA0(void)
{
	TCA0.SINGLE.CTRLA = 0;
	//TCA0.SINGLE.CTRLB = TCA_SINGLE_WGMODE_NORMAL_gc; // Normal mode
	TCA0.SINGLE.PER = (F_CPU/(64UL*SAMPLE_RATE))-1; // Set number of ticks for period
	//TCA0.SINGLE.INTCTRL = TCA_SINGLE_OVF_bm; // Enable TCA0 Overflow ISR
	TCA0.SINGLE.CTRLA |= (TCA_SINGLE_CLKSEL_DIV64_gc | TCA_SINGLE_ENABLE_bm);
}

void init_clock() {
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.XOSCHFCTRLA = CLKCTRL_FRQRANGE_16M_gc | CLKCTRL_ENABLE_bm;
	CPU_CCP = CCP_IOREG_gc;
	CLKCTRL.MCLKCTRLA = CLKCTRL_CLKSEL_EXTCLK_gc;
	while(!(CLKCTRL.MCLKSTATUS & CLKCTRL_EXTS_bm));
}

void TWI_Host_Initialize()
{
	TWI0.MBAUD = (uint8_t) ((F_CPU / 100000UL -10)/2);
	TWI0.MCTRLA |= TWI_ENABLE_bm; // enable the I2C
	TWI0.MSTATUS |= TWI_BUSSTATE_IDLE_gc; // force the bus state to IDLE
}

int TWI_Address(uint8_t Address, uint8_t read)
{
	while (1) {
		
		TWI0.MADDR = (Address << 1) | (read ? 1 :0);
		
		while (!(TWI0.MSTATUS & TWI_WIF_bm));
		
		if (TWI0.MSTATUS & TWI_RXACK_bm) {
			return 1;
		}
		
		return 0;
	}
}


int TWI_Transmit_Data(uint8_t data)
{
	TWI0.MDATA = data;
	while (!(TWI0.MSTATUS & TWI_WIF_bm));
	
	if (TWI0.MSTATUS & TWI_RXACK_bm) {
		return 1;
	}
	
	
	return 0;
}

void TWI_Stop()
{
	TWI0.MCTRLB |= TWI_MCMD_STOP_gc;
}

void SSD1306_init(void){
	_delay_ms(100);
	
	SSD1306_command(0xAE); // display off
	SSD1306_command(0xA8); SSD1306_command(0x3F); //mux ratio, ensure every row is on
	SSD1306_command(0xD3); SSD1306_command(0x00); //Display offset = 0
	SSD1306_command(0x20); SSD1306_command(0x02); // Page Addressing Mode (state when you want to change page)
	SSD1306_command(0x40); //set display start line at 0
	SSD1306_command(0xA1); //Flips screen vertically
	SSD1306_command(0xC8); //flip screen horizontally
	SSD1306_command(0xDA); SSD1306_command(0x12); //configures pins for 128x64 screen
	SSD1306_command(0x81); SSD1306_command(0xFF); //
	SSD1306_command(0xA4); //Turn on display
	SSD1306_command(0xA6); //1 is lit 0 is off mode
	SSD1306_command(0xD5); SSD1306_command(0x80); // default clock frequency
	SSD1306_command(0x8D); SSD1306_command(0x14); // charge pump setting
	SSD1306_command(0xAF); // turn on display

}

void SSD1306_command(uint8_t data) {
	TWI_Address(SSD1306_ADDR, 0);
	TWI_Transmit_Data(0x00); //00 control bit for command
	TWI_Transmit_Data(data);
	TWI_Stop();
}

void SSD1306_data_start(void) {
	TWI_Address(SSD1306_ADDR, 0);
	TWI_Transmit_Data(0x40); //continuation bit (sending more data)
}

void SSD1306_data(uint8_t data) {
	TWI_Transmit_Data(data);
}

void SSD1306_data_stop(void){
	TWI_Stop();
}

void SSD1306_setPosition(uint8_t page, uint8_t column) {
	SSD1306_command(0xB0 | page); //bit for setting page combined with page number
	SSD1306_command(0x00 | (column & 0x0F)); // byte for setting collum w/ lower 4 bits
	SSD1306_command(0x10 | (column >> 4)); //byte for setting upper collum with upper 4 bits
}


void SSD1306_clearPattern(void)
{
	for (uint8_t page = 0; page < 8; page++)
	{
		SSD1306_setPosition(page, 0);
		SSD1306_data_start();
		for (uint8_t col = 0; col < 128; col++)
		{
			SSD1306_data(0x00); // blank
		}

		SSD1306_data_stop();
	}
}

void SSD1306_print_note(Note N, int page, int col) {
	
	int current_col = col;
	//print Note name
	SSD1306_print_char(N.letter, page, current_col);
	current_col+=FONT_WIDTH;
	//print if sharp
	if (N.letter_sharp) {
		SSD1306_print_char('#', page, current_col);
		current_col+=FONT_WIDTH;
	}
	//print intonation indicator
	if (N.cents >=0) {
		SSD1306_print_char('>', page, current_col);
	}
	else if (N.cents < 0) {
		SSD1306_print_char('<', page, current_col);
	}
	current_col+=FONT_WIDTH;
	//print cents out of tune
	int n = SSD1306_print_int(abs(N.cents), page, current_col);
	current_col+=FONT_WIDTH*n;
	//print space to clear old characters
	SSD1306_print_char(' ', page, current_col);
	current_col+=FONT_WIDTH;
	SSD1306_print_char(' ', page, current_col);
}

void SSD1306_print_char(char c, int page, int col) {
	const uint8_t* bit_map = font_alph(c);
	
	if (col <= 128-12) {
		SSD1306_setPosition(page, col);
		SSD1306_data_start();
		//print first page
		int i =0;
		while (i < FONT_CHAR_SIZE/2) {
			SSD1306_data(bit_map[i]);
			i++;
		}
		SSD1306_data_stop();
		if (page <7) {
			SSD1306_setPosition(page+1, col);
		}
		//print second page
		SSD1306_data_start();
		while (i < FONT_CHAR_SIZE) {
			SSD1306_data(bit_map[i]);
			i++;
		}
	}
	SSD1306_data_stop();
}


int SSD1306_print_int(int n, int page, int col) {
	//find the size of the int
	int divisor = 1;
	int current_digit;
	int total_digits =1;
	while (n / (divisor*10) >0) {
		divisor *= 10;
		total_digits ++;
	}
	//prints each digit from left to right
	for (int k = 0; k<total_digits; k++) {
		
		current_digit = n / divisor;
		n = n % divisor;
		divisor /= 10;
		int current_collumn = col + k*FONT_WIDTH;
		
		const uint8_t* bit_map = font_num(current_digit);
		if (current_collumn <= 128-FONT_WIDTH) {
			SSD1306_setPosition(page, current_collumn);
			SSD1306_data_start();
			int i =0;
			while (i < FONT_CHAR_SIZE/2) {
				SSD1306_data(bit_map[i]);
				i++;
			}
			SSD1306_data_stop();
			if (page <7) {
				SSD1306_setPosition(page+1, current_collumn);
			}
			SSD1306_data_start();
			while (i < FONT_CHAR_SIZE) {
				SSD1306_data(bit_map[i]);
				i++;
			}
			}
			SSD1306_data_stop();
	}
	return total_digits;
}

Note note_Find(int f) {
	Note temp;
	int best_note;
	int cents;
	
	int32_t notes[73] = {
		3270, 3465, 3671, 3889, 4120, 4365, 4625, 4900, 5191,
		5500, 5827, 6174, 6541, 6930, 7342, 7778, 8241, 8731, 9250, 9800, 10383,
		11000, 11654, 12347, 13081, 13859, 14683, 15556, 16481, 17461, 18500, 19600, 20765,
		22000, 23308, 24694, 26163, 27718, 29366, 31113, 32963, 34923, 37000, 39200, 41530,
		44000, 46616, 49388, 52325, 55437, 58733, 62225, 65926, 69846, 74000, 78400, 83061,
		88000, 93233, 98777, 104650, 110873, 117466, 124451, 131851, 139691, 148000, 156800, 166122,
		176000, 186466, 197553, 209300
	}; //from c1 to c7 in Hz*100
	
	char note_name[12] = {
		'C', 'C', 'D', 'D', 'E', 'F', 'F', 'G', 'G', 'A', 'A', 'B'
	};
	
	int sharp_index[12] = {
		 0,   1,   0,  1,   0,   0,   1,   0,   1,   0,   1,   0
	};
	
	int32_t norm_f = (int32_t)f *100L;
	
	//range slightly above midpoints of border notes to ensure it doesn't oscilate
	int low = 0;
	int high = 72;
	int note_floor = -1;
	
	//binary search 
	while (low <= high) {
		int mid = low + (high-low)/2;
		if(notes[mid] <= norm_f) {
			note_floor = mid;
			low = mid +1;
		}
		else {
			high = mid -1;
		}
	}
	if (note_floor <0) {
		note_floor = 0;
	}
	else if (note_floor >72) {
		note_floor = 72;
	}
	
	if (note_floor<72) {
		//use taylor series cents = 1200*log(N1/N2) giving 1731 * (N1 -n2)/n2
		int cents_sharp = (int)(((norm_f - notes[note_floor]) * 1731L) / notes[note_floor]); //sharp than notefloor
		int cents_flat = (int)(((norm_f - notes[note_floor+1]) * 1731L) / notes[note_floor+1]); // flatter than notefloor +1
		if (abs(cents_flat) < abs(cents_sharp)) {
			cents = cents_flat;
			best_note = (note_floor+1)%12;
		} else {
			cents = cents_sharp;
			best_note = (note_floor)%12;
		}
	}
	//if beyond samplng ability
	else {
		cents = (int)(((norm_f - notes[72]) * 1731L) / notes[72]);
		best_note = (72)%12;
	}
	
	temp.cents = cents;
	temp.letter = note_name[best_note];
	temp.letter_sharp = sharp_index[best_note];
	return temp;
}