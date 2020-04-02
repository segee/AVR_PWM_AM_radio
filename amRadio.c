#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdio.h>
#include <avr/eeprom.h>
#include <math.h>
#include <avr/sleep.h>


void init_pwm(void);

/* globals used by interrupt service routine */
char duration[]={
	255,  /* long rest */
	128,  /*dotted half plus quarter (same as whole) */
	16,   /* 8th */
	32,   /* quarter */
	16,   /* 8th */
	32,   /* quarter */
	48,   /* dotted quarter */
	96,  /* dotted half */
	48, 
	31,1,
	16,   /* 8th */

	     /* shout till the */
	32, 48, /* rafters */
	96,  /* dotted half */
	95,1,
	128, /* Stand */
	16, 32,16, /* and drink a*/
	48, 32,16, 96, /* toast once again */
	16, 32,16,32,16,48,48,122,

	255
};

int pitch[]={
	0,
	201,  /* Eb */
	179,  /* F  */
	201,  /* Eb*/
	213,  /* D */
	201,
	150,  /* Ab*/
	119,  /* C */
	100,  /* Eb*/
	150,0,
	150,

	     /* shout till the */
	134, /* Bb*/
	150, 
	159, /* G */
	179,0,
	179,
	190,  /* E */
	179,
	159,
	179,
	225,  /* once*/
	239,  /* a */
	268,  // This frequency should be 268
	179,201,179,159,150,134,127,119, /* Let every loyal Maine man sing */

	0
};

char note=-1; //index of which note
int current_pitch=0;
char current_duration=1;


/************************************************************************/
/* This program borrows heavily from the blink program that makes an LED*/ 
/* glow brighter and dimmer.  This is an AM transmitter.  It uses two   */
/* timers.  Timer 1 handles the PWM for the AM signal.  Timer0 modulates*/ 
/* this signal by changing the duty cycle of timer 1 in an audio range. */
/************************************************************************/

int main(void) 
{
         update_clock_speed();
	 init_pwm();
	 TIMSK1= (1<<OCIE1A);	//Enables interrupts for compare on timer 1 
	 TIMSK2= (1<<TOIE2); 	//Enables interrupts for overflow on timer 2 
	 sei();

	 while(1) ;		//stay here forever still service interrupts


	 return 0;  		//never get here
}
//The first byte is an unsigned byte that is the amount to adjust 
//the OSCCAL register.  The next byte will be 0 or 1 depending on 
//whether the adjustment should be positive (0) or negative (1).
//The value 0xff is intentionally avoided to distinguish unprogrammed
//eeprom locations.



//read the first two bytes of eeprom, if they have been programmed
//use the first byte as an offset for adjusting OSCCAL and the second as
//the direction to adjust 0=increase, 1=decrease.
//Any offset is allowed and users are cautioned that it is possible to
// adjust the oscillator beyond safe operating bounds.
void update_clock_speed(void)
{
  char temp;
  temp=eeprom_read_byte((void *)1); //read oscillator offset sign 
                                    //0 is positive 1 is  negative
                                    //erased reads as ff (so avoid that)
  if(temp==0||temp==1)      //if sign is invalid, don't change oscillator
  {
      if(temp==0)
          {
             temp=eeprom_read_byte((void *)0);
                 if(temp != 0xff) OSCCAL+=temp;
          }
          else
          {
             temp=eeprom_read_byte((void *)0);
                 if(temp!=0xff) OSCCAL -=temp;
          }
  }
}



void init_pwm(void){
/*	TIMER 0 used for PWM output		*/
/*	PIN 11 (PD5) used to output PWM signal 	*/
	DDRD = 0x60; 		// Sets PD5 as an Output
	OCR0B = 1;		//initial compare 
	TCCR0A = 0x33;		//COM1B = 1:1 and WGM[0:1] = 1:1 
	TCCR0B = 0x09; 		//internal clock no prescaling amd WGM[2] = 1
       				//WGM is PWM mode 7	
	OCR0A = 13; 		//AM radio modulation at 8 MHz / 14 = 570 kHz 
				//second harmonic at about 1140 khz
			
/*	TIMER 2 used for duration		*/
	TCCR2B = 0x06;		//clock with 256 prescaler
	current_pitch = 0;	
	current_duration = 1;

/*	TIMER 1 used for audio frequencies	*/
	TCCR1A = 0x00;		//CTC mode OCR1A at top value
	TCCR1B = 0x0B;		//set prescaler to 64 and  sets WGM21 = 1
				//CTC mode 4
	OCR1A = 1;		//set initial compare
}


ISR(TIMER1_COMPA_vect) 
{  // update OCR0B to change duty cycle
	if(current_pitch !=0) OCR0B^=6;  //swap between 1 and 7 (square wave)
	else OCR0B=1;                    // during rest do low amplitude
}

ISR(TIMER2_OVF_vect)
{  //indexs each note and sets pitch to OCR1A
	current_duration--;
	if(current_duration==0) 
	{
		note++;
		if(note>=sizeof(duration))note=0;
		current_pitch = pitch[note];
		current_duration = duration[note];
		if(current_pitch !=0){
			TCNT1 = 0;	//resets timer after each note	
		       	OCR1A=current_pitch;
		}
	}
}

