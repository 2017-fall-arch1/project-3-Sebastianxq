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

#define GREEN_LED BIT6
#define RED_LED BIT0


//Create a rectangle of the given dimensions
AbRect rect10 = {abRectGetBounds, abRectCheck, {3,3}};

AbRect playerPaddle = {abRectGetBounds, abRectCheck, {2,12}};

AbRect enemyPaddle = {abRectGetBounds, abRectCheck, {2,12}};

//creates a right arrow of the given dimension
AbRArrow rightArrow = {abRArrowGetBounds, abRArrowCheck, 20};

//Self explanatory, the edges of the playing field
AbRectOutline fieldOutline = { 
  abRectOutlineGetBounds, abRectOutlineCheck,   
  {screenWidth/2 - 5, screenHeight/2 - 5}
};

//enemy layer
Layer enemyPaddleLayer = {	    
  (AbShape *)&enemyPaddle,
  {screenWidth-15, screenHeight/2}, /**< center */
  {0,0}, {0,0},				    /* last & next pos */
  COLOR_RED,
  0,
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
  (AbShape *)&circle10,
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
//MovLayer ml3 = { &layer3, {1,1}, 0 }; /**< not all layers move */
MovLayer ml1 = { &enemyPaddleLayer, {0,2},0 }; 
MovLayer ml0 = { &ballLayer, {1,1}, &ml1}; 

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
void mlAdvance(MovLayer *ml, Region *fence, Region *paddle)
{
  Vec2 newPos;
  u_char axis;
  Region shapeBoundary; /*the ever changing mov boundary */
  for (; ml; ml = ml->next) {
    vec2Add(&newPos, &ml->layer->posNext, &ml->velocity);
    abShapeGetBounds(ml->layer->abShape, &newPos, &shapeBoundary);
    for (axis = 0; axis < 2; axis ++) {
      //conditional for fence
      //checks that balls topleft/bottom right is always inside fences.
      if ((shapeBoundary.topLeft.axes[axis] < fence->topLeft.axes[axis]) ||
	  (shapeBoundary.botRight.axes[axis] > fence->botRight.axes[axis]) ) {
	int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
	newPos.axes[axis] += (3*velocity);
      }	/**< if outside of fence */
      
      if ( (shapeBoundary.topLeft.axes[0] < paddle->botRight.axes[0])     &&
	   (shapeBoundary.botRight.axes[1] < paddle->botRight.axes[1])   &&
	   (shapeBoundary.topLeft.axes[1] > paddle->topLeft.axes[1]) )
      {    
      int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
      newPos.axes[axis] += (2*velocity);
      }	/**< if inside of paddle */
      
    } /**< for axis */

    //POINT DETECTION
    /* if ( shapeBoundary.topLeft.axes[0] < paddle->topLeft.axes[0]){
      int velocity = ml->velocity.axes[axis] = -ml->velocity.axes[axis];
      newPos.axes[axis] += (2*velocity);
    }	/**< if ball passes paddle */
    ml->layer->posNext = newPos;
    
  } /**< for ml */ 
}


u_int bgColor = COLOR_WHITE;     /**< The background color */
int redrawScreen = 1;           /**< Boolean for whether screen needs to be redrawn */

Region fieldFence;		/**< fence around playing field  */
Region ePaddle;                  /** opponent on the right side */
Region urPaddle;

/** Initializes everything, enables interrupts and green LED, 
 *  and handles the rendering for the screen
 */
void main()
{
  P1DIR |= GREEN_LED;		/**< Green led on when CPU on */	      
  P1OUT |= GREEN_LED;

  configureClocks();
  lcd_init();
  shapeInit();
  p2sw_init(15);
  shapeInit();

  layerInit(&ballLayer);
  layerDraw(&ballLayer);


  layerGetBounds(&fieldLayer, &fieldFence);
  layerGetBounds(&enemyPaddleLayer, &ePaddle);
  layerGetBounds(&playerPaddleLayer, &urPaddle);


  enableWDTInterrupts();      /**< enable periodic interrupt */
  or_sr(0x8);	              /**< GIE (enable interrupts) */

  //Something about char creation here lol
  //u_char width = screenWidth, height = screenHeight;
  // drawString5x7(10,10, "Welcome to pong", COLOR_BLACK, COLOR_WHITE);

  for(;;) { 
    while (!redrawScreen) { /**< Pause CPU if screen doesn't need updating */
      P1OUT &= ~GREEN_LED;    /**< Green led off witHo CPU */
      or_sr(0x10);	      /**< CPU OFF */
    }
    P1OUT |= GREEN_LED;       /**< Green led on when CPU on */
    
    u_char width = screenWidth, height = screenHeight;
    drawString5x7(10,10, "Welcome to pong", COLOR_BLACK, COLOR_WHITE);
    u_int switchDisplay = p2sw_read(), i;
    char str[5];
    for (i = 0; i< 4; i++)
      str[i] = (switchDisplay & (1<<i)) ? '-' : '0'+i;
    str[4] = 0;
    drawString5x7(20,20, str, COLOR_BLACK, COLOR_WHITE);
    redrawScreen = 0;
    movLayerDraw(&ml0, &ballLayer);
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
    mlAdvance(&ml0, &fieldFence, &urPaddle);
      //mlAdvance(&ml0, &urPaddle);
      redrawScreen = 1;
    count = 0;
  }
  P1OUT &= ~GREEN_LED;		    /**< Green LED off when cpu off */
}
