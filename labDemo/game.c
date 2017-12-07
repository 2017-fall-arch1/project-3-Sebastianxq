#include <msp430.h>
#include <libTimer.h>
#include <lcdutils.h>    /*used for shhape creation */
#include <lcddraw.h>
#include <shape.h>
#include <abCircle.h>
#include <p2switches.h>  /*used for switch manipulation */
#include "buzzer.h"      /* for buzzer */

#define GREEN_LED BIT6   /*sets bit for leds */
#define RED_LED BIT0
#define MIN_PERIOD 1000  /*buzzer advance parameters */
#define MAX_PERIOD 5000
static int points = 0;   /*universal point counter */
static unsigned int period = 1000;  /*/*buzzer advance parameters */
static signed int rate = 200;	

//Instantiates shapes
AbRect rect10 = {abRectGetBounds, abRectCheck, {3,3}};
AbRect playerPaddle = {abRectGetBounds, abRectCheck, {2,12}};
AbRect enemyPaddle = {abRectGetBounds, abRectCheck, {2,12}};
AbRect yourScoreArea = {abRectGetBounds, abRectCheck, {1,screenWidth-10}};
AbRect enemyScoreArea = {abRectGetBounds, abRectCheck, {1,screenWidth-10}};

//Self explanatory, the edges of the playing field
AbRectOutline fieldOutline = { 
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 5, screenHeight/2 - 10}
};

/*instantiates score zone for enemy's side */
Layer enemyScoreZone = {
  (AbShape *)&enemyScoreArea,
  {screenWidth-5,screenHeight/2}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  0,
};

/*instantiates score zone on your side */
Layer yourScoreZone = {	    
  (AbShape *)&yourScoreArea,
  {screenWidth-122, screenHeight/2}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_WHITE,
  &enemyScoreZone,
};

//enemy paddle
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

//Playing field layer
Layer fieldLayer = {	    
  (AbShape *) &fieldOutline,
  {screenWidth/2, screenHeight/2},/**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  &playerPaddleLayer,
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

/*All layers that move contained in a linked list, {col,row} velocity */
MovLayer ml3 = { &playerPaddleLayer, {0,0}, 0 }; 
MovLayer ml1 = { &enemyPaddleLayer, {0,0}, &ml3 };
MovLayer ml0 = { &ballLayer, {1,0}, &ml1}; 


/*initializes buzzer bits */
void buzzerInit()
{
    timerAUpmode();		/* used to drive speaker */
    P2SEL2 &= ~(BIT6 | BIT7);
    P2SEL &= ~BIT7; 
    P2SEL |= BIT6;
    P2DIR = BIT6;		/* enable output to speaker (P2.6) */
}

/*sets the period for buzzer*/
void buzzer_set_period(short cycles)
{
  CCR0 = cycles; 
  CCR1 = cycles >> 1;		/* one half cycle */
}

/*advances buzzer through a set interval */
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

/*advances through the moving layers */
void movLayerDraw(MovLayer *movLayers, Layer *layers)
{
  int row, col;
  MovLayer *movLayer;

  and_sr(~8);			/**< disable interrupts (GIE off) */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) { /* for each moving layer */
    Layer *l = movLayer->layer;
    l->posLast = l->pos;
    l->pos = l->posNext;
  }
  or_sr(8);			/**< disable interrupts (GIE on) */

  /*for each moving layer */
  for (movLayer = movLayers; movLayer; movLayer = movLayer->next) {
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

/*method for detecting collision with other objects */
void mlAdvance(MovLayer *ml, Region *fence, Region *paddle, Region *enemy,
	       Region *enemyGotScore, Region *youGotScore)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary; /*the ever changing mov boundary */
  u_int switchDisplay = p2sw_read(), i; /*used for switch detection */
  int switchPress = switchDisplay & (1<<i);
  
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);

     /* if switch is pressed on the player paddle layer */
    if((switchPress!=1) && !(ml->next) ){
     ml->velocity.axes[1] = -2;
      
    } /** If switch is pressed */
    
    for (axis = 0; axis < 2; axis ++) {
      //Fence Collision Detection
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (2*velocity);
	buzzer_set_period(2000);
	}	/**< if outside of fence */

      //Your paddle collision detection 
      if(ml->next){
       if ((shapeBoundary.topLeft.axes[0] < paddle->botRight.axes[0]) &&
	   (shapeBoundary.topLeft.axes[1] < paddle->botRight.axes[1]) &&   
	   (shapeBoundary.topLeft.axes[1] > paddle->topLeft.axes[1]))
       {  
        int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
        newPos.axes[axis] += (2*velocity);
        buzzer_set_period(750);
       }	/**< if inside of paddle */
      } /**<if the mov layer is NOT your paddle */
      
      //Enemy Paddle collision detection
      if(ml->next->next){
	//Once object passes closest x coor withing the given y coor
	if ((shapeBoundary.botRight.axes[0] > enemy->topLeft.axes[0]) &&  
	    (shapeBoundary.botRight.axes[1] < enemy->botRight.axes[1]) && 
	    (shapeBoundary.botRight.axes[1] > enemy->topLeft.axes[1]))
	 {  
	  int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	  newPos.axes[axis] += (2*velocity);
          buzzer_set_period(750);   
	 } 	/**< if inside of enemy */
      } /**<if layer is ball */
      
      //Your score zone collision detection
      if (shapeBoundary.topLeft.axes[0] < youGotScore->botRight.axes[0]){ 
	points++;
	drawString5x7(60,0, ("Right: 0"), COLOR_RED, COLOR_WHITE);
	buzzerAdvance();
      }	/**< if inside your scoring */

      //Enemy score zone collision detection
      if (shapeBoundary.botRight.axes[0] > enemyGotScore->topLeft.axes[0]){
	points++;
	drawString5x7(10,0, "Left: 0", COLOR_BLUE, COLOR_WHITE);
	buzzerAdvance();
      }	/**< if inside of Enemy Scoring */
      
    } /**< for axis */
    buzzer_set_period(0);  /*resets buzzer period */
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

