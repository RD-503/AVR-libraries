/*
 * I2C.c
 *
 * Created: 3/21/2018 4:14:56 PM
 *  Author: Developer
 */ 

#include "I2C.h"

volatile static int8_t I2C_TransmitBuffer[I2C_TRANSMISSION_BUFFER_SIZE];
volatile static int8_t I2C_ReceiveBuffer[I2C_RECEPTION_BUFFER_SIZE];

static uint8_t I2C_MessageSize;

union I2C_StatusFlags I2C_StatusFlags = {0};										//	Reset status flags

void I2C_IO_Init (void)																//	Enable I2c outputs
{
	I2C_DDR &= ~(1 << I2C_SDA_PIN | 1 << I2C_SCL_PIN);								//	Disable DDR
	I2C_PORT |= 1 << I2C_SDA_PIN | 1 << I2C_SCL_PIN;								//	Turn on pull-up resistors
}

void I2C_Init (uint32_t clockFrequency, uint8_t prescaler)
{
	I2C_IO_Init();																	//	If there is no pull-up resistors in the electrical scheme, uncomment this line
	TWBR = ((F_CPU / clockFrequency) - 16) / (2 * prescaler);
	TWSR = I2C_PRESCALER_BITS;
	TWCR =	(1 << TWEN) |															//	Enable I2C
			(0 << TWIE) | (0 << TWINT) |											//	Turn off interrupts, set interrupt flag
			(0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) |								//	Not ACK, no star condition, no stop condition
			(0 << TWWC);															//	Clear Write Collision flag
}

void I2C_Start (void)
{
	TWCR =	(1 << TWEN) |															//	Enable I2C
			(1 << TWIE) | (1 << TWINT) |											//	Turn on interrupts, clear interrupt flag
			(0 << TWEA) | (1 << TWSTA) | (0 << TWSTO) |								//	Not ACK, send START condition, no stop condition
			(0 << TWWC);															//	Clear Write Collision flag
}

void I2C_Stop (void)
{
	TWCR =	(1 << TWEN) |															// 
			(1 << TWIE) | (1 << TWINT) |											// 
			(0 << TWEA) | (0 << TWSTA) | (1 << TWSTO) |								// 
			(0 << TWWC);															//	Clear Write Collision flag
}

uint8_t I2C_Busy (void)
{
	return (TWCR & (1 << TWIE));
}

void I2C_MT_TransmitData (int8_t* messageToTransmit, uint8_t messageSize)
{
	while (I2C_Busy());
	
	I2C_MessageSize = messageSize;
	I2C_TransmitBuffer[0] = messageToTransmit[0];
	if (!(messageToTransmit[0] & 0x01))
	{
		for (uint8_t byteCounter = 1; byteCounter < messageSize; byteCounter++)	I2C_TransmitBuffer[byteCounter] = messageToTransmit[byteCounter];
	}
	I2C_StatusFlags.all = I2C_FAIL_FLAG;
	I2C_Start();
}

uint8_t I2C_MR_ReceiveData (int8_t* placeToReceive, uint8_t messageSize)
{
	while (I2C_Busy());
	
	if (I2C_StatusFlags.transfer == 1)
	{
		for (uint8_t byteCounter = 0; byteCounter < messageSize; byteCounter++)	placeToReceive[byteCounter] = I2C_ReceiveBuffer[byteCounter];
	}
	
	return (I2C_StatusFlags.transfer);
}

