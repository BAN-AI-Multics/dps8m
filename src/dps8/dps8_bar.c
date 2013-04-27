/**
 * \file dps8_bar.c
 * \project dps8
 * \date 10/18/12
 * \copyright Copyright (c) 2012 Harry Reed. All rights reserved.
*/


/*
 * stuff to handle BAR mode ...
 */
// XXX needs testing ....

#include <stdio.h>

#include "dps8.h"

/*
 * The Base Address Register provides automatic hardware Address relocation and Address range limitation when the processor is in BAR mode.
 BAR.BASE: Contains the 9 high-order bits of an 18-bit address relocation constant. The low-order bits are generated as zeros.
 BAR.BOUND: Contains the 9 high-order bits of the unrelocated address limit. The low- order bits are generated as zeros. An attempt to access main memory beyond this limit causes a store fault, out of bounds. A value of 0 is truly 0, indicating a null memory range.
 
 In BAR mode, the base address register (BAR) is used. The BAR contains an address bound and a base address. All computed addresses are relocated by adding the base address. The relocated address is combined with the procedure pointer register to form the virtual memory address. A program is kept within certain limits by subtracting the unrelocated computed address from the address bound. If the result is zero or negative, the relocated address is out of range, and a store fault occurs.
 */

word18 getBARaddress(word18 addr)
{
    if (BAR.BOUND == 0)
        // store fault, out of bounds.
        ;
    if ((addr & 0777000) >= (BAR.BOUND << 9))
        // store fault, out of bounds.
        ;
    
    word18 barAddr = (addr + (BAR.BASE << 9)) & 0777777;
    finalAddress = barAddr;
    return barAddr;
    //return (addr + (BAR.BASE << 9)) & 0777777;
}
