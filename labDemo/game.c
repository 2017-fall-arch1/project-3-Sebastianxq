/** \EXACT COPY OF SHAPEMOTION.c
 *  \brief This is a simple shape motion demo.
 *  This demo creates two layers containing shapes.
 *  One layer contains a rectangle and the other a circle.
 *  While the CPU is running the green LED is on, and
 *  when the screen does not need to be redrawn the CPU
 *  is turned off along with the green LED.
 */

#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>
#include <lcddraw.h>
#include <p2switches.h>
#include <shape.h>
#include <abCircle.h>
#include "libTimer.h"    /* for buzzer */
#include "buzzer.h"      /* for buzzer */

#define GREEN_LED BIT6
#define RED_LED BIT0
static int points = 0;

AbRect rect10 = {abRectGetBounds, abRectCheck, {3,3}};
AbRect playerPaddle = {abRectGetBounds, abRectCheck, {2,12}};
AbRect enemyPaddle = {abRectGetBounds, abRectCheck, {2,12}};
AbRArrow rightArrow = {abRArrowGetBounds, abRArrowCheck, 20};
AbRect yourScoreArea = {abRectGetBounds, abRectCheck, {1,screenWidth-10}};
AbRect enemyScoreArea = {abRectGetBounds, abRectCheck, {1,screenWidth-10}};

//Self explanatory, the edges of the playing field
AbRectOutline fieldOutline = { 
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 5, screenHeight/2 - 10}
};

//Layer welcome = {
// u_char width = screenWidth, height = screenHeight;
// drawString5x7(10,10, "Welcome to pong", COLOR_BLACK, COLOR_WHITE);
Layer enemyScoreZone = {
  (AbShape *)&enemyScoreArea,
  {screenWidth-5,screenHeight/2}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLUE,
  0,
};
Layer yourScoreZone = {	    
  (AbShape *)&yourScoreArea,
  {screenWidth-122, screenHeight/2}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_BLUE,
  &enemyScoreZone,
};
//enemy layer
Layer enemyPaddleLayer = {	    
  (AbShape *)&enemyPaddle,
  {screenWidth-15, screenHeight/2}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  &yourScoreZone,
};

//player controlled paddle layer
Layer playerPaddleLayer = {	    
  (AbShape *)&playerPaddle,
  {screenWidth-(screenWidth-15), screenHeight/2}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_PURPLE,
  &enemyPaddleLayer,
};

//Layer for the right arrow
Layer arrow = {
  (AbShape *)&rightArrow,
  {(screenWidth/2)+10, (screenHeight/5)}, //< bit below & right of center 
  {0,0}, {0,0},				    // last & next pos 
  COLOR_BLACK,
  &playerPaddleLayer,
}; 


//Playing field layer
Layer fieldLayer = {	    
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  &arrow,
};

