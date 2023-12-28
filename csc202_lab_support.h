//*****************************************************************************
//*****************************    C Source Code    ***************************
//*****************************************************************************
//
// DESIGNER NAME: Bruce Link
//
//     FILE NAME: csc202_lab_support.h
//
//          DATE: 01/05/2022
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    This file contains the definitions of standard data types used and 
//    various CSC-202 support functions for Dragon12 Board. It adds some 
//    support for to display strings to the serial port.
//
//*****************************************************************************

#ifndef CSC202_SUPPORT_H_
#define CSC202_SUPPORT_H_

#include <stdio.h>

//-----------------------------------------------------------------------------
//                        Define symbolic constants
//-----------------------------------------------------------------------------

#define SUCCESS   0
#define FAILURE   -1

#define ENABLE    0
#define DISABLE   1

#define FALSE     0
#define TRUE      1

#define alt_println   alt_printf("\r\n", 0)


//-----------------------------------------------------------------------------
//                        Define Embedded Data Types
//-----------------------------------------------------------------------------

//  Integer Types
typedef   signed char       sint8;      // signed 8 bit values
typedef unsigned char       uint8;      // unsigned 8 bit values
typedef   signed short int  sint16;     // signed 16 bit values
typedef unsigned short int  uint16;     // unsigned 16 bit values
typedef   signed long  int  sint32;     // signed 32 bit values
typedef unsigned long  int  uint32;     // unsigned 32 bit values

//  Floating Point Types
typedef float  real32;                  // single precision floating values
typedef double real64;                  // double precision floating values


//  Register Types
typedef volatile uint8*  register8;     //  8-bit register
typedef volatile uint16* register16;    // 16-bit register
typedef volatile uint32* register32;    // 32-bit register

typedef unsigned short bool;            // Boolean




//-----------------------------------------------------------------------------
//                      Define Public Functions
//-----------------------------------------------------------------------------

void alt_printf(sint8 buffer[70], uint16 value);
void alt_printfL(sint8 buffer[70], uint32 value);
void alt_clear();



//-----------------------------------------------------------------------------
//                 Define Public Global Variables
//-----------------------------------------------------------------------------

uint8  test_reg8  = 0xC0;
uint16 test_reg16 = 0x0000;


#ifdef USE_SCI0
  #define send_char   outchar0
#else
  #define send_char   outchar1
#endif


//-----------------------------------------------------------------------------
//                             private functions
//-----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// NAME: Print a string message
//
// DESCRIPTION:
//    This function prints a string message to the terminal window. The 
//    string message can be up to 70 characters long. The string to print 
//    supports basic format of the C printf function. This version of the 
//    print function allows you to display 1 or 2 byte integer in the string.
//
// INPUT:
//   buffer  - the string array up to 70 characters to print
//   buffer  - a long integer value to be substituted in string formatting
//
// OUTPUT:
//   none
//
// RETURN:
//   none
//----------------------------------------------------------------------------
void alt_printf(sint8 buffer[70], uint16 value)
{
  uint16 i = 0;
  uint16 len = 0;
  sint8  string[80];

  len = sprintf(string, buffer, value);

  // Walk through array to send each character to serial port
  for (i = 0; i< len; i++)
  {
    send_char(string[i]);      
  } /* for */
  
} /* alt_printfL */


//----------------------------------------------------------------------------
// NAME: Print a string message
//
// DESCRIPTION:
//    This function prints a string message to the terminal window. The 
//    string message can be up to 70 characters long. The string to print 
//    supports basic format of the C printf function. This version of the 
//    print function allows you to display a long integer in the string.
//
// INPUT:
//   buffer  - the string array up to 70 characters to print
//   buffer  - a long integer value to be substituted in string formatting
//
// OUTPUT:
//   none
//
// RETURN:
//   none
//----------------------------------------------------------------------------
void alt_printfL(sint8 buffer[70], uint32 value)
{
  uint16 i = 0;
  uint16 len = 0;
  sint8  string[80];

  len = sprintf(string, buffer, value);

  // Walk through array to send each character to serial port
  for (i = 0; i< len; i++)
  {
    send_char(string[i]);      
  } /* for */
  
} /* alt_printfL */


//----------------------------------------------------------------------------
// NAME: Clear Terminal Screen
//
// DESCRIPTION:
//    This function sends the escape sequence to terminal window to clear 
//    the screen. Then it places the cursor in the home location. This works 
//    best when using a terminal emulator like PuTTY.
//
// INPUT:
//   none
//
// OUTPUT:
//   none
//
// RETURN:
//   none
//----------------------------------------------------------------------------
void alt_clear()
{
  // ESC to alt_clear screen
  send_char(0x1B);
  send_char('[');
  send_char('2');
  send_char('J');

  // ESC to home cursor
  send_char(0x1B);
  send_char('[');
  send_char('H');

}  /* alt_clear */





#endif /* CSC202_SUPPORT_H_ */