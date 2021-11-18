%{

#include <string.h>
#include "ugbc.tab.h" /* The tokens */
%}

%option yylineno
      
%%

"#["[a-fA-F0-9]+"]" { yylval.string = strdup(yytext); return(BufferDefinition); }
"#["[a-fA-F0-9]+ { yylval.string = strdup(yytext); return(BufferDefinition); }

_[\n\r]+
[\n\r]+ { return(NewLine);}
";" { return(OP_SEMICOLON); }
":" { return(OP_COLON); }
"(" { return(OP); }
")" { return(CP); }
"," { return(OP_COMMA); }
"=" { return(OP_ASSIGN); }
"==" { return(OP_EQUAL); }
":=" { return(OP_ASSIGN_DIRECT); }
"+" { return(OP_PLUS); }
"-" { return(OP_MINUS); }
"++" { return(OP_INC); }
"--" { return(OP_DEC); }
"#" { return(OP_HASH); }
"<" { return(OP_LT); }
"<=" { return(OP_LTE); }
">" { return(OP_GT); }
">=" { return(OP_GTE); }
"<>" { return(OP_DISEQUAL); }
"*" { return(OP_MULTIPLICATION); }
"**" { return(OP_MULTIPLICATION2); }
"$" { return(OP_DOLLAR); }
"^" { return(OP_POW); }
"/" { return(OP_DIVISION); }
"\\" { return(OP_DIVISION2); }
"[" { return(OSP); }
"]" { return(CSP); }
"{" { return(OGP); }
"}" { return(CGP); }
"?" { return(QM); }

8BIT { return (BYTE); }
16BIT { return (WORD); }
32BIT { return (DWORD); }

