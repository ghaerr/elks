10 i = pi
20 print i,i
30 i = rnd
40 print i
50 i = 1
60 print abs(i)
70 i = -1
80 print abs(i)
90 i = 0
100 print abs(i)
110 print
120 print "INT(1.000000001) = ";int(1.000000001)
200 pio2 = PI/2
300 for i = 0 to 2*PI step pio2
400 print "cos(";i;") = ";cos(i)
500 next i
600 rem print "pow(3,4) = ";pow(3,4)
700 print "exp(1) = ";exp(1)
710 print chr$(65);"=A"
720 print code(" ");"=32"
780 rem test negative 0 return on macOS
800 j = cos(pio2)
900 if j < 0 then print "ERROR: cos(pi/2) < 0",j