//Ball layer
Layer ballLayer = {
  (AbShape *)&circle5,
  {(screenWidth/2)+10, (screenHeight/2)+5}, /**< bit below & right of center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_ORANGE,
  &fieldLayer,
};

/** Moving Layer
 *  Linked list of layer references
 *  Velocity represents one iteration of change (direction & magnitude)
 */
typedef struct MovLayer_s {
  Layer *layer;
  Vec2 velocity;
  struct MovLayer_s *next;
} MovLayer;

//IF THERE IS NO OTHER MOVING LAYER SET TAIL TO 0

/* initial value of {0,0} will be overwritten, {col,row} velocity */
MovLayer ml3 = { &playerPaddleLayer, {0,2}, 0 }; /**< not all layers move */
MovLayer ml1 = { &enemyPaddleLayer, {0,2}, &ml3 };
MovLayer ml0 = { &ballLayer, {1,1}, &ml1}; 


void buzzerInit()
{
    /* 
       Direct timer A output "TA0.1" to P2.6.  
        According to table 21 from data sheet:
          P2SEL2.6, P2SEL2.7, anmd P2SEL.7 must be zero
          P2SEL.6 must be 1
        Also: P2.6 direction must be output
    */
    timerAUpmode();		/* used to drive speaker */
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL &= ~BIT7; 
    P2SEL |= BIT6;
    P2DIR = BIT6;		/* enable output to speaker (P2.6) */

    //buzzer_set_period(1000);	/* start buzzing!!! */
}

void buzzer_set_period(short cycles)
{
  CCR0 = cycles; 
  CCR1 = cycles >> 1;		/* one half cycle */
}
static unsigned int period = 1000;
static signed int rate = 200;	
#define MIN_PERIOD 1000
#define MAX_PERIOD 5000
void buzzerAdvance() 
{
  period += rate;
  if ((rate > 0 && (period > MAX_PERIOD)) || 
      (rate < 0 && (period < MIN_PERIOD))) {
    rate = -rate;
    period += (rate << 1);
  }
  buzzer_set_period(period);
}

void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    //u_int switches = p2sw_read();
    //if (switches){   
      //}
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */


  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Region bounds;
    layerGetBounds(movLayer->layer, &bounds);
    lcd_setArea(bounds.topLeft.axes[0], bounds.topLeft.axes[1], 
		bounds.botRight.axes[0], bounds.botRight.axes[1]);
    for (row = bounds.topLeft.axes[1]; row <= bounds.botRight.axes[1]; row++) {
      for (col = bounds.topLeft.axes[0]; col <= bounds.botRight.axes[0]; col++) {
	Vec2 pixelPos = {col, row};
	u_int color = bgColor;
	Layer *probeLayer;
	for (probeLayer = layers; probeLayer; 
	     probeLayer = probeLayer->next) { /* probe all layers, in order */
	  if (abShapeCheck(probeLayer->abShape, &probeLayer->pos, &pixelPos)) {
	    color = probeLayer->color;
	    break; 
	  } /* if probe check */
	} // for checking all layers at col, row
	lcd_writeColor(color); 
      } // for col
    } // for row
  } // for moving layer being updated
}	  

/** Advances a moving shape within a fence
 *  
 *  \param ml The moving shape to be advanced
 *  \param fence The region which will serve as a boundary for ml
 */

//need score,eScore
void mlAdvance(MovLayer *ml, Region *fence, Region *paddle, Region *enemy,
	       Region *enemyGotScore, Region *youGotScore)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary; /*the ever changing mov boundary */
  u_int switchDisplay = p2sw_read(), i;
  int switchPress = switchDisplay & (1<<i);
  int yourScore = 0;
  int enemyScore = 0;
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);

    if (switchPress!=1 && !(ml->next) ){
	int velocity = ml->velocity.axes[1] = -ml->velocity.axes[1];
      } /** If switch is pressed */

    for (axis = 0; axis < 2; axis ++) {
      //Why cant i have controls in here since collision works fine
      //Button Press detection
      // u_int switches = p2sw_read();
      
      //Fence Collision Detection
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	//buzzer_set_period(2000);
      }	/**< if outside of fence */

      //Your paddle collision detection 
      if(ml->next){
       if ( //If top left of shape is LEFT of right paddle X
	  (shapeBoundary.topLeft.axes[0] < paddle->botRight.axes[0])     &&
	  //If ball is above paddle 
	  (shapeBoundary.topLeft.axes[1] < paddle->botRight.axes[1]) &&   
	  //if top left y is LOWER of top left paddle Y
	  (shapeBoundary.topLeft.axes[1] > paddle->topLeft.axes[1]))
	   {  
      int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
      newPos.axes[axis] += (2*velocity);
      buzzer_set_period(750);
       }	/**< if inside of paddle */
      } /**<if the mov layer is NOT your paddle */
      
      //Enemy Paddle collision detection
      if(ml->next->next){
	//Once it passes the x axis
	if ((shapeBoundary.botRight.axes[0] > enemy->topLeft.axes[0]) &&  
	  //once it passes above
	  (shapeBoundary.botRight.axes[1] < enemy->botRight.axes[1]) && 
	  //once it passes below??
	  (shapeBoundary.botRight.axes[1] > enemy->topLeft.axes[1]))
	   {  
	 //int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	 //newPos.axes[axis] += (2*velocity);
	     newPos.axes[0] = screenWidth/2;
      newPos.axes[1] = screenHeight/2;
      //buzzerAdvance(); 
	 } 	/**< if inside of enemy */
      }
      //Your score zone collision detection
      if (shapeBoundary.topLeft.axes[0] < youGotScore->botRight.axes[0]){ 
	//Resets the coordinates to the center
	//newPos.axes[0] = screenHeight/2; //x
	//newPos.axes[1] = screenWidth/2; //y
	drawString5x7(60,0, ("Right: 0"), COLOR_RED, COLOR_WHITE);
	//buzzer_set_period(2000);
	points++;
	buzzerAdvance();
      }	/**< if inside your scoring */

      //Enemy score zone collision detection
      if (shapeBoundary.botRight.axes[0] > enemyGotScore->topLeft.axes[0]){
	points++;
	drawString5x7(10,0, "Left: 0", COLOR_BLUE, COLOR_WHITE);
	//buzzer_set_period(2000);
	buzzerAdvance();
      }	/**< if inside of Enemy Scoring */
      
    } /**< for axis */
    buzzer_set_period(0);
    ml->layer->posNext = newPos;
    
  } /**< for ml */ 
}


