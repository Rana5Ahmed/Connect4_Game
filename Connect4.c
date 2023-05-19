// ******* Required Hardware I/O connections*******************
// Blue Nokia 5110
// ---------------
// Signal        (Nokia 5110) LaunchPad pin
// Reset         (RST, pin 1) connected to PA7
// SSI0Fss       (CE,  pin 2) connected to PA3
// Data/Command  (DC,  pin 3) connected to PA6
// SSI0Tx        (Din, pin 4) connected to PA5
// SSI0Clk       (Clk, pin 5) connected to PA2
// 3.3V          (Vcc, pin 6) power
// back light    (BL,  pin 7) not connected, consists of 4 white LEDs which draw ~80mA total
// Ground        (Gnd, pin 8) ground


#include "tm4c123gh6pm.h"
#include "Nokia5110.h"
#include "Random.h"
#include "TExaS.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdint.h>

// *************************** Images ***************************
// width=16 x height=10

#define BOARD_ROWS 6
#define BOARD_COLS 7

////////////// portF driver//////////////////////////////
void PortF_Init(void){ 
	volatile unsigned long delay;
  SYSCTL_RCGC2_R |= 0x00000020| 0x00000002;     // 1) B & F clock
  delay = SYSCTL_RCGC2_R;           // delay
  GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock PortF PF0
  GPIO_PORTF_CR_R = 0x1F;           // allow changes to PF4-0
  GPIO_PORTF_AMSEL_R = 0x00;        // 3) disable analog function
  GPIO_PORTF_PCTL_R = 0x00000000;   // 4) GPIO clear bit PCTL
  GPIO_PORTF_DIR_R = 0x0E;          // 5) PF4,PF0 input, PF3,PF2,PF1 output
  GPIO_PORTF_AFSEL_R = 0x00;        // 6) no alternate function
  GPIO_PORTF_PUR_R = 0x11;          // enable pullup resistors on PF4,PF0
  GPIO_PORTF_DEN_R = 0x1F;          // 7) enable digital pins PF4-PF0
}
// Set the specified output pin on Port F
void PortF_SetPin(uint8_t pin)
{
    GPIO_PORTF_DATA_R |= (1 << pin);
}

// Clear the specified output pin on Port F
void PortF_ClearPin(uint8_t pin)
{
    GPIO_PORTF_DATA_R &= ~(1 << pin);
}

// Read the specified input pin on Port F
uint8_t PortF_ReadPin(uint8_t pin)
{
    return (GPIO_PORTF_DATA_R & (1 << pin)) ? 1 : 0;
}

/////////////////////////////////////////////////////////////////

///////////////port B Driver//////////////////////
// Initialize Port B as a GPIO port
void PortB_Init(void)
{
    volatile uint32_t delay;
    SYSCTL_RCGCGPIO_R |= 0x02;           // Enable clock for Port B
    delay = SYSCTL_RCGCGPIO_R;           // Delay for clock stabilization
    GPIO_PORTB_LOCK_R = 0x4C4F434B;      // Unlock Port B
    GPIO_PORTB_CR_R = 0xFF;              // Allow changes to PB0-PB7
    GPIO_PORTB_AMSEL_R &= ~0xFF;         // Disable analog function
    GPIO_PORTB_PCTL_R &= ~0xFFFFFFFF;    // GPIO clear bit PCTL
    GPIO_PORTB_DIR_R |= 0x03;            // PB0-PB1 outputs, PB2-PB7 inputs
    GPIO_PORTB_AFSEL_R &= ~0xFF;         // No alternate functions
    GPIO_PORTB_DEN_R |= 0xFF;	// Digital enable PB0-PB7// Set the specified output pin on Port B
	
}

void PortB_SetPin(uint8_t pin)
{
    GPIO_PORTB_DATA_R |= (1 << pin);
}

// Clear the specified output pin on Port B
void PortB_ClearPin(uint8_t pin)
{
    GPIO_PORTB_DATA_R &= ~(1 << pin);
}

// Read the specified input pin on Port B
uint8_t PortB_ReadPin(uint8_t pin)
{
    return (GPIO_PORTB_DATA_R & (1 << pin)) ? 1 : 0;
}
//////////////////////////////////////////

void printBoard(char *board);
int takeTurn(char *board, int player, const char*);
int takeTurnRemote(char *board, int player, const char*);
int takeTurnAI(char *board, int player, const char* , int i);
int hasEmptyCol(char *board, int col  ); 
int changeBoard(char *board,int player , const char *PIECES , int col );
int checkWin(char *board);
int checkFour(char *board, int, int, int, int);
int horizontalCheck(char *board);
int verticalCheck(char *board);
int diagonalCheck(char *board);
void Delay100ms(unsigned long count); // time delay in 0.1 seconds


#define GPIO_PORTF_DATA_R       (*((volatile unsigned long *)0x400253FC))
#define GPIO_PORTF_DIR_R        (*((volatile unsigned long *)0x40025400))
#define GPIO_PORTF_AFSEL_R      (*((volatile unsigned long *)0x40025420))
#define GPIO_PORTF_PUR_R        (*((volatile unsigned long *)0x40025510))
#define GPIO_PORTF_DEN_R        (*((volatile unsigned long *)0x4002551C))
#define GPIO_PORTF_LOCK_R       (*((volatile unsigned long *)0x40025520))
#define GPIO_PORTF_CR_R         (*((volatile unsigned long *)0x40025524))
#define GPIO_PORTF_AMSEL_R      (*((volatile unsigned long *)0x40025528))
#define GPIO_PORTF_PCTL_R       (*((volatile unsigned long *)0x4002552C))
#define SYSCTL_RCGC2_R          (*((volatile unsigned long *)0x400FE108))
#define SYSCTL_RCGC2_GPIOF      0x00000020  // port F Clock Gating Control
