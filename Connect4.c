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

unsigned long SW1,SW2;  // input from PF4,PF0
unsigned long Out;      // outputs to PF3,PF2,PF1 (multicolor LED)
void PortF_Init(void);

void UARTB_init(void);
char UARTB_InChar(void) ;
void UARTB_OutChar( char data);
void UARTB_outString(char* buffer) ;
int selectMode(void); 
void startingScreen(void);


int r = 0 , rr = 0 , uturn = 0 , f = 0 , i = 0 , j = 0 ;
int turn=0, done = 0;
const char *PIECES = "XO";
char board1[(BOARD_ROWS * BOARD_COLS)+1];
unsigned int seed = 25 ; 
int last = 0 , lastOne = -1 ;


void Set_LED1(void)
{
    GPIO_PORTF_DATA_R |= 0x40;   // turn on LED connected to pin 6
		GPIO_PORTF_DATA_R &= ~0x80;  // turn off LED connected to pin 7
}

void Clear_LED1(void)
{
    GPIO_PORTF_DATA_R &= ~0x40;  // turn off LED connected to pin 6
}

void Set_LED2(void)
{
    GPIO_PORTF_DATA_R |= 0x80;   // turn on LED connected to pin 7
		GPIO_PORTF_DATA_R &= ~0x40;  // turn off LED connected to pin 6
}

void Clear_LED2(void)
{
    GPIO_PORTF_DATA_R &= ~0x80;  // turn off LED connected to pin 7
}


int main(void){
  //TExaS_Init(SSI0_Real_Nokia5110_Scope);  // set system clock to 80 MHz
  // initialization goes here
	int mode ; 
	PortF_Init();        // Call initialization of port PF4, PF3, PF2, PF1, PF0
	for(i = 0 ; i < BOARD_COLS*BOARD_ROWS +1;i++)
	{
		board1[i] = ' ';
		//input[i]=' ';
	}
	
  Nokia5110_Init();
	UARTB_init();
  Nokia5110_Clear();
	startingScreen();
	mode = selectMode(); 
	srand(seed);
	Nokia5110_Clear();
	if(mode)
	{
	r = (rand()%9)+'0';
	UARTB_OutChar(r);
	rr = UARTB_InChar();
	
	if(r<rr)
	{
		uturn = 1 ;
	}

	printBoard(board1);
	for(turn = 0; turn < (BOARD_ROWS * BOARD_COLS) && !done; turn++){ // 42
      
		if(uturn)
		{
			do{
         printBoard(board1);
      }
			while(!takeTurnRemote(board1, (turn) % 2, PIECES));
			done = checkWin(board1);
			turn++;
			if(done) break;
			do{
         printBoard(board1);
      }
			while(!takeTurn(board1, (turn) % 2, PIECES));
			done = checkWin(board1);
		}
		else
		{
      do{
         printBoard(board1);
      }
			while(!takeTurn(board1, (turn) % 2, PIECES));
			done = checkWin(board1);
			turn++;
			if(done) break;
			do{
         printBoard(board1);
      }
			while(!takeTurnRemote(board1, (turn) % 2, PIECES));
			done = checkWin(board1);
		}
   }
	printBoard(board1);
	 if(turn == BOARD_ROWS * BOARD_COLS && !done){
     Nokia5110_OutString("It's a tie!");
   } else {
     turn--;
		 Nokia5110_Clear();
		 Nokia5110_SetCursor(1,1);
		Nokia5110_OutString("Player");
		Nokia5110_SetCursor(8,1);
		Nokia5110_OutString(turn%2==0?"X":"O");
		 Nokia5110_SetCursor(3,2);
		 Nokia5110_OutString("wins!");
		 Nokia5110_SetCursor(1,4);
		 Nokia5110_OutString("GAME OVER");
   }
	}
	else
	{
    
	printBoard(board1);
	for(turn = 0; turn < (BOARD_ROWS * BOARD_COLS) && !done; turn++){ // 42
			do{
         printBoard(board1);
      }
			while(!takeTurnAI(board1, (turn) % 2, PIECES , turn));
			done = checkWin(board1);
			turn++;
			if(done) break;
			do{
         printBoard(board1);
      }
			while(!takeTurn(board1, (turn) % 2, PIECES));
			done = checkWin(board1);
		
   }
	printBoard(board1);
	 if(turn == BOARD_ROWS * BOARD_COLS && !done){
     Nokia5110_OutString("It's a tie!");
   } else {
     turn--;
		 Nokia5110_Clear();
		 Nokia5110_SetCursor(1,1);
		Nokia5110_OutString("Player");
		Nokia5110_SetCursor(8,1);
		Nokia5110_OutString(turn%2==0?"X":"O");
		 Nokia5110_SetCursor(3,2);
		 Nokia5110_OutString("wins!");
		 Nokia5110_SetCursor(1,4);
		 Nokia5110_OutString("GAME OVER");
   }
	}
}

