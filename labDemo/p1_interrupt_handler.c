#include <msp430.h>
#include "switches.h"

/* Switch on P1 (S2) */
/*switched all the ones to 2 in a last ditch effort lol*/
void
__interrupt_vec(PORT2_VECTOR) Port_2(){
  if (P2IFG & SWITCHES) {	      /* did a button cause this interrupt? */
    P2IFG &= ~SWITCHES;		      /* clear pending sw interrupts */
    switch_interrupt_handler_sw1();
    //switch_interrupt_handler_sw2();/* single handler for all switches */
  }
}

