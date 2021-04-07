Decompressor routines for Exomizer 2 & 3 by Magnus Lind

https://bitbucket.org/magli143/exomizer

Exomizer 2 routines are deexo****.asm files (4 variants):
-simple: no literal sequences and exo_mapbasebits aligned to a 256 boundary.
-b: backwards.

Exomizer 3 routines are deexoopt_XY.asm (10 variants):
-b for backwards, f for forwards.
-0..4 is the speed of the algorithm. 4 is without literal sequences and
mapbase aligned to a 256 boundary.
-deexoopt.asm is the unification of 8 variants (speed 4 not included).

The original decompressor was written by Jaime Tejedor Gomez (Metalbrain).
The assembler used is SjAsmPlus. For use with others assemblers maybe you
must adapt the conditional assembly directives.
