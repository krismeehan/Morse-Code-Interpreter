//*****************************************************************************
//*****************************    C Source Code    ***************************
//*****************************************************************************
//
//  DESIGNER NAME:  Ethan Battaglia & Kris Meehan
//
//       LAB NAME:  Final Project
//
//      FILE NAME:  main.c
//
//-----------------------------------------------------------------------------
//
// DESCRIPTION:
//    This code is leveraged from the Programming the Dragon12-Plus-USB in C
//    and Assembly book by Haskell and Hanna. The code runs on the
//    Dragon12-Plus Light board.
//
//    This program translate from ASCII text to morse code and vice versa.
//    Additionally it can output the morse translation of ASCII text through
//    the LEDs and speakers at a rate that is adjustable via the potentiometer.      
//
//*****************************************************************************
//*****************************************************************************

//-----------------------------------------------------------
// Required Dragon12 Board support information
//-----------------------------------------------------------
#include <hidef.h>                  // common defines and macros
#include <mc9s12dg256.h>            // derivative information

#pragma LINK_INFO DERIVATIVE "mc9s12dg256b"

#include "main_asm.h"               // interface to the assembly module
#include "csc202_lab_support.h"     // include CSC202 Support
#include "queue.h"

//-----------------------------------------------------------
// Function Prototypes
//-----------------------------------------------------------
void my_print(sint8*);
void menu(void);
void speaker_leds_output(uint8, uint8);
void clear_array(void);
void delay_seg7(void);

//-----------------------------------------------------------
// Define symbolic constants used by program
//-----------------------------------------------------------
#define TOP_LEFT         0x00
#define BOT_LEFT         0x40
#define DELAY            100
#define LEDS_ON          0xFF
#define LEDS_OFF         0x00
#define FLASH3           3
#define SEG7_OFF_MASK    0xFF
#define DIG0             3
#define DIG1             2
#define DIG2             1
#define TEMP_MOTOR       5
#define DELAY300         300
#define NULL_CHAR        '\0'
#define TEMP_DIVIDE      2
#define TEMP_MULT        1.8
#define TEMP_ADD         32
#define DOT_DELAY        100
#define RETURN_KEY      '\r'
#define EMPTY_Q          1
#define CHAR_NOT_1       (character!='1')
#define CHAR_NOT_2       (character!='2')
#define CHAR_NOT_3       (character!='3')
#define CHAR_NOT_4       (character!='4')
#define CHAR_NOT_5       (character!='5')
#define PRGM_STOP        "Program Stopped"
#define NEW_LINE         "\n\r"
#define MENU_HEAD        "MENU OPTIONS"
#define MENU_1           "    1. Type ASCII message to be translated to morse code"
#define MENU_2           "    2. Type morse code to be translated to ASCII"
#define MENU_3           "    3. Output current message on LCD to speaker and LEDs in morse code"
#define MENU_4           "    4. Change decoding speed (length of beep for dots/dashes)"
#define MENU_5           "    5. End the program"
#define MENU_SELECTION   "Enter your selection:"
#define THANK_MSG        "Thank you for using the program"
#define SW5_BITMASK      1
uint8 g_end_program    =  FALSE;
uint8 g_enter_msg      =  FALSE;
uint8 g_found          =  FALSE;
uint8 g_pitch_idx      =  0;
uint8 g_default_speed  =  TRUE; 
uint8 g_hundreds_place =  1;
uint8 g_delay_flag     =  FALSE;
uint16 g_delay         =  DOT_DELAY;

// These are the two pitch values we use. 1434 is for dot, 2554 is for dash
int pitch_values[2] = 
{
  1434, 2554
};

// These are the ASCII values
char alphabet_numbers[36] = {
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I',
    'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R',
    'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '0',
    '1', '2', '3', '4', '5', '6', '7', '8', '9'
};

// These are the corresponding morse codes
char* morse_alphabet_numbers[36] = {
    ".- ", "-... ", "-.-. ", "-.. ", ". ", "..-. ", "--. ", ".... ", ".. ",
    ".--- ", "-.- ", ".-.. ", "-- ", "-. ", "--- ", ".--. ", "--.- ", ".-. ",
    "... ", "- ", "..- ", "...- ", ".-- ", "-..- ", "-.-- ", "--.. ", "----- ",
    ".---- ", "..--- ", "...-- ", "....- ", "..... ", "-.... ", "--... ", "---.. ", "----. "
};

void interrupt 13 alarm()
{
  tone(pitch_values[g_pitch_idx]);
}
void interrupt 25 sw5() 
{
  g_delay_flag = TRUE; 
  PIFH = 0xFF;               
}

