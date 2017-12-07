## Introduction

This program creates a 2 player pong match on your msp430

## How To Play

The objective of the game is to score 5 points against your opponent.
- SW1 and SW2 control the up and down of Player 1 respectively
- SW3 and SW4 control the up and down of Player 1 respectively

## How To Run
Input "Make game" into the console and wait for the program to load.

## Method Descriptions
- buzzerInit: initializes buzzer bits

- buzzer_Set_period: sets a specific frequency that the buzzer will ring at

- buzzerAdvance: sets a frequency interval that the buzzer will ring at

- movLayerDraw: instantiates the moving objects and checks to see if they need to be redrawn

- mlAdvance: checks collision for all the moving objects

- main: instantiates needed librarys and progresses through the game states