A { return (A); }
ABS { return (ABS); }
ALT { return (ALT); }
ARRAY { return (ARRAY); }
AS { return (AS); }
ASC { return (ASC); }
ASTERISK { return (ASTERISK); }
AT { return (AT); }
ATARI { return (ATARI); }
ATARIXL { return (ATARIXL); }
ADD { return (ADD); }
ADDRESS { return (ADDRESS); }
AND { return (AND); }
ARROW { return (ARROW); }
B { return (B); }
BACK { return (BACK); }
BACKGROUND { return (BACKGROUND); }
BANK { return (BANK); }
BEGIN { return (BEG); }
BIN\$ { return (BIN); }
BIT { return (BIT); }
BOTH { return (BOTH); }
BOX { return (BOX); }
BITMAP { return (BITMAP); }
BLACK { return (BLACK); }
BLUE { return (BLUE); }
BROWN { return(BROWN); }
BYTE { return (BYTE); }
BORDER { return (BORDER); }
BUFFER { return (BUFFER); }
C { return (C); }
C64 { return (C64); }
CALL { return (CALL); }
CAN { return (CAN); }
CAPS { return (CAPS); }
CAPSLOCK { return (CAPSLOCK); }
CASE { return (CASE); }
CDOWN { return (CDOWN); }
CENTER { return (CENTER); }
CENTRE { return (CENTRE); }
CHR { return (CHR); }
CHR\$ { return (CHR); }
CIRCLE { return (CIRCLE); }
CLEAR { return (CLEAR); }
CLEFT { return (CLEFT); }
CLINE { return (CLINE); }
CLIP { return (CLIP); }
CLS { return (CLS); }
CMOVE { return (CMOVE); }
CODE { return (CODE); }
COLLISION { return (COLLISION); }
COLON { return (COLON); }
COLOR { return (COLOR); }
COLORS { return (COLORS); }
COLORMAP { return (COLORMAP); }
COMMA { return (COMMA); }
COMMODORE { return (COMMODORE); }
COMPRESS { return (COMPRESS); }
CONST { return (CONST); }
CONTROL { return (CONTROL); }
COUNT { return (COUNT); }
CRIGHT { return (CRIGHT); }
CRSR { return (CRSR); }
CUP { return (CUP); }
CURSOR { return (CURSOR); }
CYAN { return(CYAN); }
CYCLES { return (CYCLES); }
D { return (D); }
DARK { return(DARK); }
DATA { return (DATA); }
DEBUG { return (DEBUG); }
DEC { return (OP_DEC); }
DEFAULT { return (DEFAULT); }
DEFINE { return (DEFINE); }
DELETE { return (DELETE); }
DIM { return (DIM); }
DISABLE { return (DISABLE); }
DISTANCE { return (DISTANCE); }
DONE { return (DONE); }
DO { return (DO); }
DOWN { return (DOWN); }
DRAW { return (DRAW); }
DRAGON { return (DRAGON); }
DRAGON32 { return (DRAGON32); }
DRAGON64 { return (DRAGON64); }
DWORD { return (DWORD); }
E { return (E); }
ECM { return(ECM); }
ELLIPSE { return(ELLIPSE); }
ELSE { return(ELSE); }
ELSEIF { return(ELSEIF); }
EMPTY { return(EMPTY); }
EMPTYTILE { return(EMPTYTILE); }
END { return (END); }
ENDIF { return (ENDIF); }
ENDSELECT { return (ENDSELECT); }
ENABLE { return (ENABLE); }
EQUAL { return (EQUAL); }
EXIT { return (EXIT); }
EXPAND { return (EXPAND); }
EVERY { return (EVERY); }
FALSE { return(FALSE); }
F { return (F); }
F1 { return (F1); }
F2 { return (F2); }
F3 { return (F3); }
F4 { return (F4); }
F5 { return (F5); }
F6 { return (F6); }
F7 { return (F7); }
F8 { return (F8); }
FILL { return (FILL); }
FIRE { return(FIRE); }
FLIP { return(FLIP); }
FLIP\$ { return(FLIP); }
FONT { return(FONT); }
FOR { return(FOR); }
FRAME { return (FRAME); }
FRAMES { return (FRAMES); }
FREE { return (FREE); }
FROM { return (FROM); }
FUNCTION { return (FUNCTION); }
G { return (G); }
GAMELOOP { return (GAMELOOP); }
GET { return (GET); }
GLOBAL { return (GLOBAL); }
GOLD { return(GOLD); }
GOTO { return (GOTO); }
GOSUB { return (GOSUB); }
GR { return (GR); }
GRAPHIC { return (GRAPHIC); }
GRAY { return(GRAY); }
GREEN { return(GREEN); }
GREY { return(GREY); }
H { return (H); }
HALT { return (HALT); }
HAS { return (HAS); }
HEIGHT { return (HEIGHT); }
HIDE { return (HIDE); }
HIDDEN { return (HIDDEN); }
HIT { return (HIT); }
HOME { return (HOME); }
HORIZONTAL { return (HORIZONTAL); }
HSCROLL { return (HSCROLL); }
I { return (I); }
IF { return (IF); }
IGNORE { return (IGNORE); }
IMAGE { return (IMAGE); }
IMAGES { return (IMAGES); }
IN { return (IN); }
INC { return (OP_INC); }
INK { return (INK); }
INKEY { return (INKEY); }
INKEY\$ { return (INKEY); }
INPUT { return (INPUT); }
INSERT { return (INSERT); }
INVERSE { return (INVERSE); }
INSTR { return (INSTR); }
IS { return (IS); }
J { return (J); }
JDOWN { return (JDOWN); }
JFIRE { return (JFIRE); }
JLEFT { return (JLEFT); }
JRIGHT { return (JRIGHT); }
JUP { return (JUP); }
JOY { return (JOY); }
JOYCOUNT { return (JOYCOUNT); }
K { return (K); }
KEY { return (KEY); }
KEYSHIFT { return (KEYSHIFT); }
KEYSTATE { return (KEYSTATE); }
L { return (L); }
LAVENDER { return(LAVENDER); }
LEFT { return (LEFT); }
LEFT\$ { return (LEFT); }
LEN { return (LEN); }
LETTER { return (LETTER); }
LIGHT { return(LIGHT); }
LINE { return(LINE); }
LOAD { return(LOAD); }
LOCATE { return(LOCATE); }
LOCK { return (LOCK); }
LOOP { return (LOOP); }
LOWER { return (LOWER); }
LOWER\$ { return (LOWER); }
M { return (M); }
MAGENTA { return(MAGENTA); }
MASKED { return(MASKED); }
MAX { return(MAX); }
MCM { return(MCM); }
MEMORIZE { return(MEMORIZE); }
MID { return(MID); }
MID\$ { return(MID); }
MIN { return(MIN); }
MINUS { return(MINUS); }
MOB { return(MOB); }
MOD { return(MOD); }
MONOCOLOR { return(MONOCOLOR); }
MS { return (MILLISECOND); }
MILLISECOND { return (MILLISECOND); }
MILLISECONDS { return (MILLISECONDS); }
MULTICOLOR { return(MULTICOLOR); }
N { return (N); }
NEXT { return (NEXT); }
NEW { return (NEW); }
NORMAL { return(NORMAL); }
NONE { return(NONE); }
NOP { return(NOP); }
NOT { return(NOT); }
O { return (O); }
OF { return (OF); }
OFF { return (OFF); }
OLIVE { return(OLIVE); }
ON { return (ON); }
ONLY { return (ONLY); }
OR { return (OR); }
ORANGE { return(ORANGE); }
OVERLAYED { return(OVERLAYED); }
P { return (P); }
PAPER { return(PAPER); }
PARALLEL { return(PARALLEL); }
PARAM { return(PARAM); }
PEACH { return(PEACH); }
PEEK { return (PEEK); }
PEN { return (PEN); }
PERIOD { return (PERIOD); }
PINK { return(PINK); }
PLOT { return(PLOT); }
PLUS { return(PLUS); }
PLUS4 { return (PLUS4); }
POINT { return (POINT); }
POKE { return (POKE); }
POLYLINE { return (POLYLINE); }
POSITIVE { return (POSITIVE); }
POUND { return (POUND); }
POP { return (POP); }
POSITION { return (POSITION); }
POW { return (POWERING); }
PRINT { return (PRINT); }
PROC { return (PROC); }
PROCEDURE { return (PROCEDURE); }
PURPLE { return(PURPLE); }
PUT { return(PUT); }
Q { return (Q); }
R { return (R); }
RANDOM { return (RANDOM); }
RANDOMIZE { return (RANDOMIZE); }
RASTER { return (RASTER); }
RED { return(RED); }
REMEMBER { return(REMEMBER); }
RENDER { return(RENDER); }
REPEAT { return (REPEAT); }
REPLACE { return (REPLACE); }
RETURN { return (RETURN); }
RIGHT { return (RIGHT); }
RIGHT\$ { return (RIGHT); }
RND { return (RND); }
ROLL { return (ROLL); }
ROWS { return (ROWS); }
RUNSTOP { return (RUNSTOP); }
RUN { return (RUN); }
S { return (S); }
SCAN { return (SCAN); }
SCANCODE { return (SCANCODE); }
SCANSHIFT { return (SCANSHIFT); }
SCREEN { return (SCREEN); }
SCROLL { return (SCROLL); }
SELECT { return (SELECT); }
SEMICOLON { return (SEMICOLON); }
SET { return (SET); }
SGN { return (SGN); }
SHARED { return (SHARED); }
SHIFT { return (SHIFT); }
SHIFTS { return (SHIFTS); }
SHOW { return (SHOW); }
SIGNED { return (SIGNED); }
SIZE { return (SIZE); }
SLASH { return (SLASH); }
SPACE { return (SPACE); }
SPAWN { return (SPAWN); }
SPRITE { return (SPRITE); }
SQR { return (SQR); }
STATE { return (STATE); }
STEP { return (STEP); }
STOP { return (STOP); }
STR { return (STR); }
STR\$ { return (STR); }
STRING { return (STRING); }
STRING\$ { return (STRING); }
T { return (T); }
TAB { return(TAB); }
TAN { return(TAN); }
TEMPORARY { return (TEMPORARY); }
TEXT { return (TEXT); }
TEXTMAP { return (TEXTMAP); }
TEXTADDRESS { return (TEXTADDRESS); }
THEN { return (THEN); }
THREAD { return (THREAD); }
TICKS { return (TICKS); }
TILEMAP { return (TILEMAP); }
TILE { return (TILE); }
TILES { return (TILES); }
TI { return (TI); }
TIMER { return (TIMER); }
TO { return (TO); }
TURQUOISE { return(TURQUOISE); }
TRANSPARENCY { return(TRANSPARENCY); }
TRUE { return(TRUE); }
U { return (U); }
UNTIL { return (UNTIL); }
UP { return (UP); }
UPPER { return (UPPER); }
UPPER\$ { return (UPPER); }
USING { return (USING); }
V { return (V); }
VAL { return (VAL); }
VAR { return (VAR); }
VBL { return (VBL); }
VIC20 { return (VIC20); }
VARIABLES { return (VARIABLES); }
VERTICAL { return (VERTICAL); }
VIOLET { return(VIOLET); }
VISIBLE { return(VISIBLE); }
VSCROLL { return (VSCROLL); }
W { return (W); }
WAIT { return (WAIT); }
WEND { return (WEND); }
WITH { return (WITH); }
WIDTH { return (WIDTH); }
WHILE { return (WHILE); }
WHITE { return(WHITE); }
WRITING { return(WRITING); }
WORD { return (WORD); }
X { return (X); }
XY { return (XY); }
XOR { return (XOR); }
XCURS { return (XCURS); }
XPEN { return (XPEN); }
Y { return (Y); }
YX { return (YX); }
YCURS { return (YCURS); }
YELLOW { return(YELLOW); }
YIELD { return(YIELD); }
YPEN { return (YPEN); }
Z { return (Z); }
ZX { return (ZX); }

"REM"[^\n\r]* { return(Remark);  }
"' "[^\n\r]* { return(Remark);  }

[A-Za-z][A-Za-z0-9\_]* { yylval.string = strdup(yytext); return(Identifier);  }
\"(\\.|[^"\\])*\" { yylval.string = strdup(yytext); memcpy(yylval.string,yylval.string+1,strlen(yylval.string)); yylval.string[strlen(yylval.string)-1]=0; return(String);  }
\$[a-fA-F0-9]+ { yylval.integer = strtol(yytext+1,0,16); return(Integer); }
&H[a-fA-F0-9]+ { yylval.integer = strtol(yytext+2,0,16); return(Integer); }
0x[a-fA-F0-9]+ { yylval.integer = strtol(yytext+2,0,16); return(Integer); }
%[0-1]+ { yylval.integer = strtol(yytext+1,0,2); return(Integer); }
\s[-][0-9]+ { yylval.integer = atoi(yytext); return(Integer);  }
[0-9]+ { yylval.integer = atoi(yytext); return(Integer);  }

[ \t]+ 
. { return(yytext[0]); }

%%