void main(void) 
{
  char character;
  char* current_morse;
  char current_letter;
  char blank = ' ';
  char current_lcd[100] = "";
  char morse_letter[] = "";
  char morse_decoded[100] = "";
  
  static uint16 adc_val;
  
  uint8 i;
  uint8 j;
  uint8 k;
  uint8 char_count;
  uint8 idx = 0;
  uint8 morse_idx = 0;
  uint8 morse_letter_idx = 0;
  uint8 letter_complete = FALSE;

  // enable the needed peripherals. Disable others
  ad0_enable();
  seg7_enable();
  led_disable();              
  PLL_init();
  lcd_init(); 
  SCI1_init(9600);
  SW_enable();
  
  asm(cli);                   
  PIFH = 0xFF;          // clear interrupt flags
  PPSH = 0;             // interrupts occur on falling edge           
  PIEH = SW5_BITMASK;   // enable SW5 and SW2 interrupts
                                      
  clear_lcd(); 
  alt_clear(); 
  seg7dec(g_hundreds_place, DIG0);
  set_lcd_addr(0x02);
  type_lcd("Please Open");
  set_lcd_addr(0x45);
  type_lcd("PuTTY");
                                                                      
  while(!g_end_program)
  {
    // Displays system menu
    menu(); 
    // Waits for user input & stores to "character"
    character = inchar1(); 
    
    // loops until user inputs valid menu choice
    while(CHAR_NOT_1 && CHAR_NOT_2 && CHAR_NOT_3 && CHAR_NOT_4 && CHAR_NOT_5)
    {
      character = inchar1();
    } /* while */
    outchar1(character);
    
    // --------------
    // MENU OPTION 1
    // --------------
    // 1. Type ASCII message to be translated to morse code
    if(character == '1')
    {
      // Displays text prompt to SCI1 
      alt_clear();
      my_print("***Spaces represent separation of letters. Forward slashes(/) separate words***");
      my_print(NEW_LINE);
      my_print("Press ENTER to translate ASCII message to morse code:");
      my_print(NEW_LINE); 
      idx = 0;
      
      // clear current string  
      for (i = 0; i < 100; i++)
      {
        current_lcd[i] = '\0';
      } /* for */
      
      // Stores all inputted letters to the queue and exits when return is pressed 
      clear_lcd();
      set_lcd_addr(TOP_LEFT);
      
      while(!g_enter_msg)
      { 
        character = inchar1();        // wait for keyboard input
    
        if(character == RETURN_KEY)   
        {
          g_enter_msg = TRUE;
          character = blank;
        } /* if */

        outchar1(character);          // displays char in terminal
        data8(character);             // write message to LCD
        
        current_lcd[idx] = character;
        idx++;
        
        if(idx == 16)
        {
          set_lcd_addr(BOT_LEFT);
        } /* if */
        
      } /* while */
      my_print(NEW_LINE);
      
      // Loop through all of the inputted letters 
      for(i = 0; i <= strlen(current_lcd); i++)
      {
        current_letter = current_lcd[i];
        for(j = 0; j<=35; j++)
        {        
          if(current_letter == alphabet_numbers[j])
          {
            // Print out the morse version of our letter
            my_print(morse_alphabet_numbers[j]); 
          } /* if */       
        } /* for */
        if(current_letter == ' ')
        {
          my_print("/ "); 
        } /* if */ 
      } /* for */
      
      // Morse code translation has been displayed to SCI1, program waits
      // for user to press Enter to quit out of function 1
      g_enter_msg = FALSE;
      my_print(NEW_LINE);
      my_print("Press ENTER to return to the menu:");
      
      while(!g_enter_msg)
      {
        character = inchar1();
        if(character == RETURN_KEY)   // ends loop if user presses "enter" key
        {
          g_enter_msg = TRUE;
        } /* if */
      } /* while */
      g_enter_msg = FALSE;
    } /* if */
    
    // --------------
    // MENU OPTION 2
    // --------------
    // 2. Type morse code to be translated to ASCII
    if(character == '2')
    {
      // Displays text prompt to SCI1 
      alt_clear();
      my_print("***Spaces represent separation of letters. Forward slashes(/) separate words***");
      my_print(NEW_LINE);
      my_print("Press ENTER to translate morse code to ASCII message:");
      my_print(NEW_LINE); 
      idx = 0;
      
      // sets values of morse letter to null
      for (j = 0; j < 6; j++)
      {
        morse_letter[j] = '\0';
      } /* for */
      // sets values of decoded morse to null
      for (k = 0; k < 100; k++)
      {
        morse_decoded[k] = '\0';
      } /* for */
      morse_idx = 0;
      char_count = 0;
      
      while(!g_enter_msg)
      { 
        character = inchar1();        // wait for keyboard input
        outchar1(character);          // displays char in terminal
        
        // exit while loop if the user presses RETURN
        if(character == RETURN_KEY)   
        {
          g_enter_msg = TRUE;
          character = blank;
        } /* if */
        
        // Store the inputted character into morse_letter
        morse_letter[morse_idx] = character;  
        morse_idx++;
        
        if(character == '/') 
        {
          morse_decoded[char_count] = ' ';
          char_count++;
          
          morse_idx = 0; 
        } /* if */
  
        if(character == ' ')
        {  
          for (i = 0; i < 35; i++)
          {
            if(strcmp(morse_letter,morse_alphabet_numbers[i]) == 0)
            {
              // ASCII values are stored in morse_decoded
              morse_decoded[char_count] = alphabet_numbers[i];
              char_count++;     
            } /* if */
          } /* for */

          // sets values of morse letter to null 
          for (j = 0; j < 6; j++)
          {
            morse_letter[j] = '\0';
          } /* for */
          morse_idx = 0;
        } /* if */
      
      } /* while */
      my_print(NEW_LINE);
      my_print(morse_decoded);

      // Morse code translation has been displayed to SCI1, program waits
      // for user to press Enter to quit out of function 1
      g_enter_msg = FALSE;
      my_print(NEW_LINE);
      my_print("Press ENTER to return to the menu:");
      
      while(!g_enter_msg)
      {
        character = inchar1();
        if(character == RETURN_KEY)   // ends loop if user presses "enter" key
        {
          g_enter_msg = TRUE;
        } /* if */
      } /* while */
      g_enter_msg = FALSE;
    } /* if  */
    
    // --------------
    // MENU OPTION 3
    // --------------
    // 3. Output current message on LCD to SCI1 and speaker in morse code
    if(character == '3')
    {
      // Print the current ASCII message displayed on the LCD to SCI1
      alt_clear();
      my_print("Message being output: ");
      my_print(current_lcd);
      my_print(NEW_LINE);
      
      // For each of the LCD letters, find its morse counterpart and print to SCI1
      for(j = 0; j < strlen(current_lcd); j++)
      {
        current_letter = current_lcd[j];
        g_found = FALSE;
        
        for(i = 0; i <= 35; i++)
        {
          
          if(current_letter == alphabet_numbers[i])
          {
            g_found = TRUE;
            morse_idx = 0;
            current_morse = morse_alphabet_numbers[i];
            
            // Until we do not find a space bar
            while(current_morse[morse_idx] != ' ')
            {           
              if(current_morse[morse_idx] == '.') 
              {
                // We found a dot, so output higher-pitch sound for short time
                PTP = SEG7_OFF_MASK;
                speaker_leds_output(0, 1); 
              } /* if */
              if(current_morse[morse_idx] == '-')
              {
                // We found a dash, so output lower-pitch sound for long time
                PTP = SEG7_OFF_MASK;
                speaker_leds_output(1, 3);
              } /* if */
              
              morse_idx++;
              ms_delay(g_delay);
            } /* while */
            
            my_print(morse_alphabet_numbers[i]);
            ms_delay(g_delay*3);
          } /* if */      
          
        } /* for */
        if(!g_found) 
        {
          my_print("/ ");
          ms_delay(g_delay*6);  
        } /* if */   
      } /* for */
     
      // Morse code translation has been displayed to SCI1, program waits
      // for user to press Enter to quit out of function 1
      seg7dec(g_hundreds_place, DIG0);
      my_print(NEW_LINE);
      my_print("Press ENTER to return to the menu:");
      my_print(NEW_LINE);
      
      while(!g_enter_msg)
      {
        character = inchar1();
        if(character == RETURN_KEY)   // ends loop if user presses "enter" key
        {
          g_enter_msg = TRUE;
        } /* if */
      } /* while */
      g_enter_msg = FALSE;  
    } /* if */
    
    // --------------
    // MENU OPTION 4
    // --------------
    // 4. Change decoding speed. Uses potentiometer to set the length of beeps for dots/dashes
    if(character == '4')
    {
      alt_clear();
      my_print("    1. Use Potentiometer to change decoding speed");
      my_print(NEW_LINE);
      my_print("    2. Set to default speed (100ms)");
      my_print(NEW_LINE);
      my_print("Enter your selection:");   
      character = inchar1();
      
      // loops until user inputs valid menu choice
      while(CHAR_NOT_1 && CHAR_NOT_2)
      {
        character = inchar1();
      } /* while */
      outchar1(character);
      
      if (character == '1')
      {   
        my_print(NEW_LINE);
        my_print("Press SW5 to lock in decoding speed (50ms - 350ms):");
        while(!g_delay_flag) 
        {
        
          // Convert the potentiometer from 0-1024 to 50-350
          adc_val = (ad0conv(7)/3.4) + 50;
          if(adc_val<100)
            g_hundreds_place = 0;
          if(adc_val<200 && adc_val >= 100)
            g_hundreds_place = 1;
          if(adc_val<300 && adc_val >= 200)
            g_hundreds_place = 2;
          if(adc_val>=300)
            g_hundreds_place = 3;
          
          // Output scalar onto seven segment display
          seg7dec(g_hundreds_place, DIG0);
          ms_delay(100);
          g_delay = adc_val;
        }
        g_delay_flag = FALSE;
      } /* if */
      
      if (character == '2')
      {
        // Reset decode speed to default
        g_delay = DOT_DELAY;
        g_hundreds_place = 1;
      } /* if */
      
      // Prompt user for exit
      my_print(NEW_LINE);
      my_print("Press ENTER to return to the menu:");
      my_print(NEW_LINE);
      
      while(!g_enter_msg)
      {
        character = inchar1();
        if(character == RETURN_KEY)   // ends loop if user presses "enter" key
        {
          g_enter_msg = TRUE;
        } /* if */
      } /* while */
      g_enter_msg = FALSE;
    } /* if */ 
    
    // --------------
    // MENU OPTION 5
    // --------------
    // 5. End the program
    if(character == '5')
    {
      g_end_program = TRUE; 
      seg7_disable(); 
    } /* if */ 
  }
  // Program ends, displays exit messages to terminal and LCD
  clear_lcd();
  leds_off();
  alt_clear();
  seg7_disable();
  
  set_lcd_addr(TOP_LEFT);  
  type_lcd(PRGM_STOP);
  my_print(THANK_MSG);   
} /* main */

