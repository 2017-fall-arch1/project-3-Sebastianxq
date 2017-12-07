# makfile configuration
CPU             	= msp430g2553
CFLAGS          	= -mmcu=${CPU} -Os -I../h
LDFLAGS		= -L../lib -L/opt/ti/msp430_gcc/include/ 

#switch the compiler (for the internal make rules)
CC              = msp430-elf-gcc
AS              = msp430-elf-gcc -mmcu=${CPU} -c

all:lab.elf

#additional rules for files
lab.elf: ${COMMON_OBJECTS} game.o wdt_handler.o ../lib/libTimer.a
	${CC} ${CFLAGS} ${LDFLAGS} -o $@ $^ -lTimer -lLcd -lShape -lCircle -lp2sw

#eventually change to game but for efficieny leave at g
g: lab.elf
	mspdebug rf2500 "prog $^"

clean:
	rm -f *.o *.elf
