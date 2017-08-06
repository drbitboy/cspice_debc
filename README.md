# cspice\_debc

CSPICE source code DE-Bounds-Check


cspice\_debc.c

  Remove bounds checking from one F2Ced CSPICE \*[^\_]?.c routine

Usage (BASH):

  ./cspice\_debc src/cspice/dskx02.c ./dskx02\_debc.c


\#\#\# CHDIR to CSPICE top level directory

cd [.../]cspice

\#\#\# Move original CSPICE source (src/) to base\_src/

mv src base\_src

\#\#\# Duplicate original src/ sub-directory

rsync -a[v] base\_src/ src/ --exclude='\*\_c.c'

\#\#\# DE-Bounds-Check CSPICE source from base\_src/cspice/ to src/cspice/

for C in base\_src/cspice/\*[^\_]?.c ; do

  ./cspice\_debc "${C} | ./cspice\_debc | ./cspice\_debc - ${C\#base\_}

done

\#\#\# Rebuild CSPICE libraries

./makeall.csh