ISR (TWI_vect)
{
	static uint8_t bufferPointer;
	
	switch(TWSR & I2C_STATUS_MASK)
	{
		case I2C_START:
		case I2C_REPEATED_START:	bufferPointer = 0;									//	
		case MT_SLA_ACK:
		case MT_DATA_ACK:
		{
			if (bufferPointer < I2C_MessageSize)
			{
				TWDR = I2C_TransmitBuffer[bufferPointer++];
				TWCR =	(1 << TWEN) |												// 
						(1 << TWIE) | (1 << TWINT) |								// 
						(0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) |					// 
						(0 << TWWC);												//	Clear Write Collision flag
			}
			else
			{
				I2C_StatusFlags.transfer = I2C_SUCCESS_FLAG;
				TWCR =	(1 << TWEN) |												// 
						(0 << TWIE) | (1 << TWINT) |								// 
						(0 << TWEA) | (0 << TWSTA) | (1 << TWSTO) |					// 
						(0 << TWWC);												//	Clear Write Collision flag
			}
			
			break;
		}
		
		case MT_SLA_NACK:
		case MT_DATA_NACK:
		{
			//  Send stop/start or stop followed by a start
			I2C_Start();
			
			break;
		}
		
		case MT_MR_ARBITRATION_LOST:
		{
			// Start will be transmitted when the bus becomes free
			I2C_Start();
			
			break;
		}

        case MR_DATA_ACK:   I2C_ReceiveBuffer[bufferPointer++] = TWDR;
        case MR_SLA_ACK:
        {
            if (bufferPointer < I2C_MessageSize)
            {
                TWCR =	(1 << TWEN) |															// 
			            (1 << TWIE) | (1 << TWINT) |											// 
			            (1 << TWEA) | (0 << TWSTA) | (0 << TWSTO) |								// 
			            (0 << TWWC);															//	Clear Write Collision flag
            }
            else
            {
                TWCR =	(1 << TWEN) |															// 
			            (1 << TWIE) | (1 << TWINT) |											// 
			            (0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) |								// 
			            (0 << TWWC);															//	Clear Write Collision flag
            }

            break;
        }
        
        case MR_SLA_NACK:
        {
	        //  Send stop/start or stop followed by a start
	        TWCR =	(1 << TWEN) |															//
					(0 << TWIE) | (1 << TWINT) |											//
				    (0 << TWEA) | (0 << TWSTA) | (1 << TWSTO) |								//
					(0 << TWWC);															//	Clear Write Collision flag

	        break;
        }
		
		case MR_DATA_NACK:
		{
			//  Send stop/start or stop followed by a start
			//I2C_ReceiveBuffer[bufferPointer++] = TWDR;	//???????????????????
			I2C_StatusFlags.transfer = 1;
			TWCR =  (1 << TWEN) |													// Enable TWI-interface and release TWI pins.
					(0 << TWIE) | (1 << TWINT) |									// Enable Interrupt and clear the flag.
					(0 << TWEA) | (0 << TWSTA) | (1 << TWSTO) |						// Initiate a STOP condition.
					(0 << TWWC);													//	Clear Write Collision flag
			
			break;
		}
		
		case I2C_BUS_ERROR:
		{
			I2C_Stop();

			break;
		}

		case I2C_NO_RELEVANT_STATE_INFORMATION:
		{
			TWCR =	(1 << TWEN) |															//
					(0 << TWIE) | (0 << TWINT) |											//
					(0 << TWEA) | (0 << TWSTA) | (0 << TWSTO) |								//
					(0 << TWWC);															//	Clear Write Collision flag

			break;
		}

		default:    break;
	}
}

/*		
			/ *  * /
		case ST_ARBITRATION_LOST_SLA_ACK:
			
			break;
			
			/ *  * /
		case ST_SLA_ACK:
			
			break;
		
			/ *  * /
		case ST_DATA_ACK:
			
			break;
		
			/ *  * /
		case ST_DATA_NACK:
			
			break;
		
			/ *  * /
		case ST_LAST_BYTE_ACK:
			
			break;
		
		
			/ *  * /
		case SR_ARBITRATION_LOST_SLA_ACK:
			
			break;
		
			/ *  * /
		case SR_SLA_ACK:
			
			break;
		
			/ *  * /
		case SR_GENERAL_CALL_ACK:
			
			break;
		
			/ *  * /
		case SR_ARBITRATION_LOST_GENERAL_CALL_ACK:
			
			break;
		
			/ *  * /
		case SR_PREVIOUSLY_ADDRESSEDD_DATA_ACK:
			
			break;
		
			/ *  * /
		case SR_PREVIOUSLY_ADDRESSEDD_DATA_NACK:
			
			break;
		
			/ *  * /
		case SR_PREVIOUSLY_ADDRESSEDD_GENERAL_CALL_DATA_ACK:
			
			break;
		
			/ *  * /
		case SR_PREVIOUSLY_ADDRESSEDD_GENERAL_CALL_DATA_NACK:
			
			break;
		
			/ *  * /
		case SR_STOP_OR_REPEATED_START:
			
			break;
		
		
			/ *  * /
		case NO_RELEVANT_STATE_INFORMATION:
			
			break;
		
			/ *  * /
		default:	break;
	}
}*/