/*States for the switch statement */
typedef enum {welcomeMenu,play,scored} states;

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */	      
  P1OUT |= GREEN_LED;

  configureClocks();            /**< initializes needed librarys/methods */
  lcd_init();
  shapeInit();
  p2sw_init(15);
  shapeInit();
  buzzerInit();

  layerInit(&ballLayer);        /**< inilitalize layers*/
  layerDraw(&ballLayer);
  
  layerGetBounds(&fieldLayer, &fieldFence); /**<Initialize moving layers*/
  layerGetBounds(&enemyScoreZone, &score);
  layerGetBounds(&yourScoreZone, &enemyScore);
  

  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  /*sets starting state, and continues to run state machine until 'end' is reached*/
  states state = welcomeMenu;
  while(state != scored){
  switch(state){
  case welcomeMenu: /*welcome screen before the game starts */
      drawString5x7(10,10, ("welcome to pong"), COLOR_BLACK, COLOR_WHITE);
      redrawScreen = 1;
      state = play;
      break;
  case play: /*play state, continues until score limit is reached */
      while(points>-1){
      while (!redrawScreen) { /**<Pause CPU if screen doesn't need updating */
        P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
        or_sr(0x10);	      /**< CPU OFF */
      }
      P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
      redrawScreen = 0;
      movLayerDraw(&ml0, &ballLayer);
      layerGetBounds(&playerPaddleLayer, &urPaddle);
      layerGetBounds(&enemyPaddleLayer, &ePaddle);
      }
      state = scored;
      break;
      
  case scored: /*final screen, reached when score limit is reached */
    drawString5x7(10,10, "Game Over!", COLOR_BLACK, COLOR_WHITE);
    redrawScreen = 0;
    break;
  default:
     drawString5x7((screenHeight/2),(screenWidth/2), "hmmm", COLOR_BLACK, COLOR_WHITE);
     break;
  
  }
  }
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
