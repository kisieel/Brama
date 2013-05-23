#include "main.h"


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
static volatile int16_t phoneBuffer[100];
static volatile int16_t bufferLength = 1;
static volatile int16_t callingNumber[10];
static volatile int16_t numbers = 0;
volatile static bool bDebug = false;
volatile int16_t iRead = 0;
volatile static bool ConfigSuccess = true;

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
unsigned char checkNumber(int corr)
{
	for(int i = 0; i < 9; i++)
	{
		if(callingNumber[i] != phoneBuffer[i + 23 + (2*corr)])
			return 0;
	}	
	
	return 1;
}

void bufferCheck()
{
	if(bufferLength > 7 && stringCheck("\r\nRING\r\n") == 1)
	{		
		if(PIND & (1 << PD0)) // tryb wpuszczaj wybranych
		{		
			uart_puts("AT+CLCC\r");
			_delay_ms(200);
			
			for(int i = 40; i < 49; i++)
				callingNumber[i - 40] = phoneBuffer[i];
				
			for(int num = 1; num < numbers + 1; num++)
			{
				char buff[5];
				itoa(num, buff, 10);
				
				int korekcja = 0;				
				if(num > 99)
				{
					korekcja = 2;
				}
				else if(num > 9)
				{
					korekcja = 1;
				}
				
						
				bufferLength = 1;	
				uart_puts("AT+CPBR=");
				
				for(int i = 0; i < korekcja+1; i++)
					uart_put(buff[i]);
				
				uart_put('\r');
				
				_delay_ms(300);
		
				
				if(checkNumber(korekcja) == 1)
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
	if(bDebug && ConfigSuccess)
		fputs(&c, &USBSerialStream); // do debugu
	
	phoneBuffer[bufferLength] = c;
	bufferLength++;		
}

void TelephoneInit()
{	
	uart_puts("AT+CGMI\r");
	
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
		
		_delay_ms(1000);
		uart_puts("AT+CGMI\r");
		_delay_ms(1000);
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
	
	numbers = eeprom_read_word(( uint16_t *)1) ;

	for (;;)
	{
	
		if(ConfigSuccess)
		{
			int16_t b = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
			
			if(b > -1)
			{
					
				if(b == '*')
					iRead = 0;	
				
				if(b == 0x1A)
				{
					numbers = iRead;
					eeprom_write_word((uint16_t*)1, (uint16_t)numbers);
				}			
					
				if(b == '\r' || b == 0x1A)
				{
					uart_put('\r');
					_delay_ms(300);
					bufferLength = 1;
					
					fputs("ok\r\n", &USBSerialStream);
				}
				
				if(b == '*' || b == '\r')
				{
					iRead++;
					uart_puts("AT+CPBW=");
					
					char buff[5];
					itoa(iRead, buff, 10);
					
					int korekcja = 0;				
					if(iRead > 99)
					{
						korekcja = 2;
					}
					else if(iRead > 9)
					{
						korekcja = 1;
					}
					
					for(int i = 0; i < korekcja+1; i++)
						uart_put(buff[i]);
						
					uart_put(',');
				}

				if(b > 47 && b < 58)
				{
					uart_put(b);			
				}
				
				if(b == 0x1B)
					openGate();
					
				if(b == 'd')
					bDebug = !bDebug;
									
			}
			
			CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
			USB_USBTask();
		}
			
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
	DDRC = (1 << PC6) | (1 << PC7);	//ledy i buttony
	
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

