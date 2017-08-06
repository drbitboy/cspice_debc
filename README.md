# cspice\_debc

CSPICE source code DE-Bounds-Check

Remove bounds checking from one F2Ced CSPICE \*[^\_]?.c routine

- saves about 5% in runtime speed

Usage (BASH):

    ./cspice_debc src/cspice/dskx02.c ./dskx02_debc.c


    ### CHDIR to CSPICE top level directory
    cd [.../]cspice

    ### Move original CSPICE source (src/) to base_src/
    mv src base_src

    ### Duplicate original src/ sub-directory
    rsync -a[v] base_src/ src/ --exclude='*_c.c'

    ### DE-Bounds-Check CSPICE source from base_src/cspice/ to src/cspice/
    for C in base_src/cspice/*[^_]?.c ; do
        ./cspice_debc "${C} | ./cspice_debc | ./cspice_debc - ${C#base_}
    done

    ### Rebuild CSPICE libraries
    ./makeall.csh

