#include "SPI.h"

int8_t SPI_TransmitBuffer[SPI_BUFFER_SIZE];										//	Buffers initialization
int8_t SPI_ReceiveBuffer[SPI_BUFFER_SIZE];										//	

volatile uint8_t SPI_CharacterCounter = 0;										//	Initializing the counter


#ifdef SPI_MASTER_MODE

volatile bool SPI_TransmitSuccess = true;										//	Initializing the success flag
volatile uint8_t SPI_CharacterCounter;											//	Initializing the counter

void SPI_Master_Init (void)
{
	SPI_DDR |= 1 << SPI_MOSI | 0 << SPI_MISO | 1 << SPI_SCK_1 | 1 << SPI_SS;	//	SS pin as output
	SPCR |= 1 << SPIE | 1 << SPE | 1 << MSTR | 1 << SPR1 | 1 << SPR0;			//	SPI Interrupt enabled
	volatile uint8_t temp = SPSR;												//	Clear the interrupt flag
	temp = SPDR;																//	Clear the interrupt flag
}

void SPI_SendData (int8_t* data)
{
	for (uint8_t i = 0; data[i] != 0; i++)	SPI_TransmitBuffer[i] = data[i];	//	Putting the data in the buffer
	SPI_CharacterCounter = 0;													//	Reset character counter
	SPI_TransmitSuccess = false;												//	Clear the success flag before sending
	SPCR |= (1 << SPIE);														//	Activate SPI interrupt
 	volatile uint8_t temp = SPSR;												//	Clear the interrupt flag
 	temp = SPDR;																//	Clear the interrupt flag
	temp = 0x00;																//	Here assign any "garbage" value to temp
	SPDR = temp;																//	and put it in the SPDR to activate the transfer process
}

ISR (SPI_STC_vect)
{
//	SPI_ReceiveBuffer[SPI_CharacterCounter] = SPDR;								//	Receive byte from the Slave	(for full-duplex communication)
	if (SPI_TransmitBuffer[SPI_CharacterCounter] != 0)							//	If not end of data
	{
		SPDR = SPI_TransmitBuffer[SPI_CharacterCounter++];						//	Put the next byte in the SPI data register
	}
	else
	{
		SPCR &= ~(1 << SPIE);													//	Disable SPI interrupt
		SPI_TransmitSuccess = true;												//	Set the success flag
		SPI_CharacterCounter = 0;												//	Reset character counter
	}
}

#endif


#ifdef SPI_SLAVE_MODE												//	Slave functions

volatile bool SPI_ReceiveSuccess = true;													//	Initializing the success flag

void SPI_Slave_Init (void)
{
	volatile uint8_t temp;
	SPI_DDR |= 0 << SPI_MOSI | 1 << SPI_MISO | 0 << SPI_SCK_1 | 0 << SPI_SS;				//	SS pin as input
	SPCR |= 1 << SPIE | 1 << SPE;															//	Enable SPI interrupt
	temp = SPSR;	//	Clear the interrupt flag
	temp = SPDR;	//
}

void SPI_ReceiveData(void)
{
	//	
}

ISR (SPI_STC_vect)
{
	SPI_ReceiveBuffer[SPI_CharacterCounter] = SPDR;											//	Receive byte from the Master
//	SPDR = SPI_TransmitBuffer[SPI_CharacterCounter];										//	(For full-duplex communication)
	if (SPI_ReceiveBuffer[SPI_CharacterCounter] != 0)	SPI_CharacterCounter++;				//	If not the end of data, increment the size of the buffer
	else	SPI_CharacterCounter = 0;														//	Reset character counter
}

#endif