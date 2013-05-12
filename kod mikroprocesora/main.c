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

void USARTWriteChar(unsigned char data)
{
   while(!(UCSR1A & (1<<UDRE1)));
   UDR1=data;
}

unsigned char USARTReadChar( void ) 
{
	PORTD &= ~(1<<2);
	while ( !(UCSR1A & (1<<RXC1)) );
	return UDR1;
}

SIGNAL(USART1_RX_vect)
{
	int16_t c = UDR1;
	fputs(&c, &USBSerialStream);
}

int main(void)
{
	SetupHardware();
    
	CDC_Device_CreateStream(&VirtualSerial_CDC_Interface, &USBSerialStream);

	PORTB |= (1 << PB6) | (1 << PB4);
	PORTB &= ~(1 << PB5);
	
	GlobalInterruptEnable();
	USARTWriteChar('o');
	USARTWriteChar('k');
	USARTWriteChar('\n');

	for (;;)
	{
		int16_t b = CDC_Device_ReceiveByte(&VirtualSerial_CDC_Interface);
		
		if(b > -1)
		{
			fputs(&b, &USBSerialStream);
			USARTWriteChar(b);			
		}
		
		CDC_Device_USBTask(&VirtualSerial_CDC_Interface);
		USB_USBTask();
		
		PORTB ^= (1 << PB5);
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
	
	
	DDRB = (1 << PB5) | (1 << PB7) | (1 << PB4) | (1 << PB5) | (1 << PB6);	
	DDRC = (1 << PC6) | (1 << PC7);
	

	PORTB |= (1 << PB7);
	_delay_ms(1000);
	PORTB &= ~(1 << PB7);
	_delay_ms(1000);
	PORTC |= (1 << PC7);
	_delay_ms(200);
	PORTC &= ~(1 << PC7); //telefon
	_delay_ms(1000);
	PORTC |= (1 << PC7);
	_delay_ms(200);
	PORTC &= ~(1 << PC7);
	

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