void startingScreen()
{
	int w; 
	Nokia5110_SetCursor(0,0);
	Nokia5110_ClearBuffer();
	Nokia5110_SetCursor(2,2);
	Nokia5110_OutString("Connect4");
	Nokia5110_SetCursor(0,5);
	Nokia5110_OutString("Loading ...");	
	Delay100ms(40);
	for(w=7 ; w>0 ; w--)
	{
		Nokia5110_SetCursor(0,w);
		Nokia5110_OutString("           ");
		Delay100ms(5);
	}

}
//////////////////////////////////////////////////////////////////////////////////////
int selectMode(){ // here is selecting if the mode is P1 VS AI or PI vs P2 
	int k = 0 ; 
	Nokia5110_SetCursor(0,0);
	Nokia5110_OutString("choose");
	Nokia5110_SetCursor(7,0);
	Nokia5110_OutString("mode");
	Nokia5110_SetCursor(3,2);
	Nokia5110_OutString("P1 VS AI");
	Nokia5110_SetCursor(3,4);
	Nokia5110_OutString("P1 VS P2");
	Nokia5110_SetCursor(0,k+2);	
	Nokia5110_OutString(">>");
	
	while(1)
	{
		SW1 = GPIO_PORTF_DATA_R&0x10;     // read PF4 into SW1
		if(!SW1)
		 {
				while (!(GPIO_PORTF_DATA_R&0x10)){
			 Delay100ms(1);
				}
				Nokia5110_SetCursor(0,k+2);	
				Nokia5110_OutString("  ");				
				k+=2;
				if(k>2)
					k=0;
				Nokia5110_SetCursor(0,k+2);	
				Nokia5110_OutString(">>");
		 }
	SW2 = GPIO_PORTF_DATA_R&0x01;     // read PF4 into SW2
		 Delay100ms(1);
		 if(!SW2)
		 {
			 while (!(GPIO_PORTF_DATA_R&0x01))
			 {
				 Delay100ms(1);
				 seed++; 
			 }
			 Nokia5110_SetCursor(3,k+2);
			 Nokia5110_OutString("        ");	
			Delay100ms(5);			 
			 Nokia5110_SetCursor(3,k+2);
			 Nokia5110_OutString(k?"P1 VS P2":"P1 VS AI");	
			Delay100ms(5);			 
			 Nokia5110_SetCursor(3,k+2);
			 Nokia5110_OutString("        ");		
			 Delay100ms(5);
			 Nokia5110_SetCursor(3,k+2);
			 Nokia5110_OutString(k?"P1 VS P2":"P1 VS AI");		
			 
				break ;
		 }
		 seed++;
	 }
	return  k ; 
}

void printBoard(char *board){
   int row, col;
	Nokia5110_ClearBuffer();
   for(row = 0; row < BOARD_ROWS; row++){
      for(col = 0; col < BOARD_COLS; col++){
				Nokia5110_SetCursorChar( col ,row,board[BOARD_COLS*row+col]);
      }
   }
}


	// take turn for the Kit player and PIECES can be X or O
int takeTurn(char *board, int player, const char *PIECES){
   int  col = 0;
		int g ;
   while(1){

		SW1 = GPIO_PORTF_DATA_R&0x10;     // read PF4 into SW1
		 Nokia5110_SetCursorChar( col ,0,PIECES[player]);
		 PortF_SetPin(1);
		 if(!SW1)
		 {
				while (!(GPIO_PORTF_DATA_R&0x10)){
			 Delay100ms(1);
				}
				col++;
			 if(col>6)
				 col = 0 ;
			Nokia5110_SetCursorChar( col ,0,PIECES[player]);
			  g = col==0?6:col-1;
			 Nokia5110_SetCursorChar( g,0,board[g]);
		 }
		 SW2 = GPIO_PORTF_DATA_R&0x01;     // read PF4 into SW2
		 Delay100ms(1);
		 if(!SW2)
		 {
			 while (!(GPIO_PORTF_DATA_R&0x01))
			 {
				 Delay100ms(1);
			 }
				PortF_ClearPin(1);
				break ;
		 }
   }
	 UARTB_OutChar(col+1+'0');
	 last = col ; 
   return changeBoard(board , player , PIECES , col );
}
