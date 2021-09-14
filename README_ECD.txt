1. Build dps8

    make clean
    make TESTING=1 HDBG=1 NO_LOCKLESS=1

2. Create a root disk with an ECD.

    cd src/dps8
    ./dps8 make_ecd_root_dsk.ini

3. Observe a ECD crash

    ./dps8 ecd.ini