//-----------------------------------------------------------------------------
// DESCRIPTION:
//   This function takes a string and displays it to the terminal emulator one
//   character at a time until it reaches the null character (end of string).   
//
// INPUT PARAMETERS:
//   message - the message to be displayed
//
// OUTPUT PARAMETERS:
//   none
//
// RETURN:
//   none
//-----------------------------------------------------------------------------
void my_print(sint8* message)
{
  uint8 idx = 0;
  
  while(message[idx] != NULL_CHAR)
  {
    outchar1(message[idx++]);
  }
}  

//-----------------------------------------------------------------------------
// DESCRIPTION:
//   This function displays the menu of options on the terminal emulator when
//   called. It uses the "my_print" function to write each menu option.
//
// INPUT PARAMETERS:
//   none
//
// OUTPUT PARAMETERS:
//   none
//
// RETURN:
//   none
//-----------------------------------------------------------------------------
void menu(void)
{
  alt_clear();     
 
  my_print(MENU_HEAD);
  my_print(NEW_LINE);
  
  my_print(MENU_1);
  my_print(NEW_LINE);   
                 
  my_print(MENU_2);
  my_print(NEW_LINE);
  
  my_print(MENU_3);
  my_print(NEW_LINE);
  
  my_print(MENU_4);
  my_print(NEW_LINE);
  
  my_print(MENU_5);
  my_print(NEW_LINE);
  
  my_print(MENU_SELECTION);
}

//-----------------------------------------------------------------------------
// DESCRIPTION:
//   This function blinks the LEDs and speaker once to the desired tone and time
//
// INPUT PARAMETERS: 
//   pitch_idx - the tone at which the speaker will output sound
//   factor - multiply this with our decoding speed to find how long to beep
//
// OUTPUT PARAMETERS:
//   none
//
// RETURN:
//   none
//-----------------------------------------------------------------------------
void speaker_leds_output(uint8 pitch_idx, uint8 factor)
{
  // Turn LED and speaker on
  led_enable();
  g_pitch_idx = pitch_idx;
  sound_init();
  sound_on();
  leds_on(LEDS_ON);
  
  // Wait a while
  ms_delay(g_delay*factor);
  
  // Turn LED and speaker off
  leds_off();
  led_disable(); 
  sound_off();
  _asm CLI;   
}