
#include "LPC8xx.h"
#include "syscon.h"
#include "rom_api.h"
#include "ctimer.h"
#include "uart.h"
#include "swm.h"
#include "lib_ENS_II1_lcd_v2.h"
#include "utilities.h"
#include "adc.h"
#include <stdio.h>


#define BP1 LPC_GPIO_PORT->B0[13]
#define BP2 LPC_GPIO_PORT->B0[12]



int main(void) {
	uint8_t previousBP2 = BP2;
	uint8_t tosend_first_part_data = 0;
	uint8_t game = 0;
	uint16_t resultat = 0;
	int i;


// Initialisation de la liaison série
    LPC_PWRD_API->set_fro_frequency(30000); //set clock frequency
    LPC_SYSCON->SYSAHBCLKCTRL0 |= GPIO |IOCON|SWM| UART0; // Enable clocks to relevant peripherals
    LPC_GPIO_PORT->DIR0 = (1<<17);
	ConfigSWM(U0_TXD, P0_4); // Connect UART0 TXD, RXD signals to port pins
	ConfigSWM(U0_RXD, P0_0); // Connect UART0 TXD, RXD signals to port pins
    LPC_SYSCON->UART0CLKSEL = FCLKSEL_FRO_CLK; // Select frg0clk as the source for fclk0 (to UART0)
    LPC_SYSCON->PRESETCTRL0 &= (UART0_RST_N); //reset USART0
    LPC_SYSCON->PRESETCTRL0 |= ~(UART0_RST_N); //reset USART0
    LPC_USART0->BRG = (15000000.0/((LPC_USART0->OSR+1)*115200) - 1 +0.5);
    //LPC_USART0->CFG = DATA_LENG_8|PARITY_EVEN|STOP_BIT_1; //8bits transmission, 1 bit de stop et 1 bit de parité
    LPC_USART0->CFG = DATA_LENG_8|PARITY_NONE|STOP_BIT_1; //8bits transmission, 1 bit de stop et 0 bit de parité
    LPC_USART0->STAT = 0xFFFF; // Clear any pending flags, just in case
    LPC_USART0->CFG |= UART_EN; // Enable USART0

    // Enable the USART0 RX Ready Interrupt
    LPC_USART0->INTENSET = RXRDY;
    NVIC_EnableIRQ(UART0_IRQn);


//Initialisation du convertisseur Aalogique/Numérique
    // Step 1. Power up and reset the ADC, and enable clocks to peripherals
	LPC_SYSCON->PDRUNCFG &= ~(ADC_PD); //power the ADC
	LPC_SYSCON->SYSAHBCLKCTRL0 |= (ADC | GPIO0 | GPIO_INT | IOCON | SWM); //enable the clock
	LPC_SYSCON->ADCCLKDIV = 1;  // Enable clock, and divide-by-1 at this clock divider
	LPC_SYSCON->ADCCLKSEL = ADCCLKSEL_FRO_CLK; // Use fro_clk as source for ADC async clock

	// Step 3. Configure the external pins we will use
	//We use PIO0_10 & PIO0_15 (ADCR et ADCL) => ADC_8 & ADC_7
	LPC_SWM->PINENABLE0 &= ~(ADC_8 | ADC_7); // value 0 far enable
	// ConfigSWM(, P0_15);

	LPC_IOCON->PIO0_15 = 0; //disable pull-up and pull-down
	LPC_IOCON->PIO0_10 = 0; //disable pull-up and pull-down

	LPC_ADC->SEQA_CTRL &= ~(1<<31); // arret du convertisseur
	LPC_ADC->SEQB_CTRL &= ~(1<<31); // arret du convertisseur
	//LPC_ADC->SEQA_CTRL |=  (1<<31); //démarrage du convertisseur
	//LPC_ADC->SEQA_CTRL |= (1<<8) | (1<<27); //activate burst and channel 8




    while (1) {

    	// Infinite loop for waiting the player to start the game
    	if (previousBP2 == 0 && BP2 != 0)
		{
			game = 1;
    		if (((LPC_USART0->STAT) & TXRDY) != 0)
			{
				LPC_USART0->TXDAT  = 's';
			}
		}
    	previousBP2 = BP2;


    	//Infinite loop from when the player starts the game
    	/*	It sends 'g' on the serial port using LPC_USART0->TXDAT
    	 *  Then it reads and sends the data from the ADC SEQA_GDAT
    	 *  Then it send 'd' and do the same with the ADC SEQB_GDAT
    	 *
    	 *  Hence, at each loop, the algorithm sends :" 'g' + 2 bytes corresponding
    	 *  to the digital analog reading of an infrared sensor + 'd' + 2 bytes
    	 *  corresponding to the digital analog reading of the other infrared sensor "
    	 *
    	 * I chose to send data continuously and manage on Python what/when i want to read
    	 * rather than sending from Python a message to ask the LPC804 to read and to send data from
    	 * the sensors. It might be faster
    	 *
    	 */

		while(game){

			//SENSOR G (left)

			while ( ((LPC_USART0->STAT) & TXRDY) == 0 ){} //Waiting for the TX to be ready

			if (((LPC_USART0->STAT) & TXRDY) != 0){
				LPC_USART0->TXDAT  = 'g';} //Sending letter 'g' so Python recognise which sensor is sending data

			LPC_ADC->SEQA_CTRL &= ~(1<<31); // arret du convertisseur
			LPC_ADC->SEQB_CTRL &= ~(1<<31); // arret du convertisseur
			LPC_ADC->SEQA_CTRL |=  (1<<31); //démarrage du convertisseur
			LPC_ADC->SEQA_CTRL |= (1<<8) | (1<<27); //activate burst and channel 8

			//for (i = 0; i < 100000; i++); //boucle de temporisation

			if( ((LPC_ADC->SEQA_GDAT)&(1<<31)) != 0) //bit31 sets to 1 when an A/D conversion on this channel completes.
			{
				resultat = LPC_ADC->SEQA_GDAT&(0b00001111111111110000);  //Read RESULT (bit 4:15) from table 278
				resultat = (resultat >> 4);
				//printf(" SEQA_GDAT resultat = %d \n", resultat );
			}
			LPC_ADC->SEQA_CTRL &= ~((1<<8) | (1<<27)); //desactivate burst and channel 8
			LPC_ADC->SEQA_CTRL &= ~(1<<31); // arret du convertisseur afin de pouvoir utiliser l'autre capteur

			while ( ((LPC_USART0->STAT) & TXRDY) == 0 ){} //Waiting for the TX to be ready


			tosend_first_part_data = resultat&0b11111111;

			if (((LPC_USART0->STAT) & TXRDY) != 0){LPC_USART0->TXDAT  = tosend_first_part_data;} // send the first byte of resultat

			while ( ((LPC_USART0->STAT) & TXRDY) == 0 ){} //Waiting for the TX to be ready

			if (((LPC_USART0->STAT) & TXRDY) != 0){LPC_USART0->TXDAT  = (resultat >> 8);} // send the 2nd byte of resultat



			// SENSOR D (right)

			for (i = 0; i < 100000; i++); //boucle de temporisation

			while ( ((LPC_USART0->STAT) & TXRDY) == 0 ){} //Waiting for the TX to be ready

			if (((LPC_USART0->STAT) & TXRDY) != 0){
				LPC_USART0->TXDAT  = 'd';} //Sending letter 'd' so Python recognise which sensor is sending data

			LPC_ADC->SEQA_CTRL &= ~(1<<31); // arret du convertisseur
			LPC_ADC->SEQB_CTRL &= ~(1<<31); // arret du convertisseur
			LPC_ADC->SEQB_CTRL |=  (1<<31); //démarrage du convertisseur
			LPC_ADC->SEQB_CTRL |= (1<<7) | (1<<27); //activate burst and channel 7

			for (i = 0; i < 100000; i++); //boucle de temporisation

			if( ((LPC_ADC->SEQB_GDAT)&(1<<31)) != 0) //bit31 sets to 1 when an A/D conversion on this channel completes.
			{
				resultat = LPC_ADC->SEQB_GDAT&(0b00001111111111110000);  //Read RESULT (bit 4:15) from table 278
				resultat = (resultat >> 4);
				//printf(" SEQB_GDAT resultat = %d \n", resultat );
			}
			LPC_ADC->SEQB_CTRL &= ~((1<<7) | (1<<27)); //activate burst and channel 8
			LPC_ADC->SEQB_CTRL &= ~(1<<31); // arret du convertisseur afin de pouvoir utiliser l'autre capteur


			while ( ((LPC_USART0->STAT) & TXRDY) == 0 ){} //Waiting for the TX to be ready

			tosend_first_part_data = resultat&0b11111111;
			if (((LPC_USART0->STAT) & TXRDY) != 0){LPC_USART0->TXDAT  = tosend_first_part_data;} // send the first byte of resultat

			while ( ((LPC_USART0->STAT) & TXRDY) == 0 ){} //Waiting for the TX to be ready

			if (((LPC_USART0->STAT) & TXRDY) != 0){LPC_USART0->TXDAT  = (resultat >> 8);} // send the 2nd byte of resultat


			for (i = 0; i < 100000; i++); //boucle de temporisation

		}

    }
}