u_int bgColor = COLOR_WHITE;    /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */
Region ePaddle;                 /**< Enemy Paddle*/
Region urPaddle;                /**< Your paddle */
Region score;                   /**< lefthand scoring region */
Region enemyScore;              /**< Righthand scoring region */

typedef enum {welcomeMenu,play,gameOver,scored,end} states;

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  ///Have score update on subsequent hits
  //Have something that resets when the score zone is hit
  //Make player controls less clunky
  //Have sounds
  //Have win screen
  //For having a FSM, do states of the game!!
  //msp430gcc to compile it into assembly!!!
  
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */	      
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);
  shapeInit();
  buzzerInit();

  //inilitalize layers
  layerInit(&ballLayer);
  layerDraw(&ballLayer);

  //Initialize moving layers
  layerGetBounds(&fieldLayer, &fieldFence);
  layerGetBounds(&enemyPaddleLayer, &ePaddle);
  //layerGetBounds(&playerPaddleLayer, &urPaddle);
  layerGetBounds(&enemyScoreZone, &score);
  layerGetBounds(&yourScoreZone, &enemyScore);
  

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  //Something about char creation here lol
  //u_char width = screenWidth, height = screenHeight;
  // drawString5x7(10,10, "Welcome to pong", COLOR_BLACK, COLOR_WHITE);
  /* local variable definition */

  states state = welcomeMenu;
  while(state != end){
  switch(state){
    //welcomeMenu,play,score,gameOver, end
    case welcomeMenu:
      drawString5x7(10,10, ("welcome to pong"), COLOR_BLACK, COLOR_WHITE);
    redrawScreen = 1;
    state = play;
    break;
  case play:
    while(points<5){
      // for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    redrawScreen = 0;
    movLayerDraw(&ml0, &ballLayer);
    layerGetBounds(&playerPaddleLayer, &urPaddle);
    //}
    }
    state = scored;
    break;
  case scored:
    drawString5x7(10,10, "DONE", COLOR_BLACK, COLOR_WHITE);
    redrawScreen = 0;
    break;
  case gameOver:
    //redrawScreen = 1;
     drawString5x7(10,10, "test", COLOR_BLACK, COLOR_WHITE);
    break;
  default:
     drawString5x7((screenHeight/2),(screenWidth/2), "hmmm", COLOR_BLACK, COLOR_WHITE);
     break;
  
  }
  }
  /* for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
  //P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      //or_sr(0x10);	      /**< CPU OFF */
      //  }
//P1OUT |= GREEN_LED;       /**< Green led on when CPU on */

    //String manipulation, probably dont need anymore tbh
    /*u_char width = screenWidth, height = screenHeight;
    drawString5x7(10,10, "Welcome to pong", COLOR_BLACK, COLOR_WHITE);
    u_int switchDisplay = p2sw_read(), i;
    char str[5];
    for (i = 0; i< 4; i++)
      str[i] = (switchDisplay & (1<<i)) ? '-' : '0'+i;
    str[4] = 0;
    drawString5x7(20,20, str, COLOR_BLACK, COLOR_WHITE); */
    
/* redrawScreen = 0;
    movLayerDraw(&ml0, &ballLayer);
    layerGetBounds(&playerPaddleLayer, &urPaddle);
    } */
}

/** Watchdog timer interrupt handler. 15 interrupts/sec */
void wdt_c_handler()
{
  static short count = 0;
  P1OUT |= GREEN_LED;		      /**< Green LED on when cpu on */
  count ++;
  u_int switchDisplay = p2sw_read(), i;
  if (count == 15) {
    mlAdvance(&ml0, &fieldFence, &urPaddle, &ePaddle, &score, &enemyScore);
      redrawScreen = 1;
    count = 0;
  }
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
