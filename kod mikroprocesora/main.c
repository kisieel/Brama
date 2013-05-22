#include "main.h"
#include <stdlib.h>

USB_ClassInfo_CDC_Device_t VirtualSerial_CDC_Interface =
	{
		.Config =
			{
				.ControlInterfaceNumber   = 0,
				.DataINEndpoint           =
					{
						.Address          = CDC_TX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.DataOUTEndpoint =
					{
						.Address          = CDC_RX_EPADDR,
						.Size             = CDC_TXRX_EPSIZE,
						.Banks            = 1,
					},
				.NotificationEndpoint =
					{
						.Address          = CDC_NOTIFICATION_EPADDR,
						.Size             = CDC_NOTIFICATION_EPSIZE,
						.Banks            = 1,
					},
			},
	};

static FILE USBSerialStream;


void uart_put( unsigned char data )
{
	 while(!(UCSR1A & (1<<UDRE1)));
		UDR1=data;		        
}

void uart_puts(const char *s )
{
    while (*s)
      uart_put(*s++);
}

unsigned char uart_get( void ) 
{
	while ( !(UCSR1A & (1<<RXC1)) );
	return UDR1;
}

static volatile int16_t phoneBuffer[100];
static volatile int16_t bufferLength = 0;
static volatile int16_t callingNumber[10];
static volatile int numbers = 2;

unsigned char stringCheck(char *s)
{
	int i = 1;
	while (*s)
	{		
		if(phoneBuffer[i] != *s++)
			return 0;
		i++;			
	}
	
	return 1;
}

void openGate()
{
	PORTC |= (1 << PC6);
	_delay_ms(2000);
	PORTC &= ~(1 << PC6);
}
unsigned char checkNumber()
{
	for(int i = 0; i < 9; i++)
	{
		if(callingNumber[i] != phoneBuffer[i + 23])
			return 0;
	}	
	
	return 1;
}

void bufferCheck()
{
	if(bufferLength > 7 && stringCheck("\r\nRING\r\n") == 1)
	{		
		if(PIND && (1 << PD0)) // tryb wpuszczaj wybranych
		{		
			uart_puts("AT+CLCC\r");
			_delay_ms(200);
			
			for(int i = 40; i < 49; i++)
				callingNumber[i - 40] = phoneBuffer[i];
				
			for(int num = 1; num < numbers + 1; num++)
			{
				char buff[5];
				itoa(num, buff, 10);
				
						
				bufferLength = 1;	
				uart_puts("AT+CPBR=");
				uart_put(buff[0]);
				uart_put('\r');
				
				_delay_ms(200);
				
				if(checkNumber() == 1)
				{
					openGate();
					break;
				}
				
			}
		
		}		
		else // wpuszaczaj wszystkich
		{
			openGate();
		}
			
		
		bufferLength = 1;			
		
		uart_puts("AT+CHUP\r");
		_delay_ms(500);
		bufferLength = 1;	
	}


}

SIGNAL(USART1_RX_vect)
{
	int16_t c = UDR1;
	fputs(&c, &USBSerialStream);
	
	phoneBuffer[bufferLength] = c;
	bufferLength++;		
}

void TelephoneInit()
{	
	uart_puts("AT+CGMI\r\n");
	
	_delay_ms(1000);
	
	if(bufferLength < 5)
	{	

		PORTB |= (1 << PB7);
		_delay_ms(1000);
		PORTB &= ~(1 << PB7);
		_delay_ms(1000);
		PORTB |= (1 << PB7);
		_delay_ms(1000);
		PORTB &= ~(1 << PB7);
		_delay_ms(4000);
		PORTC |= (1 << PC7);
		_delay_ms(200);
		PORTC &= ~(1 << PC7);
		_delay_ms(1000);
		PORTC |= (1 << PC7);
		_delay_ms(200);
		PORTC &= ~(1 << PC7);
		
	}

	bufferLength = 1;

}

int main(void)
{
	SetupHardware();	 
	
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);
	
	GlobalInterruptEnable();
	
	_delay_ms(1000);
	
	TelephoneInit(); 

	for (;;)
	{
		int16_t b = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
		
		if(b > -1)
		{
			if(b == 'a')
			{
				
				for(int i = 0; i < bufferLength; i++)
					fputs(&phoneBuffer[i], &USBSerialStream);
		
				
			}
			else if(b == 'b')
			bufferLength = 1;
			else
			uart_put(b);
				
		}
		
		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();
		
		PORTB ^= (1 << PB5);	
		
		bufferCheck();
		
	}
}

void USARTInit(unsigned int ubrr_value)
{   
   UCSR1A |= (1 << U2X1);
   UCSR1B |= (1 << RXEN1) | (1 << TXEN1) | (1 << RXCIE1);
   UCSR1C |= (1 << UCSZ11) | (1 << UCSZ10);
   UBRR1 = ubrr_value;
   DDRD |= (1 << PD3);
}

void SetupHardware(void)
{
	MCUSR &= ~(1 << WDRF);
	wdt_disable();	
	
	DDRB = (1 << PB5) | (1 << PB7) | (1 << PB4) | (1 << PB6);	
	DDRC = (1 << PC6) | (1 << PC7);	
	
	DDRD &= ~(1 << PD0);
	PORTD |= (1 << PD0); // wybor trybu

	clock_prescale_set(clock_div_1);

	USB_Init();
	
	USARTInit(103);
}

void EVENT_USB_Device_Connect(void)
{
	PORTB |= (1 << PB4);
	PORTB &= ~((1 << PB5) | (1 << PB6));
}

void EVENT_USB_Device_Disconnect(void)
{
	PORTB |= (1 << PB5);
	PORTB &= ~((1 << PB4) | (1 << PB6));
}

void EVENT_USB_Device_ConfigurationChanged(void)
{
	bool ConfigSuccess = true;

	ConfigSuccess &= CDC_Device_ConfigureEndpoints(&VirtualSerial_CDC_Interface);

	
	if(ConfigSuccess)
	{
		PORTB |= (1 << PB5) | (1 << PB4) | (1 << PB6);
	}
	else
	{
		PORTB |= (1 << PB6);
		PORTB &= ~((1 << PB5) | (1 << PB4));
	}
}

void EVENT_USB_Device_ControlRequest(void)
{
	CDC_Device_ProcessControlRequest(&VirtualSerial_CDC_Interface);
}

