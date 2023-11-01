﻿REM @english
REM OTHER CONTRIBUTIONS GET/PUT PERFORMANCE
REM
REM This example attempts to calculate the speed of capturing 
REM and redrawing the ''GET IMAGE'' and ''PUT IMAGE'' primitives. The example is 
REM configurable to identify the weight of various factors 
REM in the operations.
REM
REM @italian
REM VARI ALTRI CONTRIBUTI PERFORMANCE GET/PUT
REM
REM Questo esempio cerca di calcolare la velocità di cattura e 
REM ridisegno delle primitive ''GET IMAGE'' e ''PUT IMAGE''. 
REM L'esempio è configurabile per individuare il peso di vari 
REM fattori nelle operazioni.
REM
REM @include coco3

CONST timeLimit = 50 * 10
CONST width = 32
CONST height = 32

BITMAP ENABLE ( 320, 192, 16 )
COLOR BORDER BLACK
CLS BLACK

VAR counter AS WORD = 0

LOCATE 0,0
PEN WHITE
PRINT "TESTING ON ";PEN(YELLOW);SCREEN WIDTH;"x";SCREEN HEIGHT;" (";SCREEN COLORS;" colors)";PEN(WHITE)
PRINT "  PICTURE SIZE = ";PEN(YELLOW);width;"x";height;PEN(WHITE)
PRINT "  DURATION = ";PEN(YELLOW);timeLimit / TICKS PER SECOND;" secs";PEN(WHITE)
PRINT
PRINT "PRESS ";PEN(RED);"ANY KEY";PEN(WHITE);" TO BEGIN TESTING"
PRINT
WAIT KEY
PRINT PEN(RED);"--- STARTING TEST ---"
PRINT

background := NEW IMAGE(#width, #height)

TIMER = 0

DO
    GET IMAGE background FROM 0, 0
    PUT IMAGE background AT 0, 0
    INC counter
    EXIT IF TIMER > timeLimit
LOOP

PRINT PEN(RED);"--- ENDING TEST ---";PEN(WHITE)
PRINT

DIM fps AS FLOAT, frames AS FLOAT, time AS FLOAT
DIM pxs AS FLOAT, pixels AS FLOAT
frames = counter
time = 10
pixels = 32 * 32
fps = frames / time
pxs = ( pixels * frames ) / time

PRINT " FRAMES/S:", fps
PRINT " PIXELS/S:", pxs

