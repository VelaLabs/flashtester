/**
 * @name Vela flash tester
 * @brief Test flash speed using Arduino
 * @copyright (C) 2015 Vela Labs Ltd
 * The MIT License (MIT) 
 * 
 * Permission is hereby granted, free of charge, to any person 
 * obtaining a copy of this software and associated documentation 
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, merge, 
 * publish, distribute, sublicense, and/or sell copies of the Software, 
 * and to permit persons to whom the Software is furnished to do so, 
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. 
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY 
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, 
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE 
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * /

/**
 * @file flashTester.ino
 * @author Matt Kane <matt@vela.io>
 */


static uint16_t triggerTime;
static uint16_t flashStart;

static uint8_t flash;
static uint8_t ignore;
static uint8_t triggering;
    
#define BUTTON_PIN 3
#define FLASH_PIN 4

void setup() {
    
    //Disable interrupts while we're fiddling with settings.
    cli();
    
    //Timer4 is our flash trigger pulse and housekeeping timer.
    //Enable overflow and output compare interrupts. Start turned off.
    TIMSK4 = _BV(TOIE4);
    TCCR4A = 0;
    TCCR4B = 0;
    OCR4A  = 1;
    
    //Timer1 has already been configured in wiring.c, but we want a different speed.
    
    //Mode 0: normal mode, no port outputs. 
    TCCR1A = 0;
    
    //This sets the clock to no prescale (16MHz).
    //Trigger on rising edge. 

    TCCR1B = _BV(CS10) | _BV(ICES1);
  
    //Enable analog comparator input capture.
    ACSR = _BV(ACIC);
  
    //Enable input capture interrupts
    TIMSK1  |= _BV(ICIE1);
  
    //Use multiplexer for comparator negative input.
    //This is our reference voltage.
    //Defaults to ADC0, i.e. PF0/Analog 5
    ADCSRB |= _BV(ACME);
    
    //Turn off the ADC as we'll be using its multiplexer.
    ADCSRA &= ~_BV(ADEN);

    //Turn off digital input on the comparator negative input (PE6)  
    DIDR1 |= _BV(AIN0D);
    
    //Enable input for start button (INT0/ digital 3) 
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    //Turn on external interrupts
    EIMSK = _BV(INT0);
    //Interrupt on falling edge
    EICRA = _BV(ISC01);
    

    //Flash is output
    pinMode(FLASH_PIN, OUTPUT);
    digitalWrite(FLASH_PIN, LOW);

    //Enable interrupts
    sei();
    Serial.begin(9600);    
    
}

void loop() {
    if(flash == 1) {
        
        //We were triggered by the button
        if(triggerTime > 0) {
            Serial.print("Delay ");
            Serial.print(clockCyclesToMicroseconds(flashStart - triggerTime), DEC);
            Serial.println("us");
        }
        
        
        Serial.print("Pulse ");
        Serial.print(clockCyclesToMicroseconds(ICR1), DEC);
        Serial.println("us");
        
        Serial.println("********");
        
        ignore = 0;
        flash = 0;
    }
}


// Input capture interrupt
ISR(TIMER1_CAPT_vect) {   
    
    if(bit_is_set(TCCR1B, ICES1)) {
        //We're currently triggering on rise. Change to fall
        TCCR1B &= ~_BV(ICES1);
        //Reset counter
        TCNT1 -= ICR1;
        flashStart = ICR1;
    } else {
        //We're triggering on fall. Change to rise.
        TCCR1B |= _BV(ICES1);
        flash = 1;     
    }
    
}

// Start button pressed
ISR(INT0_vect) {

    if(ignore == 1) {
        return;
    }
    
    ignore = 1;
    digitalWrite(FLASH_PIN, HIGH);
    triggerTime = TCNT1;
    //Reset and start timer4 with no prescaler
    TCNT4 = 0;
    TCCR4B = _BV(CS40);
    triggering = 1;
}

// Timer overflow
ISR(TIMER4_OVF_vect) {
    if(triggering == 1) {
        //Turn off the trigger pulse after 1 overflow (~40us)
        digitalWrite(FLASH_PIN, LOW);
    } else if(triggering == 100) {
        //Reset everything after 100 overflows (~4ms)
        //Turn off timer4
        TCCR4B = 0;
        ignore = 0;
        triggerTime = 0;
    }    
    triggering++;
}
