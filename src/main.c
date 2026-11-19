#define F_CPU 8000000UL   
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <avr/power.h>
#include <util/delay.h>

#define LED_PIN    PB0
#define BUZZER_PIN PB1
#define BUTTON_PIN PD2     
#define INACTIVITY_LIMIT 5  
volatile uint16_t inactivity_counter = 0;

void USART_init(unsigned int ubrr) {
    UBRR0H = (unsigned char)(ubrr >> 8);
    UBRR0L = (unsigned char)ubrr;
    UCSR0B = (1 << TXEN0);                   
    UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);  
}

void USART_print(const char *str) {
    while (*str) {
        while (!(UCSR0A & (1 << UDRE0)));  
        UDR0 = *str++;
    }
}

void Timer1_init(void) {
    TCCR1B |= (1 << WGM12) | (1 << CS12) | (1 << CS10);
    OCR1A = 7812;              
    TIMSK1 |= (1 << OCIE1A);   
}

ISR(TIMER1_COMPA_vect) {
    inactivity_counter++;
}

ISR(INT0_vect) {
    inactivity_counter = 0;  
}


void enterSleep(void) {
    USART_print("Entering POWER-DOWN sleep...\r\n");
    PORTB &= ~(1 << LED_PIN);       
    ADCSRA &= ~(1 << ADEN);          
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); 
    cli();
    sleep_enable();                  
    sleep_bod_disable();             
    EIFR |= (1 << INTF0);            
    sei();
    sleep_cpu();                   
    sleep_disable();                 
    ADCSRA |= (1 << ADEN);          
    USART_print("\nWoke up! Resuming normal operation.\r\n");

    PORTB |= (1 << BUZZER_PIN);
    _delay_ms(200);
    PORTB &= ~(1 << BUZZER_PIN);
}

int main(void) {
    /* I/O setup */
    DDRB |= (1 << LED_PIN) | (1 << BUZZER_PIN);
    DDRD &= ~(1 << BUTTON_PIN);
    PORTD |= (1 << BUTTON_PIN);     
    EICRA &= ~((1 << ISC01) | (1 << ISC00)); // explicit: low-level sensing
    EIMSK |= (1 << INT0);           // unmask INT0 in Interrupt Mask Register

    USART_init(51);                 // 9600 baud @ 8MHz (UBRR = F_CPU/16/baud - 1)
    Timer1_init();

    sei();                          // global interrupt enable (I-bit in SREG)

    USART_print("System started - NORMAL OPERATION\r\n");

    while (1) {
        /* Normal operation: LED blinks to show MCU is active */
        PORTB |= (1 << LED_PIN);
        _delay_ms(300);
        PORTB &= ~(1 << LED_PIN);
        _delay_ms(300);

        if (inactivity_counter >= INACTIVITY_LIMIT) {
            enterSleep();
            inactivity_counter = 0;
        }
    }
}
