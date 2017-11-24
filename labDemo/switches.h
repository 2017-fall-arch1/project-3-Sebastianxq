#ifndef switches_included
#define switches_included
//switch on LCD are S2.0,1,2,3
#define SW1 BIT0		/* switch 1 is p2.0 AKA S1  */
#define SW2 BIT1                /* 2 is p2.1 AKA S2 */
#define SW3 BIT2                /* 3 is p2.2 AKA S3 */
#define SW4 BIT3                /* 4 is p2.3 AKA S4 */

#define SWITCHES (SW1 | SW2 | SW3 | SW4)   /* 4 switches on this board */
//#define SWITCHES SW2
void switch_init();
void switch_interrupt_handler_sw1();
//void switch_interrupt_handler_sw2();
//void switch_interrupt_handler_sw3();
//void switch_interrupt_handler_sw4();

extern char switch_state_down,switch_state_down2,switch_state_down3,switch_state_down4, switch_state_changed; /* effectively boolean */
#endif // included
