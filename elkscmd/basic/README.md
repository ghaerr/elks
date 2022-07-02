ELKS Basic
==========
From Robin Edwards' ArduinoBASIC (https://github.com/robinhedwards/ArduinoBASIC).

A complete BASIC interpreter for your 80's home computer! This BASIC supports almost all the usual features, with float and string variables, multi-dimensional arrays, FOR-NEXT, GOSUB-RETURN, etc.

This interpreter uses some of the commands from Sinclair the original BASIC, but string functions are the newer RIGHT$/LEFT$ variety, rather than A$(2 TO 5) Sinclair-style slicing.

BASIC Language
--------------
Variables names can be up to 8 alphanumeric characters but start with a letter e.g. a, bob32
String variable names must end in $ e.g. a$, bob32$
Case is ignored (for all identifiers). BOB32 is the same as Bob32. print is the same as PRINT

Array variables are independent from normal variables. So you can use both:
```
LET a = 5
DIM a(10)
```
There is no ambiguity, since a on its own refers to the simple variable 'a', and a(n) referes to an element of the 'a' array. For string arrays, this is incompatible with some older verisons of BASIC where ordinary string variable's lengths needed preallocation, e.g. DIM A$(80) for an 80 character string. Here, all string variable values are managed dynamically.

```
Arithmetic operators: + - * / MOD
Comparisons: <> (not equals) > >= < <=
Logical: AND, OR, NOT
```

Expressions can be numerical e.g. 5*(3+2), or string "Hello "+"world".
Only the addition operator is supported on strings (plus the functions below).

Commands
```
PRINT <expr>;,<expr>... e.g. PRINT "A=";a
LET variable = <expr> e.g. LET A$="Hello"
variable = <expr> e.g. A=5
GOTO lineNumber
REM <comment> e.g. REM ** My Program ***
STOP
CONT (continue from a STOP)
INPUT [string prompt,] variable e.g. INPUT a$ or INPUT a(5,3)
IF <expr> THEN cmd e.g. IF a>10 THEN a = 0: GOTO 20
FOR variable = start TO end STEP step
NEXT variable
GOSUB lineNumber
RETURN
DIM variable(n1,n2...)
READ var
DATA NumberOrString [,NumberOrString...]
RESTORE [lineNumber]
CLS
PAUSE milliseconds
POSITION x,y sets the cursor
NEW
LIST [start],[end] e.g. LIST or LIST 10,100
RUN [lineNumber]
LOAD "filename" loads filename.bas
SAVE "filename" saves filename.bas
SAVE+ "filename" saves filename.bas, sets auto-run on load
DELETE "filename" deletes filename.bas
DIR
MODE number (set graphics mode, PC-98 only for now, e.g MODE 1 to use PLOT/DRAW/CIRCLE)
COLOR fg,bg (PC-98 only for now)
PLOT x,y (PC-98 only for now)
DRAW x,y (PC-98 only for now)
CIRCLE x,y,r (PC-98 only for now)
INPB(port) (IO read byte from `port`)
INPW(port) (IO read word from `port`)
OUTB port, value (IO write byte `value` from `port`)
OUTW port, value (IO write word `value` from `port`)
POKE offset,segment,value

Architecture-specific
PIN pinNum, value (0 = low, non-zero = high)
PINMODE pinNum, mode - not implemented

Not yet implemented
RANDOMIZE [nmber]
```

"Pseudo-identifiers"
```
INKEY$ - returns (and eats) the last key pressed buffer (non-blocking). e.g. PRINT INKEY$
RND - random number betweeen 0 and 1. e.g. LET a = RND
PI - 3.1415926
```

Functions
```
LEN(string) e.g. PRINT LEN("Hello") -> 5
VAL(string) e.g. PRINT VAL("1+2") runtime evaluated
INT(number) e.g. INT(1.5)-> 1
ABS(number) e.g. ABS(-1)-> 1
CHR$(number) e.g. CHR$(32) -> " "
CODE(string) e.g. CODE(" ") -> 32
STR$(number) e.g. STR$(2) -> "2"
LEFT$(string,n)
RIGHT$(string,n)
MID$(string,start,n)
COS(x) cosine
SIN(x) sine
TAN(x) tangent
ACS(x) arc cosine
ASN(x) arc sine
ATN(x) arc tangent
EXP(x) e exponential
LN(x) natural logarithm
POW(x,y) e.g. POW(2,0.5) -> 1.414 square root of 2
HEX$(number) e.g. HEX$(25923) -> "6543"
PEEK(offset,segment)

Architecture-specific
PINREAD(pin)
ANALOGRD(pin) - not implemented

Not implemented
x^y x to the y power - use POW(x,y)
SQR(number) square root, use POW(number,0.5)
SGN(number) sign, use IF number < 0 etc

Not yet implemented
SCREEN$(line,col)
ATTR(line,col)
POINT(x,y)
```
