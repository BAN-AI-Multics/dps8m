/**
 * \file dps8_eis.c
 * \project dps8
 * \date 12/31/12
 * \copyright Copyright (c) 2012 Harry Reed. All rights reserved.
 * \brief EIS support code...
*/

#include <stdio.h>

#include "dps8.h"

#ifdef TESTER
#ifdef rIC
#undef rIC
extern word18 rIC;
#endif
#endif


static void EISWrite(EISaddr *p, word36 data)
{
    if (p->mat == viaPR && get_addr_mode() == APPEND_mode)
    {
        TPR.TRR = p->RNR;
        TPR.TSR = p->SNR;
        Write(p->e->ins, p->address, data, DataWrite, 0); // write data
    } else
        Write(p->e->ins, p->address, data, OperandWrite, 0); // write data
}

static word36 EISRead(EISaddr *p)
{
    word36 data;
    if (p->mat == viaPR && get_addr_mode() == APPEND_mode)
    {
        TPR.TRR = p->RNR;
        TPR.TSR = p->SNR;
        Read(p->e->ins, p->address, &data, DataRead, 0);     // read data via AR/PR. TPR.{TRR,TSR} already set up
    }
    else
        Read(p->e->ins, p->address, &data, OperandRead, 0);  // read operand
    
    return data;
}

static void EISReadN(EISaddr *p, int N, word36 *dst)
{
    for(int n = 0 ; n < N ; n++)
    {
        *dst++ = EISRead(p);
        p->address += 1;
        p->address &= AMASK;
    }
}



PRIVATE
word36 getMFReg(int n, bool RType)
{
    switch (n)
    {
        case 0: ///< n
            return 0;
        case 1: ///< au
            return GETHI(rA);
        case 2: ///< qu
            return GETHI(rQ);
        case 3: ///< du
            // XXX: IPR generate Illegal Procedure Fault
            return 0;
        case 4: ///< ic - The ic modifier is permitted in MFk.REG and C (od)32,35 only if MFk.RL = 0, that is, if the contents of the register is an address offset, not the designation of a register containing the operand length.
            return rIC;
        case 5: ///< al / a
            return RType ? GETLO(rA) : rA;
        case 6: ///< ql / a
            return RType ? GETLO(rQ) : rQ;
        case 7: ///< dl
            // XXX: IPR generate Illegal Procedure Fault
            return 0;
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            return rX[n - 8];
    }
    fprintf(stderr, "getMFReg(): How'd we get here? n=%d\n", n);
    return 0;
}



/*!
 5.2.10.5  Operand Descriptor Address Preparation Flowchart
    A flowchart of the operations involved in operand descriptor address preparation is shown in Figure 5-2. The chart depicts the address preparation for operand descriptor 1 of a multiword instruction as described by modification field 1 (MF1). A similar type address preparation would be carried out for each operand descriptor as specified by its MF code.
    (Bull Nova 9000 pg 5-40  67 A2 RJ78 REV02)
 1. The multiword instruction is obtained from memory.
 2. The indirect (ID) bit of MF1 is queried to determine if the descriptor for operand 1 is present or is an indirect word.
 3. This step is reached only if an indirect word was in the operand descriptor location. Address modification for the indirect word is now performed. If the AR bit of the indirect word is 1, address register modification step 4 is performed.
 4. The y field of the indirect word is added to the contents of the specified address register.
 5. A check is now made to determine if the REG field of the indirect word specifies that a register type modification be performed.
 6. The indirect address as modified by the address register is now modified by the contents of the specified register, producing the effective address of the operand descriptor.
 7. The operand descriptor is obtained from the location determined by the generated effective address in item 6.
 8. Modification of the operand descriptor address begins. This step is reached directly from 2 if no indirection is involved. The AR bit of MF1 is checked to determine if address register modification is specified.
 9. Address register modification is performed on the operand descriptor as described under "Address Modification with Address Registers" above. The character and bit positions of the specified address register are used in one of two ways, depending on the type of operand descriptor, i.e., whether the type is a bit string, a numeric, or an alphanumeric descriptor.
 10. The REG field of MF1 is checked for a legal code. If DU is specified in the REG field of MF2 in one of the four multiword instructions (SCD, SCDR, SCM, or SCMR) for which DU is legal, the CN field is ignored and the character or characters are arranged within the 18 bits of the word address portion of the operand descriptor.
 11. The count contained in the register specified by the REG field code is appropriately converted and added to the operand address.
 12. The operand is retrieved from the calculated effective address location.
 */
// prepare MFk operand descriptor for use by EIS instruction ....

void setupOperandDescriptor(int k, EISstruct *e)
{
    switch (k)
    {
        case 1:
            e->MF1 = (int)bitfieldExtract36(e->op0,  0, 7);  ///< Modification field for operand descriptor 1
            break;
        case 2:
            e->MF2 = (int)bitfieldExtract36(e->op0, 18, 7);  ///< Modification field for operand descriptor 2
            break;
        case 3:
            e->MF3 = (int)bitfieldExtract36(e->op0, 27, 7);  ///< Modification field for operand descriptor 3
            break;
    }
    
    word18 MFk = e->MF[k-1];
    
    if (MFk & MFkID)
    {
        word36 opDesc = e->op[k-1];
        
        //! fill operand according to MFk....
        word18 address = GETHI(opDesc);
        e->addr[k-1].address = address;
        
        //!Indirect descriptor control. If ID = 1 for Mfk, then the kth word following the instruction word is an indirect pointer to the operand descriptor for the kth operand; otherwise, that word is the operand descriptor.
        //If MFk.ID = 1, then the kth word following an EIS multiword instruction word is not an operand descriptor, but is an indirect pointer to an operand descriptor and is interpreted as shown in Figure 4-5.
        
        
        // Mike Mondy michael.mondy@coffeebird.net sez' ...
        // EIS indirect pointers to operand descriptors use PR registers.
        // However, operand descriptors use AR registers according to the
        // description of the AR registers and the description of EIS operand
        // descriptors. However, the description of the MF field
        // claims that operands use PR registers. The AR doesn't have a
        // segment field. Emulation confirms that operand descriptors
        // need to be fetched via segments given in PR registers.

        bool a = opDesc & (1 << 6); 
        
        if (a)
        {
            // A 3-bit pointer register number (n) and a 15-bit offset relative to C(PRn.WORDNO) if A = 1 (all modes)
            int n = (int)bitfieldExtract36(address, 15, 3);
            int offset = address & 077777;  // 15-bit signed number
            address = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;

            e->addr[k-1].address = address;
            if (get_addr_mode() == APPEND_mode)
            {
                e->addr[k-1].SNR = PR[n].SNR;
                e->addr[k-1].RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
                
                e->addr[k-1].mat = viaPR;   // ARs involved
            }
        } else
            e->addr[k-1].mat = IndirectRead;      // no ARs involved yet

        
        // Address modifier for ADDRESS. All register modifiers except du and dl may be used. If the ic modifier is used, then ADDRESS is an 18-bit offset relative to value of the instruction counter for the instruction word. C(REG) is always interpreted as a word offset. REG 
        int reg = opDesc & 017;
        address += getMFReg(reg, true);
        address &= 0777777;
        
        e->addr[k-1].address = address;
        
        //Read(e->ins, address, &e->op[k-1], OperandRead, 0);  // read operand
        e->op[k-1] = EISRead(&e->addr[k-1]);  // read EIS operand .. this should be an indirectread
        e->addr[k-1].mat = OperandRead; 
    }
}

PRIVATE
void parseAlphanumericOperandDescriptor(int k, EISstruct *e)
{
    word18 MFk = e->MF[k-1];
    
    word36 opDesc = e->op[k-1];
    
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    
    word18 address = GETHI(opDesc);
    
    if (MFk & MFkAR)
    {
        // if MKf contains ar then it Means Y-charn is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(address, 15, 3);
        int offset = address & 077777;  // 15-bit signed number
        address = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;
        
        if (get_addr_mode() == APPEND_mode)
        {
            e->addr[k-1].SNR = PR[n].SNR;
            e->addr[k-1].RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);

            e->addr[k-1].mat = viaPR;   // ARs involved
        }
    }

    int CN = (int)bitfieldExtract36(opDesc, 15, 3);    ///< character number
    
    e->TA[k-1] = (int)bitfieldExtract36(opDesc, 13, 2);    // type alphanumeric
    
    if (MFk & MFkRL)
    {
        int reg = opDesc & 017;
        e->N[k-1] = (int32)getMFReg(reg, false);
        switch (e->TA[k-1])
        {
            case CTA4:
                e->N[k-1] &= 017777777; ///< 22-bits of length
                break;
            case CTA6:
            case CTA9:
                e->N[k-1] &= 07777777;  ///< 21-bits of length.
                break;
            default:
                fprintf(stderr, "parseAlphanumericOperandDescriptor(ta=%d) How'd we get here 1?\n", e->TA[k-1]);
                break;
        }
    }
    else
        e->N[k-1] = opDesc & 07777;
    
    word18 r = (word18)getMFReg(MFk & 017, true);
    
    if (!(MFk & MFkRL) && (MFk & 017) == 4)   // reg == IC ?
    {
        //The ic modifier is permitted in MFk.REG and C (od)32,35 only if MFk.RL = 0, that is, if the contents of the register is an address offset, not the designation of a register containing the operand length.
        address += r;
        r = 0;
    }

    e->effBITNO = 0;
    e->effCHAR = 0;
  
    // If seems that the effect address calcs given in AL39 p.6-27 are not quite right.
    // E.g. For CTA4/CTN4 because of the 4 "slop" bits you need to do 32-bit calcs not 36-bit!
    switch (e->TA[k-1])
    {
        case CTA4:
            e->effBITNO = 4 * (ARn_CHAR + 2*r + ARn_BITNO/4) % 2 + 1;   // XXX Check
            e->effCHAR = ((4*CN + 9*ARn_CHAR + 4*r + ARn_BITNO) % 32) / 4;  //9;36) / 4;  //9;
            e->effWORDNO = address + (4*CN + 9*ARn_CHAR + 4*r + ARn_BITNO) / 32;    // 36
            e->effWORDNO &= AMASK;
            
            e->YChar4[k-1] = e->effWORDNO;
            //e->CN[k-1] = CN;    //e->effCHAR;
            e->CN[k-1] = e->effCHAR;
            break;
        case CTA6:
            e->effBITNO = (9*ARn_CHAR + 6*r + ARn_BITNO) % 9;
            e->effCHAR = ((6*CN + 9*ARn_CHAR + 6*r + ARn_BITNO) % 36) / 6;//9;
            e->effWORDNO = address + (6*CN + 9*ARn_CHAR + 6*r + ARn_BITNO) / 36;
            e->effWORDNO &= AMASK;
            
            e->YChar6[k-1] = e->effWORDNO;
            e->CN[k-1] = e->effCHAR;   // ??????
            break;
        case CTA9:
            CN = (CN >> 1) & 07;  // XXX Do error checking
            
            e->effBITNO = 0;
            e->effCHAR = (CN + ARn_CHAR + r) % 4;
            e->effWORDNO = address + ((9*CN + 9*ARn_CHAR + 9*r + ARn_BITNO) / 36);
            e->effWORDNO &= AMASK;
            
            e->YChar9[k-1] = e->effWORDNO;
            e->CN[k-1] = e->effCHAR;   // ??????
            break;
        default:
            fprintf(stderr, "parseAlphanumericOperandDescriptor(ta=%d) How'd we get here 2?\n", e->TA[k-1]);
            break;
    }
    
    EISaddr *a = &e->addr[k-1];
    a->address = e->effWORDNO;
    a->cPos= e->effCHAR;
    a->bPos = e->effBITNO;
    
    a->_type = eisTA;
    a->TA = e->TA[k-1];

}

void parseNumericOperandDescriptor(int k, EISstruct *e)
{
    word18 MFk = e->MF[k-1];
    
    word36 opDesc = e->op[k-1];
    
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    
    word18 address = GETHI(opDesc);
    if (MFk & MFkAR)
    {
        // if MKf contains ar then it Means Y-charn is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(address, 15, 3);
        int offset = address & 077777;  ///< 15-bit signed number
        address = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;
        
        if (get_addr_mode() == APPEND_mode)
        {
            e->addr[k-1].SNR = PR[n].SNR;
            e->addr[k-1].RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            
            e->addr[k-1].mat = viaPR;   // ARs involved
        }
    }
    
    word8 CN = (word8)bitfieldExtract36(opDesc, 15, 3);    ///< character number
    // XXX need to do some error checking here with CN
    
    e->TN[k-1] = (int)bitfieldExtract36(opDesc, 14, 1);    // type numeric
    e->S[k-1]  = (int)bitfieldExtract36(opDesc, 12, 2);    // Sign and decimal type of data
    e->SF[k-1] = (int)SIGNEXT6(bitfieldExtract36(opDesc, 6, 6));    // Scaling factor.
    
    // Operand length. If MFk.RL = 0, this field contains the operand length in digits. If MFk.RL = 1, it contains the REG code for the register holding the operand length and C(REG) is treated as a 0 modulo 64 number. See Table 4-1 and EIS modification fields (MF) above for a discussion of register codes.
    if (MFk & MFkRL)
    {
        int reg = opDesc & 017;
        e->N[k-1] = getMFReg(reg, true) & 077;
    }
    else
        e->N[k-1] = opDesc & 077;
    
    word36 r = getMFReg(MFk & 017, true);
    if (!(MFk & MFkRL) && (MFk & 017) == 4)   // reg == IC ?
    {
        //The ic modifier is permitted in MFk.REG and C (od)32,35 only if MFk.RL = 0, that is, if the contents of the register is an address offset, not the designation of a register containing the operand length.
        address += r;
        r = 0;
    }

    e->effBITNO = 0;
    e->effCHAR = 0;
    e->effWORDNO = 0;
    
    // If seems that the effect address calcs given in AL39 p.6-27 are not quite right.
    // E.g. For CTA4/CTN4 because of the 4 "slop" bits you need to do 32-bit calcs not 36-bit!

    switch (e->TN[k-1])
    {
        case CTN4:
            e->effBITNO = 4 * (ARn_CHAR + 2*r + ARn_BITNO/4) % 2 + 1; // XXX check
            e->effCHAR = ((4*CN + 9*ARn_CHAR + 4*r + ARn_BITNO) % 32) / 4;  //9; 36) / 4;  //9;
            e->effWORDNO = address + (4*CN + 9*ARn_CHAR + 4*r + ARn_BITNO) / 32;    //36;
            e->effWORDNO &= AMASK;
            
            e->CN[k-1] = e->effCHAR;        // ?????
            e->YChar4[k-1] = e->effWORDNO;
            
            break;
        case CTN9:
            CN = (CN >> 1) & 07;  // XXX Do error checking
            
            e->effBITNO = 0;
            e->effCHAR = (CN + ARn_CHAR + r) % 4;
            e->effWORDNO = address + (9*CN + 9*ARn_CHAR + 9*r + ARn_BITNO) / 36;
            e->effWORDNO &= AMASK;
            
            e->YChar9[k-1] = e->effWORDNO;
            e->CN[k-1] = e->effCHAR;        // ?????
            
            break;
        default:
            fprintf(stderr, "parseNumericOperandDescriptor(ta=%d) How'd we get here 2?\n", e->TA[k-1]);
            break;
    }
    
    EISaddr *a = &e->addr[k-1];
    a->address = e->effWORDNO;
    a->cPos = e->effCHAR;
    a->bPos = e->effBITNO;
    
    a->_type = eisTN;
    a->TN = e->TN[k-1];
}


PRIVATE
void parseBitstringOperandDescriptor(int k, EISstruct *e)
{
    word18 MFk = e->MF[k-1];
    
    word36 opDesc = e->op[k-1];
    
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    
    word18 address = GETHI(opDesc);
    if (MFk & MFkAR)
    {
        // if MKf contains ar then it Means Y-charn is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(address, 15, 3);
        int offset = address & 077777;  // 15-bit signed number
        address = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;
        
        if (get_addr_mode() == APPEND_mode)
        {
            e->addr[k-1].SNR = PR[n].SNR;
            e->addr[k-1].RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            
            e->addr[k-1].mat = viaPR;   // ARs involved
        }
    }
    
    //Operand length. If MFk.RL = 0, this field contains the string length of the operand. If MFk.RL = 1, this field contains the code for a register holding the operand string length. See Table 4-1 and EIS modification fields (MF) above for a discussion of register codes.
    if (MFk & MFkRL)
    {
        int reg = opDesc & 017;
        e->N[k-1] = getMFReg(reg, false) & 077777777;
    }
    else
        e->N[k-1] = opDesc & 07777;
    
    
    //e->B[k-1] = (int)bitfieldExtract36(opDesc, 12, 4) & 0xf;
    //e->C[k-1] = (int)bitfieldExtract36(opDesc, 16, 2) & 03;
    int B = (int)bitfieldExtract36(opDesc, 12, 4) & 0xf;    // bit# from descriptor
    int C = (int)bitfieldExtract36(opDesc, 16, 2) & 03;     // char# from descriptor
    
    word36 r = getMFReg(MFk & 017, true);
    if (!(MFk & MFkRL) && (MFk & 017) == 4)   // reg == IC ?
    {
        //The ic modifier is permitted in MFk.REG and C (od)32,35 only if MFk.RL = 0, that is, if the contents of the register is an address offset, not the designation of a register containing the operand length.
        address += r;
        r = 0;
    }

    //e->effBITNO = (9*ARn_CHAR + 36*r + ARn_BITNO) % 9;
    //e->effCHAR = ((9*ARn_CHAR + 36*r + ARn_BITNO) % 36) / 9;
    //e->effWORDNO = address + (9*ARn_CHAR + 36*r + ARn_BITNO) / 36;
    e->effBITNO = (9*ARn_CHAR + r + ARn_BITNO + B + 9*C) % 9;
    e->effCHAR = ((9*ARn_CHAR + r + ARn_BITNO + B + 9*C) % 36) / 9;
    e->effWORDNO = address + (9*ARn_CHAR + r + ARn_BITNO + B + 9*C) / 36;
    e->effWORDNO &= AMASK;
    
    e->B[k-1] = e->effBITNO;
    e->C[k-1] = e->effCHAR;
    e->YBit[k-1] = e->effWORDNO;
    
    EISaddr *a = &e->addr[k-1];
    a->address = e->effWORDNO;
    a->cPos = e->effCHAR;
    a->bPos = e->effBITNO;
    a->_type = eisBIT;
}


/*!
 * determine sign of N*9-bit length word
 */
PRIVATE
bool sign9n(word72 n128, int N)
{
    
    // sign bit of  9-bit is bit 8  (1 << 8)
    // sign bit of 18-bit is bit 17 (1 << 17)
    // .
    // .
    // .
    // sign bit of 72-bit is bit 71 (1 << 71)
    
    if (N < 1 || N > 8) // XXX largest int we'll play with is 72-bits? Makes sense
        return false;
    
    word72 sgnmask = (word72)1 << ((N * 9) - 1);
    
    return (bool)(sgnmask & n128);
}

/*!
 * sign extend a N*9 length word to a (word72) 128-bit word
 */
PRIVATE
word72 signExt9(word72 n128, int N)
{
    // ext mask for  9-bit = 037777777777777777777777777777777777777400  8 0's
    // ext mask for 18-bit = 037777777777777777777777777777777777400000 17 0's
    // ext mask for 36-bit = 037777777777777777777777777777400000000000 35 0's
    // etc...
    
    int bits = (N * 9) - 1;
    if (sign9n(n128, N))
    {
        uint128 extBits = ((uint128)-1 << bits);
        return n128 | extBits;
    }
    uint128 zeroBits = ~((uint128)-1 << bits);
    return n128 & zeroBits;
}

/**
 * get sign to buffer position p
 */
PRIVATE
int getSign(word72s n128, EISstruct *e)
{
    // 4- or 9-bit?
    if (e->TN2 == CTN4) // 4-bit
    {
        // If P=1, positive signed 4-bit results are stored using octal 13 as the plus sign.
        // If P=0, positive signed 4-bit results are stored with octal 14 as the plus sign.
        if (n128 >= 0)
        {
            if (e->P)
                return 013;  // alternate + sign
            else
                return 014;  // default + sign
        }
        else
        {
            SETF(e->_flags, I_NEG); 
            return 015;      // - sign
        }
    }
    else
    {   // 9-bit
        if (n128 >= 0)
            return 053;     // default 9-bit +
        else
        {
            SETF(e->_flags, I_NEG);
            return 055;     // default 9-bit -
        }
    }
}


/*!
 * add sign to buffer position p
 */
#ifndef QUIET_UNUSED
PRIVATE
void addSign(word72s n128, EISstruct *e)
{
    *(e->p++) = getSign(n128, e);
}
#endif

/*!
 * load a 9*n bit integer into e->x ...
 */
PRIVATE
//void load9x(int n, word18 sourceAddr, int pos, EISstruct *e)
void load9x(int n, EISaddr *addr, int pos, EISstruct *e)
{
    int128 x = 0;
    
    //word36 data;
    //Read(e->ins, sourceAddr, &data, OperandRead, 0);    // read data word from memory
    word36 data = EISRead(addr);
    
    int m = n;
    while (m)
    {
        x <<= 9;         // make room for next 9-bit byte
        
        if (pos > 3)        // overflows to next word?
        {   // yep....
            pos = 0;        // reset to 1st byte
            //sourceAddr = (sourceAddr + 1) & AMASK;          // bump source to next address
            //Read(e->ins, sourceAddr, &data, OperandRead, 0);    // read it from memory
            addr->address = (addr->address + 1) & AMASK;          // bump source to next address
            data = EISRead(addr);    // read it from memory
        }
        
        x |= GETBYTE(data, pos);   // fetch byte at position pos and 'or' it in
        
        pos += 1;           // onto next posotion
        
        m -= 1;             // decrement byte counter
    }
    e->x = signExt9(x, n);  // form proper 2's-complement integer
}

PRIVATE
word4 get4(word36 w, int pos)
{
    switch (pos)
    {
        case 0:
            return bitfieldExtract36(w, 31, 4);
            break;
        case 1:
            return bitfieldExtract36(w, 27, 4);
            break;
        case 2:
            return bitfieldExtract36(w, 22, 4);
            break;
        case 3:
            return bitfieldExtract36(w, 18, 4);
            break;
        case 4:
            return bitfieldExtract36(w, 13, 4);
            break;
        case 5:
            return bitfieldExtract36(w, 9, 4);
            break;
        case 6:
            return bitfieldExtract36(w, 4, 4);
            break;
        case 7:
            return bitfieldExtract36(w, 0, 4);
            break;
    }
    fprintf(stderr, "get4(): How'd we get here?\n");
    return 0;
}

PRIVATE
word4 get6(word36 w, int pos)
{
    switch (pos)
    {
        case 0:
            return bitfieldExtract36(w, 30, 6);
            break;
        case 1:
            return bitfieldExtract36(w, 24, 6);
            break;
        case 2:
            return bitfieldExtract36(w, 18, 6);
            break;
        case 3:
            return bitfieldExtract36(w, 12, 6);
            break;
        case 4:
            return bitfieldExtract36(w, 6, 6);
            break;
        case 5:
            return bitfieldExtract36(w, 0, 6);
            break;
    }
    fprintf(stderr, "get6(): How'd we get here?\n");
    return 0;
}

PRIVATE
word9 get9(word36 w, int pos)
{
    
    switch (pos)
    {
        case 0:
            return bitfieldExtract36(w, 27, 9);
            break;
        case 1:
            return bitfieldExtract36(w, 18, 9);
            break;
        case 2:
            return bitfieldExtract36(w, 9, 9);
            break;
        case 3:
            return bitfieldExtract36(w, 0, 9);
            break;
    }
    fprintf(stderr, "get9(): How'd we get here?\n");
    return 0;
}


/*!
 * return a 4- or 9-bit character at memory "*address" and position "*pos". Increment pos (and address if necesary)
 */
PRIVATE
int EISget49(EISaddr *p, int *pos, int tn)
{
    if (!p)
    //{
    //    p->lastAddress = -1;
    //    p->data = 0;
        return 0;
    //}
    
    int maxPos = tn == CTN4 ? 7 : 3;
    
    //if (p->lastAddress != p->address)                 // read from memory if different address
        p->data = EISRead(p);   // read data word from memory
    
    if (*pos > maxPos)        // overflows to next word?
    {   // yep....
        *pos = 0;        // reset to 1st byte
        p->address = (p->address + 1) & AMASK;          // bump source to next address
        p->data = EISRead(p);    // read it from memory
    }
    
    int c = 0;
    switch(tn)
    {
        case CTN4:
            c = (word4)get4(p->data, *pos);
            break;
        case CTN9:
            c = (word9)get9(p->data, *pos);
            break;
    }
    
    *pos += 1;
    //p->lastAddress = p->address;
    
    return c;
}

/*!
 * return a 4-, 6- or 9-bit character at memory "*address" and position "*pos". Increment pos (and address if necesary)
 * NB: must be initialized before use or else unpredictable side-effects may result. Not thread safe!
 */
static int EISget469(EISaddr *p, int *pos, int ta)
{
    if (!p)
    //{
    //    p->lastAddress = -1;
        return 0;
    //}
    
    int maxPos = 3; // CTA9
    switch(ta)
    {
        case CTA4:
            maxPos = 7;
            break;
            
        case CTA6:
            maxPos = 5;
            break;
    }
    
    //if (p->lastAddress != p->address)                 // read from memory if different address
    //    p->data = EISRead(p);   // read data word from memory
    
    if (*pos > maxPos)        // overflows to next word?
    {   // yep....
        *pos = 0;        // reset to 1st byte
        p->address = (p->address + 1) & AMASK;          // bump source to next address
        //p->data = EISRead(p);    // read it from memory
    }
    p->data = EISRead(p);    // read it from memory
    
    int c = 0;
    switch(ta)
    {
        case CTA4:
            c = (word4)get4(p->data, *pos);
            break;
        case CTA6:
            c = (word6)get6(p->data, *pos);
            break;
        case CTA9:
            c = (word9)get9(p->data, *pos);
            break;
    }
    
    *pos += 1;
    //p->lastAddress = p->address;
    return c;
}

/*!
 * return a 4-, 6- or 9-bit character at memory "*address" and position "*pos". Decrement pos (and address if necesary)
 * NB: must be initialized before use or else unpredictable side-effects may result. Not thread safe
 */

PRIVATE
int EISget469r(EISaddr *p, int *pos, int ta)
{
    //static word18 lastAddress;// try to keep memory access' down
    //static word36 data;
    
    if (!p)
   // {
    //    lastAddress = -1;
        return 0;
   // }
    
    int maxPos = 3; // CTA9
    switch(ta)
    {
        case CTA4:
            maxPos = 7;
            break;
            
        case CTA6:
            maxPos = 5;
            break;
    }
    
    if (*pos < 0)  // underlows to next word?
    {
        *pos = maxPos;
        p->address = (p->address - 1) & AMASK;          // bump source to prev address
        p->data = EISRead(p);    // read it from memory
    }
    
    //if (p->lastAddress != p->address) {                // read from memory if different address
        p->data = EISRead(p);   // read data word from memory
    //    p->lastAddress = p->address;
    //}
    
    int c = 0;
    switch(ta)
    {
        case CTA4:
            c = (word4)get4(p->data, *pos);
            break;
        case CTA6:
            c = (word6)get6(p->data, *pos);
            break;
        case CTA9:
            c = (word9)get9(p->data, *pos);
            break;
    }
    
    *pos -= 1;
    
    return c;
}

/*!
 * load a decimal number into e->x ...
 */
PRIVATE
void loadDec(EISaddr *p, int pos, EISstruct *e)
{
    int128 x = 0;
    
    
    // XXX use get49() for this later .....
    //word36 data;
    //Read(e->ins, sourceAddr, &data, OperandRead, 0);    // read data word from memory
    p->data = EISRead(p);    // read data word from memory
    
    int maxPos = e->TN1 == CTN4 ? 7 : 3;

    int sgn = 1;
    
    for(int n = 0 ; n < e->N1 ; n += 1)
    {
        if (pos > maxPos)   // overflows to next word?
        {   // yep....
            pos = 0;        // reset to 1st byte
            //sourceAddr = (sourceAddr + 1) & AMASK;      // bump source to next address
            //Read(e->ins, sourceAddr, &data, OperandRead, 0);    // read it from memory
            p->address = (p->address + 1) & AMASK;      // bump source to next address
            p->data = EISRead(p);    // read it from memory
        }
        
        int c = 0;
        switch(e->TN1)
        {
            case CTN4:
                c = (word4)get4(p->data, pos);
                break;
            case CTN9:
                c = (word9)GETBYTE(p->data, pos);
                break;
        }
        //fprintf(stderr, "c=%d\n", c);
        
        if (n == 0 && e->TN1 == CTN4 && e->S1 == CSLS)
        {
            switch (c)
            {
                case 015:   // 6-bit - sign
                    SETF(e->_flags, I_NEG);
                    
                    sgn = -1;
                    break;
                case 013:   // alternate 4-bit + sign
                case 014:   // default   4-bit + sign
                    break;
                default:
                    fprintf(stderr, "loadDec:1\n");
                    // not a leading sign
                    // XXX generate Ill Proc fault
            }
            continue;
        } else if (n == 0 && e->TN1 == CTN9 && e->S1 == CSLS)
        {
            switch (c)
            {
                case '-':
                    SETF(e->_flags, I_NEG);
                    
                    sgn = -1;
                    break;
                case '+':
                    break;
                default:
                    fprintf(stderr, "loadDec:2\n");
                    // not a leading sign
                    // XXX generate Ill Proc fault

            }
            continue;
        } else if (n == e->N1-1 && e->TN1 == CTN4 && e->S1 == CSTS)
        {
            switch (c)
            {
                case 015:   // 4-bit - sign
                    SETF(e->_flags, I_NEG);
                    
                    sgn = -1;
                    break;
                case 013:   // alternate 4-bit + sign
                case 014:   // default   4-bit + sign
                    break;
                default:
                    fprintf(stderr, "loadDec:3\n");
                    // not a trailing sign
                    // XXX generate Ill Proc fault
            }
            continue;
        } else if (n == e->N1-1 && e->TN1 == CTN9 && e->S1 == CSTS)
        {
            switch (c)
            {
                case '-':
                    SETF(e->_flags, I_NEG);
                    
                    sgn = -1;
                    break;
                case '+':
                    break;
                default:
                    fprintf(stderr, "loadDec:4\n");
                    // not a trailing sign
                    // XXX generate Ill Proc fault
            }
            continue;
        }
        
        x *= 10;
        x += c & 0xf;
        
        pos += 1;           // onto next posotion
    }
    
    e->x = sgn * x;
}

/*!
 * write 9-bit bytes to memory @ pos (in reverse)...
 */
PRIVATE
void EISwrite9r(EISaddr *p, int *pos, int char9)
{
    word36 w;
    if (*pos < 0)    // out-of-range?
    {
        *pos = 3;    // reset to 1st byte
        p->address = (p->address - 1) & AMASK;        // goto next dstAddr in memory
    }
    
    //Read(e->ins, *dstAddr, &w, OperandRead, 0);      // read dst memory into w
    w = EISRead(p);      // read dst memory into w
    
    switch (*pos)
    {
        case 0:
            w = bitfieldInsert36(w, char9, 27, 9);
            break;
        case 1:
            w = bitfieldInsert36(w, char9, 18, 9);
            break;
        case 2:
            w = bitfieldInsert36(w, char9, 9, 9);
            break;
        case 3:
            w = bitfieldInsert36(w, char9, 0, 9);
            break;
    }
    
    //Write(e->ins, *dstAddr, w, OperandWrite, 0); // XXX this is the ineffecient part!
    EISWrite(p, w); // XXX this is the ineffecient part!
    
    *pos -= 1;       // to prev byte.
}

/*!
 * write 6-bit chars to memory @ pos (in reverse)...
 */
PRIVATE
void EISwrite6r(EISaddr *p, int *pos, int char6)
{
    word36 w;
    if (*pos < 0)    // out-of-range?
    {
        *pos = 5;    // reset to 1st byte
        p->address = (p->address - 1) & AMASK;        // goto next dstAddr in memory
    }
    
    //Read(e->ins, *dstAddr, &w, OperandRead, 0);      // read dst memory into w
    w = EISRead(p);      // read dst memory into w
    
    switch (*pos)
    {
        case 0:
            w = bitfieldInsert36(w, char6, 30, 6);
            break;
        case 1:
            w = bitfieldInsert36(w, char6, 24, 6);
            break;
        case 2:
            w = bitfieldInsert36(w, char6, 18, 6);
            break;
        case 3:
            w = bitfieldInsert36(w, char6, 12, 6);
            break;
        case 4:
            w = bitfieldInsert36(w, char6, 6, 6);
            break;
        case 5:
            w = bitfieldInsert36(w, char6, 0, 6);
            break;
    }
    
    //Write(e->ins, *dstAddr, w, OperandWrite, 0); // XXX this is the ineffecient part!
    EISWrite(p, w); // XXX this is the ineffecient part!
    
    *pos -= 1;       // to prev byte.
}



/*!
 * write 4-bit digits to memory @ pos (in reverse) ...
 */
PRIVATE
void EISwrite4r(EISaddr *p, int *pos, int char4)
{
    word36 w;
    
    if (*pos < 0)    // out-of-range?
    {
        *pos = 7;    // reset to 1st byte
        p->address = (p->address - 1) & AMASK;         // goto prev dstAddr in memory
    }
    //Read(e->ins, *dstAddr, &w, OperandRead, 0);      // read dst memory into w
    w = EISRead(p);
    
    switch (*pos)
    {
        case 0:
            w = bitfieldInsert36(w, char4, 31, 4);
            break;
        case 1:
            w = bitfieldInsert36(w, char4, 27, 4);
            break;
        case 2:
            w = bitfieldInsert36(w, char4, 22, 4);
            break;
        case 3:
            w = bitfieldInsert36(w, char4, 18, 4);
            break;
        case 4:
            w = bitfieldInsert36(w, char4, 13, 4);
            break;
        case 5:
            w = bitfieldInsert36(w, char4, 9, 4);
            break;
        case 6:
            w = bitfieldInsert36(w, char4, 4, 4);
            break;
        case 7:
            w = bitfieldInsert36(w, char4, 0, 4);
            break;
    }
    
    //Write(e->ins, *dstAddr, w, OperandWrite, 0); // XXX this is the ineffecient part!
    EISWrite(p, w); // XXX this is the ineffecient part!
    
    *pos -= 1;       // to prev byte.
}

/*!
 * write 4-bit chars to memory @ pos ...
 */
PRIVATE
void EISwrite4(EISaddr *p, int *pos, int char4)
{
    word36 w;
    if (*pos > 7)    // out-of-range?
    {
        *pos = 0;    // reset to 1st byte
        p->address = (p->address + 1) & AMASK;        // goto next dstAddr in memory
    }
    
    w = EISRead(p);      // read dst memory into w
    
    switch (*pos)
    {
        case 0:
            w = bitfieldInsert36(w, char4, 31, 4);
            break;
        case 1:
            w = bitfieldInsert36(w, char4, 27, 4);
            break;
        case 2:
            w = bitfieldInsert36(w, char4, 22, 4);
            break;
        case 3:
            w = bitfieldInsert36(w, char4, 18, 4);
            break;
        case 4:
            w = bitfieldInsert36(w, char4, 13, 4);
            break;
        case 5:
            w = bitfieldInsert36(w, char4, 9, 4);
            break;
        case 6:
            w = bitfieldInsert36(w, char4, 4, 4);
            break;
        case 7:
            w = bitfieldInsert36(w, char4, 0, 4);
            break;
    }
    
    
    EISWrite(p, w); // XXX this is the ineffecient part!
    
    *pos += 1;       // to next char.
}

/*!
 * write 6-bit digits to memory @ pos ...
 */
PRIVATE
void EISwrite6(EISaddr *p, int *pos, int char6)
{
    word36 w;
    
    if (*pos > 5)    // out-of-range?
    {
        *pos = 0;    // reset to 1st byte
        p->address = (p->address + 1) & AMASK;        // goto next dstAddr in memory
    }
    
    w = EISRead(p);      // read dst memory into w
    
    switch (*pos)
    {
        case 0:
            w = bitfieldInsert36(w, char6, 30, 6);
            break;
        case 1:
            w = bitfieldInsert36(w, char6, 24, 6);
            break;
        case 2:
            w = bitfieldInsert36(w, char6, 18, 6);
            break;
        case 3:
            w = bitfieldInsert36(w, char6, 12, 6);
            break;
        case 4:
            w = bitfieldInsert36(w, char6, 6, 6);
            break;
        case 5:
            w = bitfieldInsert36(w, char6, 0, 6);
            break;
    }
    
    EISWrite(p, w); // XXX this is the ineffecient part!
    
    *pos += 1;       // to next byte.
}

/*!
 * write 9-bit bytes to memory @ pos ...
 */
PRIVATE
void EISwrite9(EISaddr *p, int *pos, int char9)
{
    word36 w;
    if (*pos > 3)    // out-of-range?
    {
        *pos = 0;    // reset to 1st byte
        p->address = (p->address + 1) & AMASK;       // goto next dstAddr in memory
    }
    
    w = EISRead(p);      // read dst memory into w
   
    switch (*pos)
    {
        case 0:
            w = bitfieldInsert36(w, char9, 27, 9);
            break;
        case 1:
            w = bitfieldInsert36(w, char9, 18, 9);
            break;
        case 2:
            w = bitfieldInsert36(w, char9, 9, 9);
            break;
        case 3:
            w = bitfieldInsert36(w, char9, 0, 9);
            break;
    }
    
    EISWrite(p, w); // XXX this is the ineffecient part!
    
    *pos += 1;       // to next byte.
}

/*!
 * write a 4-, 6-, or 9-bit char to dstAddr ....
 */
PRIVATE
void EISwrite469(EISaddr *p, int *pos, int ta, int c469)
{
    switch(ta)
    {
        case CTA4:
            return EISwrite4(p, pos, c469);
        case CTA6:
            return EISwrite6(p, pos, c469);
        case CTA9:
            return EISwrite9(p, pos, c469);
    }
    
}

/*!
 * write a 4-, 6-, or 9-bit char to dstAddr (in reverse)....
 */
PRIVATE
void EISwrite469r(EISaddr *p, int *pos, int ta, int c469)
{
    switch(ta)
    {
        case CTA4:
            return EISwrite4r(p, pos, c469);
        case CTA6:
            return EISwrite6r(p, pos, c469);
        case CTA9:
            return EISwrite9r(p, pos, c469);
    }
    
}

/*!
 * write a 4-, or 9-bit numeric char to dstAddr ....
 */
void EISwrite49(EISaddr *p, int *pos, int tn, int c49)
{
    switch(tn)
    {
        case CTN4:
            return EISwrite4(p, pos, c49);
        case CTN9:
            return EISwrite9(p, pos, c49);
    }
}

/*!
 * write char to output string in Reverse. Right Justified and taking into account string length of destination
 */
static void EISwriteToOutputStringReverse(EISstruct *e, int k, int charToWrite)
{
    /// first thing we need to do is to find out the last position is the buffer we want to start writing to.
    
    static int N = 0;           ///< length of output buffer in native chars (4, 6 or 9-bit chunks)
    static int CN = 0;          ///< character number 0-7 (4), 0-5 (6), 0-3 (9)
    static int TN = 0;          ///< type code
    //static word18 address  = 0; ///< current write address
    static int pos = 0;         ///< current character position
    static int size = 0;        ///< size of char
    static int _k = -1;         ///< k of MFk

    if (k)
    {
        _k = k;
        
        N = e->N[k-1];      // length of output buffer in native chars (4, 9-bit chunks)
        CN = e->CN[k-1];    // character number (position) 0-7 (4), 0-5 (6), 0-3 (9)
        TN = e->TN[k-1];    // type code
        
        switch (TN)
        {
            case CTN4:
                //address = e->addr[k-1].address;
                size = 4;
                break;
            case CTN9:
                //address = e->addr[k-1].address;
                size = 9;
                break;
        }
        
        /// since we want to write the data in reverse (since it's right justified) we need to determine
        /// the final address/CN for the type and go backwards from there
        
        int numBits = ((TN == CTN4) ? 4 : 9) * N;               ///< 8 4-bit digits, 4 9-bit bytes / word
        ///< (remember there are 4 slop bits in a 36-bit word when dealing with BCD)
        //int numWords = numBits / ((TN == CTN4) ? 32 : 36);      ///< how many additional words will the N chars take up?
        int numWords = (numBits-1 + CN * size) / ((TN == CTN4) ? 32 : 36);      ///< how many additional words will the N chars take up?
        int lastChar = (CN + N - 1) % ((TN == CTN4) ? 8 : 4);   ///< last character number
        
        if (numWords > 0)           // more that the 1 word needed?
            //address += numWords;    // highest memory address
            e->addr[k-1].address += numWords;
        
        pos = lastChar;             // last character number
        
        //fprintf(stderr, "numWords=%d lastChar=%d\n", numWords, lastChar);
        return;
    }
    
    // any room left in output string?
    if (N == 0)
    {
        SETF(e->_flags, I_OFLOW);
        
        return;
    }
    
    // we should write character to word/pos in memory .....
    switch(TN)
    {
        case CTN4:
            EISwrite4r(&e->addr[_k-1], &pos, charToWrite);
            break;
        case CTN9:
            EISwrite9r(&e->addr[_k-1], &pos, charToWrite);
            break;
    }
    N -= 1;
}

PRIVATE
void EISwriteToBinaryStringReverse(EISaddr *p, int k)
{
    /// first thing we need to do is to find out the last position is the buffer we want to start writing to.
    
    int N = p->e->N[k-1];            ///< length of output buffer in native chars (4, 6 or 9-bit chunks)
    int CN = p->e->CN[k-1];          ///< character number 0-3 (9)
    //word18 address  = e->YChar9[k-1]; ///< current write address
    
    /// since we want to write the data in reverse (since it's right justified) we need to determine
    /// the final address/CN for the type and go backwards from there
    
    int numBits = 9 * N;               ///< 4 9-bit bytes / word
    //int numWords = numBits / 36;       ///< how many additional words will the N chars take up?
    int numWords = (numBits + CN * 9) / 36;       ///< how many additional words will the N chars take up?
    int lastChar = (CN + N - 1) % 4;   ///< last character number
    
    if (numWords > 1)           // more that the 1 word needed?
        p->address += numWords;    // highest memory address
    int pos = lastChar;             // last character number
    
    int128 x = p->e->x;
    
    for(int n = 0 ; n < N ; n += 1)
    {
        int charToWrite = x & 0777; // get 9-bits of data
        x >>=9;
        
        // we should write character to word/pos in memory .....
        //write9r(e, &address, &pos, charToWrite);
        EISwrite9r(p, &pos, charToWrite);
    }
    
    // anything left in x?. If it's not all 1's we have an overflow!
    if (~x)    // if it's all 1's this will be 0
        SETF(p->e->_flags, I_OFLOW);
}



/// perform a binary to decimal conversion ...

/// Since (according to DH02) we want to "right-justify" the output string it might be better to presere the reverse writing and start writing
/// characters directly into the output string taking into account the output string length.....


PRIVATE
void _btd(EISstruct *e)
{
    word72s n128 = e->x;    ///< signExt9(e->x, e->N1);          ///< adjust for +/-
    int sgn = (n128 < 0) ? -1 : 1;  ///< sgn(x)
    if (n128 < 0)
        n128 = -n128;
    
    //if (n128 == 0)  // If C(Y-charn2) = decimal 0, then ON: otherwise OFF
        //SETF(e->_flags, I_ZERO);
    SCF(n128 == 0, e->_flags, I_ZERO);
   
    int N = e->N2;  // number of chars to write ....
    
    // handle any trailing sign stuff ...
    if (e->S2 == CSTS)  // a trailing sign
    {
        EISwriteToOutputStringReverse(e, 0, getSign(sgn, e));
        if (TSTF(e->_flags, I_OFLOW))   // Overflow! Too many chars, not enough room!
            return;
        N -= 1;
    }
    do
    {
        int n = n128 % 10;
        
        EISwriteToOutputStringReverse(e, 0, (e->TN2 == CTN4) ? n : (n + '0'));
        
        if (TSTF(e->_flags, I_OFLOW))   // Overflow! Too many chars, not enough room!
            return;
        
        N -= 1;
        
        n128 /= 10;
    } while (n128);
    
    // at this point we've exhausted our digits, but may still have spaces left.
    
    // handle any leading sign stuff ...
    if (e->S2 == CSLS)  // a leading sign
    {
        while (N > 1)
        {
            EISwriteToOutputStringReverse(e, 0, (e->TN2 == CTN4) ? 0 : '0');
            N -= 1;
        }
        EISwriteToOutputStringReverse(e, 0, getSign(sgn, e));
        if (TSTF(e->_flags, I_OFLOW))   // Overflow! Too many chars, not enough room!
            return;
    }
    else
    {
        while (N > 0)
        {
            EISwriteToOutputStringReverse(e, 0, (e->TN2 == CTN4) ? 0 : '0');
            N -= 1;
        }
    }
}

#ifndef QUIET_UNUSED
char *_btdOld(EISstruct *e)
{
    word72s n128 = e->x;    //signExt9(e->x, e->N1);          // adjust for +/-
    int sgn = (n128 < 0) ? -1 : 1;  // sgn(x)
    if (n128 < 0)
        n128 = -n128;
    
    memset(e->buff, 0, sizeof(e->buff));       // fill buff with all 1's so we know where we stopped
    e->p = e->buff;
    
    // handle any leading sign stuff ...
    if (e->S2 == CSLS)  // a trailing sign
        addSign(sgn, e);
    
    char bcd[23];   // 2^72 = ~22 digits
    memset(bcd, 0, sizeof(bcd));
    
#ifdef ALG2 // determine which is faster
    // less elegant but simpler operations
    if (n128 > 0)
    {
        
        // Xilinx XAPP029 Serial Code Conversion between BCD and Binary application note v1 1 (10/97)
        // http://www.eng.utah.edu/~nmcdonal/Tutorials/BCDTutorial/BCDConversion.html
        
        for(int i = 71 ; i >= 0 ; i -= 1)
        {
            // add 3 to columns >= 5
            for(int j = 0 ; j < sizeof(bcd); j += 1)
                if (bcd[j] >= 5)
                    bcd[j] += 3;
            
            // shift all binary digits left 1
            for(int j = sizeof(bcd) - 1; j >= 0 ; j -= 1)
            {
                if (j > 0)
                {
                    bcd[j] = (bcd[j] << 1) & 0xf;
                    if (bcd[j - 1] & 8)
                        bcd[j] |= 1;
                } else {
                    bcd[j] = (bcd[j] << 1) & 0xf;
                    if (n & ((uint128)1 << 71))
                        bcd[0] |= 1;
                    n = (n << 1);
                }
            }
            //        for(int j = 0 ; j < sizeof(bcd) ; j += 1)
            //        {
            //            if (j < sizeof(bcd) - 1)
            //            {
            //                bcd[j] = (bcd[j] << 1) & 0xf;
            //                if (bcd[j + 1] & 8)
            //                    bcd[j] |= 1;
            //            } else {
            //                bcd[j] = (bcd[j] << 1) & 0xf;
            //                if (n & ((uint128)1 << 71))
            //                    bcd[sizeof(bcd)-1] |= 1;
            //                n = (n << 1);
            //            }
            //        }
        }
    } else
        // just a single 0
        bcd[0] = '0';
    
#else
    
    // elegant but slower?
    char *q = bcd;
    do
    {
        int n = n128 % 10;
        
        *q++ = n + '0'; // write as ascii to keep tract of what we've generated
        
        n128 /= 10;
    } while (n128);
    
#endif
    
    // copy bcd to e->buff ....
    //    int i = 0;
    //    for(; i < sizeof(bcd) ; i ++)
    //        if (bcd[i])
    //            break;
    //    for(; i < sizeof(bcd) ; i ++)
    //        *(e->p++) = bcd[i];
    int i = sizeof(bcd)-1;
    for(; i >= 0 ; i -= 1)
        if (bcd[i])
            break;
    for(; i >= 0 ; i -= 1)
        *(e->p++) = (bcd[i] & 017) + ((e->TN2 == CTN4) ? 0 : '0');    // write as 4- or 9-bit
    
    
    // handle any trailing sign stuff ...
    if (e->S2 == CSTS)  // a leading sign
        addSign(sgn, e);
    
    return e->buff;
    
    // e->p points to just after last char of converted data
}
#endif

void btd(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    
    //! \brief C(Y-char91) converted to decimal → C(Y-charn2)
    /*!
     * C(Y-char91) contains a twos complement binary integer aligned on 9-bit character boundaries with length 0 < N1 <= 8.
     * If TN2 and S2 specify a 4-bit signed number and P = 1, then if C(Y-char91) is positive (bit 0 of C(Y-char91)0 = 0), then the 13(8) plus sign character is moved to C(Y-charn2) as appropriate.
     *   The scaling factor of C(Y-charn2), SF2, must be 0.
     *   If N2 is not large enough to hold the digits generated by conversion of C(Y-char91) an overflow condition exists; the overflow indicator is set ON and an overflow fault occurs. This implies that an unsigned fixed-point receiving field has a minimum length of 1 character and a signed fixed- point field, 2 characters.
     * If MFk.RL = 1, then Nk does not contain the operand length; instead; it contains a register code for a register holding the operand length.
     * If MFk.ID = 1, then the kth word following the instruction word does not contain an operand descriptor; instead, it contains an indirect pointer to the operand descriptor.
     * C(Y-char91) and C(Y-charn2) may be overlapping strings; no check is made.
     * Attempted conversion to a floating-point number (S2 = 0) or attempted use of a scaling factor (SF2 ≠ 0) causes an illegal procedure fault.
     * If N1 = 0 or N1 > 8 an illegal procedure fault occurs.
     * Attempted execution with the xed instruction causes an illegal procedure fault.
     * Attempted repetition with the rpt, rpd, or rpl instructions causes an illegal procedure fault.
     */
    
    //! C(string 1) -> C(string 2) (converted)
    
    //! The two's complement binary integer starting at location YC1 is converted into a signed string of decimal characters of data type TN2, sign and decimal type S2 (S2 = 00 is illegal) and scale factor 0; and is stored, right-justified, as a string of length L2 starting at location YC2. If the string generated is longer than L2, the high-order excess is truncated and the overflow indicator is set. If strings 1 and 2 are not overlapped, the contents of string 1 remain unchanged. The length of string 1 (L1) is given as the number of 9-bit segments that make up the string. L1 is equal to or is less than 8. Thus, the binary string to be converted can be 9, 18, 27, 36, 45, 54, 63, or 72 bits long. CN1 designates a 9-bit character boundary. If P=1, positive signed 4-bit results are stored using octal 13 as the plus sign. If P=0, positive signed 4-bit results are stored with octal 14 as the plus sign.

    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);

    parseNumericOperandDescriptor(1, e);
    parseNumericOperandDescriptor(2, e);
    
    e->P = (bool)bitfieldExtract36(e->op0, 35, 1);  // 4-bit data sign character control
    
    //word18 addr = (e->TN1 == CTN4) ? e->YChar41 : e->YChar91;
    //load9x(e->N1, addr, e->CN1, e);
    
    load9x(e->N1, &e->ADDR1, e->CN1, e);
    
    EISwriteToOutputStringReverse(e, 2, 0);    // initialize output writer .....
    
    e->_flags = rIR;
    
    CLRF(e->_flags, I_NEG);     // If a minus sign character is moved to C(Y-charn2), then ON; otherwise OFF
    CLRF(e->_flags, I_ZERO);    // If C(Y-charn2) = decimal 0, then ON: otherwise OFF

    _btd(e);
    
    rIR = e->_flags;
    if (TSTF(rIR, I_OFLOW))
        ;   // XXX generate overflow fault
    
}

void dtb(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseNumericOperandDescriptor(1, e);
    parseNumericOperandDescriptor(2, e);
   
    //Attempted conversion of a floating-point number (S1 = 0) or attempted use of a scaling factor (SF1 ≠ 0) causes an illegal procedure fault.
    //If N2 = 0 or N2 > 8 an illegal procedure fault occurs.
    if (e->S1 == 0 || e->SF1 != 0 || e->N2 == 0 || e->N2 > 8)
        ; // generate ill proc fault
    
    e->_flags = rIR;
    
    // Negative: If a minus sign character is found in C(Y-charn1), then ON; otherwise OFF
    CLRF(e->_flags, I_NEG);
    
    //loadDec(e->TN1 == CTN4 ? e->YChar41 : e->YChar91, e->CN1, e);
    loadDec(&e->ADDR1, e->CN1, e);
    
    // Zero: If C(Y-char92) = 0, then ON: otherwise OFF
    SCF(e->x == 0, e->_flags, I_ZERO);
    
    EISwriteToBinaryStringReverse(&e->ADDR2, 2);
    
    rIR = e->_flags;

    if (TSTF(rIR, I_OFLOW))
        ;   // XXX generate overflow fault
    
}

/*!
 * Edit instructions & support code ...
 */
PRIVATE
void EISwriteOutputBufferToMemory(EISaddr *p)
{
    //4. If an edit insertion table entry or MOP insertion character is to be stored, ANDed, or ORed into a receiving string of 4- or 6-bit characters, high-order truncate the character accordingly.
    //5. If the receiving string is 9-bit characters, high-order fill the (4-bit) digits from the input buffer with bits 0-4 of character 8 of the edit insertion table. If the receiving string is 6-bit characters, high-order fill the digits with "00"b.
    
    int pos = p->e->dstCN; // what char position to start writing to ...
    //word24 dstAddr = p->e->dstAddr;
    
    for(int n = 0 ; n < p->e->dstTally ; n++)
    {
        int c49 = p->e->outBuffer[n];
        
        switch(p->e->dstSZ)
        {
            case 4:
                EISwrite4(p, &pos, c49 & 0xf);    // write only-4-bits
                break;
            case 6:
                EISwrite6(p, &pos, c49 & 0x3f);   // write only 6-bits
                break;
            case 9:
                EISwrite9(p, &pos, c49);
                break;
        }
        //                break;
        //        }
    }
}

PRIVATE
void writeToOutputBuffer(EISstruct *e, word9 **dstAddr, int szSrc, int szDst, int c49)
{
    //4. If an edit insertion table entry or MOP insertion character is to be stored, ANDed, or ORed into a receiving string of 4- or 6-bit characters, high-order truncate the character accordingly.
    //5. If the receiving string is 9-bit characters, high-order fill the (4-bit) digits from the input buffer with bits 0-4 of character 8 of the edit insertion table. If the receiving string is 6-bit characters, high-order fill the digits with "00"b.
    
    switch (szSrc)
    {
        case 4:
            switch(szDst)
            {
                case 4:
                    **dstAddr = c49 & 0xf;
                    break;
                case 6:
                    **dstAddr = c49 & 077;   // high-order fill the digits with "00"b.
                    break;
                case 9:
                    **dstAddr = c49 | (e->editInsertionTable[7] & 0760);
                    break;
            }
            break;
        case 6:
            switch(szDst)
            {
                case 4:
                    **dstAddr = c49 & 0xf;    // write only-4-bits
                    break;
                case 6:
                    **dstAddr = c49;   
                    break;
                case 9:
                    **dstAddr = c49;
                    break;
            }
            break;
        case 9:
            switch(szDst)
            {
                case 4:
                    **dstAddr = c49 & 0xf;    // write only-4-bits
                    break;
                case 6:
                    **dstAddr = c49 & 077;   // write only 6-bits
                    break;
                case 9:
                    **dstAddr = c49;
                    break;
            }
            break;
    }
    e->dstTally -= 1;
    *dstAddr += 1;
}


/*!
 * Load the entire sending string number (maximum length 63 characters) into the decimal unit input buffer as 4-bit digits (high-order truncating 9-bit data). Strip the sign and exponent characters (if any), put them aside into special holding registers and decrease the input buffer count accordingly.
 */
void EISloadInputBufferNumeric(DCDstruct *ins, int k)
{
    EISstruct *e = ins->e;
    
    word9 *p = e->inBuffer; // p points to position in inBuffer where 4-bit chars are stored
    memset(e->inBuffer, 0, sizeof(e->inBuffer));   // initialize to all 0's
    
    int pos = e->CN[k-1];
    
    int TN = e->TN[k-1];
    int S = e->S[k-1];  // This is where MVNE gets really nasty.
    // I spit on the designers of this instruction set (and of COBOL.) >Ptui!<
    
    int N = e->N[k-1];  // number of chars in src string
    
    //word18 addr = (TN == CTN4) ? e->YChar4[k-1] : e->YChar9[k-1];   // get address of numeric source string
    EISaddr *a = &e->addr[k-1];
    
    e->sign = 1;
    e->exponent = 0;
    
    // load according to MFk.....
    //word36 data;
    //Read(addr, &data, OperandRead, 0);    // read data word from memory
    
    //int maxPos = e->TN1 == CTN4 ? 7 : 3;
    EISget49(NULL, 0, 0);
    
    for(int n = 0 ; n < N ; n += 1)
    {
        int c = EISget49(a, &pos, TN);
        
        /*
         * Here we need to distinguish between 4 type of numbers.
         *
         * CSFL - Floating-point, leading sign
         * CSLS - Scaled fixed-point, leading sign
         * CSTS - Scaled fixed-point, trailing sign
         * CSNS - Scaled fixed-point, unsigned
         */
        switch(S)
        {
            case CSFL:  // this is the real evil one ....
                /* Floating-point:
                 * [sign=c0] c1×10(n-3) + c2×10(n-4) + ... + c(n-3) [exponent=8 bits]
                 * where:
                 *  ci is the decimal value of the byte in the ith byte position.
                 *  [sign=ci] indicates that ci is interpreted as a sign byte.
                 *  [exponent=8 bits] indicates that the exponent value is taken from the last 8 bits of the string. If the data is in 9-bit bytes, the exponent is bits 1-8 of c(n-1). If the data is in 4- bit bytes, the exponent is the binary value of the concatenation of c(n-2) and c(n-1).
                 */
                if (n == 0) // first had better be a sign ....
                {
                    c &= 0xf;   // hack off all but lower 4 bits
                    
                    if (c < 012 || c > 017)
                        doFault(ins, 0, 0, "loadInputBufferNumric(1): illegal char in input"); // TODO: generate ill proc fault
                    
                    if (c == 015)   // '-'
                        e->sign = -1;
                    
                    e->srcTally -= 1;   // 1 less source char
                }
                else if (TN == CTN9 && n == N-1)    // the 9-bit exponent (of which only 8-bits are used)
                {
                    e->exponent = (signed char)(c & 0377); // want to do a sign extend
                    e->srcTally -= 1;   // 1 less source char
                }
                else if (TN == CTN4 && n == N-2)    // the 1st 4-chars of the 8-bit exponent
                {
                    e->exponent = (c & 0xf);// << 4;
                    e->exponent <<= 4;
                    e->srcTally -= 1;   // 1 less source char
                }
                else if (TN == CTN4 && n == N-1)    // the 2nd 4-chars of the 8-bit exponent
                {
                    e->exponent |= (c & 0xf);
                    
                    signed char ce = e->exponent & 0xff;
                    e->exponent = ce;
                    
                    e->srcTally -= 1;   // 1 less source char
                }
                else
                {
                    c &= 0xf;   // hack off all but lower 4 bits
                    if (c > 011)
                        doFault(ins, 0,0,"loadInputBufferNumric(2): illegal char in input"); // TODO: generate ill proc fault
                    
                    *p++ = c; // store 4-bit char in buffer
                }
                break;
                
            case CSLS:
                // Only the byte values [0,11]8 are legal in digit positions and only the byte values [12,17]8 are legal in sign positions. Detection of an illegal byte value causes an illegal procedure fault
                c &= 0xf;   // hack off all but lower 4 bits
                
                if (n == 0) // first had better be a sign ....
                {
                    if (c < 012 || c > 017)
                        doFault(ins, 0,0,"loadInputBufferNumric(3): illegal char in input"); // TODO: generate ill proc fault
                    if (c == 015)   // '-'
                        e->sign = -1;
                    e->srcTally -= 1;   // 1 less source char
                }
                else
                {
                    if (c > 011)
                        doFault(ins, 0,0,"loadInputBufferNumric(4): illegal char in input"); // XXX generate ill proc fault
                    *p++ = c; // store 4-bit char in buffer
                }
                break;
                
            case CSTS:
                c &= 0xf;   // hack off all but lower 4 bits
                
                if (n == N-1) // last had better be a sign ....
                {
                    if (c < 012 || c > 017)
                        ; // XXX generate ill proc fault
                    if (c == 015)   // '-'
                        e->sign = -1;
                    e->srcTally -= 1;   // 1 less source char
                }
                else
                {
                    if (c > 011)
                        doFault(ins, 0,0,"loadInputBufferNumric(5): illegal char in input"); // XXX generate ill proc fault
                    *p++ = c; // store 4-bit char in buffer
                }
                break;
                
            case CSNS:
                c &= 0xf; // hack off all but lower 4 bits
                
                *p++ = c; // the "easy" one
                break;
        }
    }
}

/*!
 * Load decimal unit input buffer with sending string characters. Data is read from main memory in unaligned units (not modulo 8 boundary) of Y-block8 words. The number of characters loaded is the minimum of the remaining sending string count, the remaining receiving string count, and 64.
 */
PRIVATE
void EISloadInputBufferAlphnumeric(EISstruct *e, int k)
{
    word9 *p = e->inBuffer; // p points to position in inBuffer where 4-bit chars are stored
    memset(e->inBuffer, 0, sizeof(e->inBuffer));   // initialize to all 0's
    
    int pos = e->CN[k-1];
    
    int TA = e->TA[k-1];
    
    int N = min3(e->N1, e->N3, 64);  // minimum of the remaining sending string count, the remaining receiving string count, and 64.
    
    //word18 addr = 0;
//    switch(TA)
//    {
//        case CTA4:
//            //addr = e->YChar4[k-1];
//            break;
//        case CTA6:
//            //addr = e->YChar6[k-1];
//            break;
//        case CTA9:
//            //addr = e->YChar9[k-1];
//            break;
//    }
    
    
    //get469(NULL, 0, 0, 0);    // initialize char getter buffer
    EISaddr *a = &e->addr[k-1];
    
    for(int n = 0 ; n < N ; n += 1)
    {
        int c = EISget469(a, &pos, TA);
        *p++ = c;
    }
    
   // get469(NULL, 0, 0, 0);    // initialize char getter buffer
    
}

/*!
 * MVE/MVNE ...
 */

///< MicroOperations ...
///< Table 4-9. Micro Operation Code Assignment Map
#ifndef QUIET_UNUSED
PRIVATE
char* mopCodes[040] = {
    //        0       1       2       3       4       5       6       7
    /* 00 */  0,     "insm", "enf",  "ses",  "mvzb", "mvza", "mfls", "mflc",
    /* 10 */ "insb", "insa", "insn", "insp", "ign",  "mvc",  "mses", "mors",
    /* 20 */ "lte",  "cht",   0,      0,      0,      0,      0,      0,
    /* 30 */   0,      0,     0,      0,      0,      0,      0,      0
};
#endif

PRIVATE
MOPstruct mopTab[040] = {
    {NULL, 0},
    {"insm", mopINSM },
    {"enf",  mopENF  },
    {"ses",  mopSES  },
    {"mvzb", mopMVZB },
    {"mvza", mopMVZA },
    {"mfls", mopMFLS },
    {"mflc", mopMFLC },
    {"insb", mopINSB },
    {"insa", mopINSA },
    {"insn", mopINSN },
    {"insp", mopINSP },
    {"ign",  mopIGN  },
    {"mvc",  mopMVC  },
    {"mses", mopMSES },
    {"mors", mopMORS },
    {"lte",  mopLTE  },
    {"cht",  mopCHT  },
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0},
    {NULL, 0}
};

PRIVATE
char* defaultEditInsertionTable = " *+-$,.0";


//PRIVATE
//MOPstruct* getMop(EISstruct *e);

// Edit Flags
// The processor provides the following four edit flags for use by the micro operations.

//bool mopES = false; // End Suppression flag; initially OFF, set ON by a micro operation when zero-suppression ends.
//bool mopSN = false; // Sign flag; initially set OFF if the sending string has an alphanumeric descriptor or an unsigned numeric descriptor. If the sending string has a signed numeric descriptor, the sign is initially read from the sending string from the digit position defined by the sign and the decimal type field (S); SN is set OFF if positive, ON if negative. If all digits are zero, the data is assumed positive and the SN flag is set OFF, even when the sign is negative.
//bool mopZ = true;   // Zero flag; initially set ON and set OFF whenever a sending string character that is not decimal zero is moved into the receiving string.
//bool mopBZ = false; // Blank-when-zero flag; initially set OFF and set ON by either the ENF or SES micro operation. If, at the completion of a move (L1 exhausted), both the Z and BZ flags are ON, the receiving string is filled with character 1 of the edit insertion table.

/*!
 * CHT Micro Operation - Change Table
 * EXPLANATION: The edit insertion table is replaced by the string of eight 9-bit characters immediately following the CHT micro operation.
 * FLAGS: None affected
 * NOTE: C(IF) is not interpreted for this operation.
 ￼￼￼*/
int mopCHT(EISstruct *e)
{
    memset(&e->editInsertionTable, 0, sizeof(e->editInsertionTable)); // XXX do we really need this?
    for(int i = 0 ; i < 8 ; i += 1)
    {
        if (e->mopTally == 0)
        {
            e->_faults |= FAULT_IPR;
            return -1;      // Oops! ran out of micro-operations!
        }
        //word9 entry = get49(e, &e->mopAddr, &e->mopPos, CTN9);  // get mop table entries
        word9 entry = EISget49(e->mopAddress, &e->mopPos, CTN9);  // get mop table entries
        e->editInsertionTable[i] = entry & 0777;            // keep to 9-bits
        
        e->mopTally -= 1;
    }
    return 0;
}

/*!
 * ENF Micro Operation - End Floating Suppression
 * EXPLANATION:
 *  Bit 0 of IF, IF(0), specifies the nature of the floating suppression. Bit 1 of IF, IF(1), specifies if blank when zero option is used.
 * For IF(0) = 0 (end floating-sign operation),
 * − If ES is OFF and SN is OFF, then edit insertion table entry 3 is moved to the receiving field and ES is set ON.
 * − If ES is OFF and SN is ON, then edit insertion table entry 4 is moved to the receiving field and ES is set ON.
 * − If ES is ON, no action is taken.
 * For IF(0) = 1 (end floating currency symbol operation),
 * − If ES is OFF, then edit insertion table entry 5 is moved to the receiving field and ES is set ON.
 * − If ES is ON, no action is taken.
 * For IF(1) = 1 (blank when zero): the BZ flag is set ON. For IF(1) = 0 (no blank when zero): no action is taken.
 * FLAGS: (Flags not listed are not affected)
 *      ES - If OFF, then set ON
 *      BZ - If bit 1 of C(IF) = 1, then set ON; otherwise, unchanged
 */
int mopENF(EISstruct *e)
{
    // For IF(0) = 0 (end floating-sign operation),
    if (!(e->mopIF & 010))
    {
        // If ES is OFF and SN is OFF, then edit insertion table entry 3 is moved to the receiving field and ES is set ON.
        if (!e->mopES && !e->mopSN)
        {
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[2]);
            e->mopES = true;
        }
        // If ES is OFF and SN is ON, then edit insertion table entry 4 is moved to the receiving field and ES is set ON.
        if (!e->mopES && e->mopSN)
        {
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[3]);
            e->mopES = true;
        }
        // If ES is ON, no action is taken.
    } else { // IF(0) = 1 (end floating currency symbol operation),
        if (!e->mopES)
        {
            // If ES is OFF, then edit insertion table entry 5 is moved to the receiving field and ES is set ON.
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[4]);
            e->mopES = true;
        }
        // If ES is ON, no action is taken.
    }
    
    // For IF(1) = 1 (blank when zero): the BZ flag is set ON. For IF(1) = 0 (no blank when zero): no action is taken.
    if (e->mopIF & 04)
        e->mopBZ = true;
    
    return 0;
}

/*!
 * IGN Micro Operation - Ignore Source Characters
 * EXPLANATION:
 * IF specifies the number of characters to be ignored, where IF = 0 specifies 16 characters.
 * The next IF characters in the source data field are ignored and the sending tally is reduced accordingly.
 * FLAGS: None affected
 */
int mopIGN(EISstruct *e)
{
    if (e->mopIF == 0)
        e->mopIF = 16;

    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->srcTally == 0)
            return -1;  // sending buffer exhausted.
        
        e->srcTally -= 1;
        e->in += 1;
    }
    return 0;
}

/*!
 * INSA Micro Operation - Insert Asterisk on Suppression
 * EXPLANATION:
 * This MOP is the same as INSB except that if ES is OFF, then edit insertion table entry 2 is moved to the receiving field.
 * FLAGS: None affected
 * NOTE: If C(IF) = 9-15, an IPR fault occurs.
 */
int mopINSA(EISstruct *e)
{
    // If C(IF) = 9-15, an IPR fault occurs.
    if (e->mopIF >= 9 && e->mopIF <= 15)
    {
        e->_faults |= FAULT_IPR;
        return -1;
    }
    
    // If IF = 0, the 9 bits immediately following the INSB micro operation are treated as a 9-bit character (not a MOP) and are moved or skipped according to ES.
    if (e->mopIF == 0)
    {
        // If ES is OFF, then edit insertion table entry 2 is moved to the receiving field. If IF = 0, then the next 9 bits are also skipped. If IF is not 0, the next 9 bits are treated as a MOP.
        if (!e->mopES)
        {
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[1]);
           
            //get49(e, &e->mopAddr, &e->mopPos, CTN9);
            EISget49(e->mopAddress, &e->mopPos, CTN9);
            e->mopTally -= 1;
        } else {
            // If ES is ON and IF = 0, then the 9-bit character immediately following the INSB micro-instruction is moved to the receiving field.
            //writeToOutputBuffer(e, &e->out, 9, e->dstSZ, get49(e, &e->mopAddr, &e->mopPos, CTN9));
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, EISget49(e->mopAddress, &e->mopPos, CTN9));
            e->mopTally -= 1;
        }
        
    } else {
        // If ES is ON and IF<>0, then IF specifies which edit insertion table entry (1-8) is to be moved to the receiving field.
        if (e->mopES)
        {
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[e->mopIF-1]);
        }
    }
    return 0;
}

/*!
 * INSB Micro Operation - Insert Blank on Suppression
 * EXPLANATION:
 * IF specifies which edit insertion table entry is inserted.
 * If IF = 0, the 9 bits immediately following the INSB micro operation are treated as a 9-bit character (not a MOP) and are moved or skipped according to ES.
 * − If ES is OFF, then edit insertion table entry 1 is moved to the receiving field. If IF = 0, then the next 9 bits are also skipped. If IF is not 0, the next 9 bits are treated as a MOP.
 * − If ES is ON and IF = 0, then the 9-bit character immediately following the INSB micro-instruction is moved to the receiving field.
 * − If ES is ON and IF<>0, then IF specifies which edit insertion table entry (1-8) is to be moved to the receiving field.
 * FLAGS: None affected
 * NOTE: If C(IF) = 9-15, an IPR fault occurs.
 */
int mopINSB(EISstruct *e)
{
    // If C(IF) = 9-15, an IPR fault occurs.
    if (e->mopIF >= 9 && e->mopIF <= 15)
    {
        e->_faults |= FAULT_IPR;
        return -1;
    }
    
    // If IF = 0, the 9 bits immediately following the INSB micro operation are treated as a 9-bit character (not a MOP) and are moved or skipped according to ES.
    if (e->mopIF == 0)
    {
        // If ES is OFF, then edit insertion table entry 1 is moved to the receiving field. If IF = 0, then the next 9 bits are also skipped. If IF is not 0, the next 9 bits are treated as a MOP.
        if (!e->mopES)
        {
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[0]);
     
            //get49(e, &e->mopAddr, &e->mopPos, CTN9);
            EISget49(e->mopAddress, &e->mopPos, CTN9);
            e->mopTally -= 1;
        } else {
           // If ES is ON and IF = 0, then the 9-bit character immediately following the INSB micro-instruction is moved to the receiving field.
            //writeToOutputBuffer(e, &e->out, 9, e->dstSZ, get49(e, &e->mopAddr, &e->mopPos, CTN9));
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, EISget49(e->mopAddress, &e->mopPos, CTN9));
            e->mopTally -= 1;            
        }
      
    } else {
      // If ES is ON and IF<>0, then IF specifies which edit insertion table entry (1-8) is to be moved to the receiving field.
        if (e->mopES)
        {
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[e->mopIF - 1]);
        }
    }
    return 0;
}

/*!
 * INSM Micro Operation - Insert Table Entry One Multiple
 * EXPLANATION:
 * IF specifies the number of receiving characters affected, where IF = 0 specifies 16 characters.
 * Edit insertion table entry 1 is moved to the next IF (1-16) receiving field characters.
 * FLAGS: None affected
 */
int mopINSM(EISstruct *e)
{
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[0]);
    }
    return 0;
}

/*!
 * INSN Micro Operation - Insert on Negative
 * EXPLANATION:
 * IF specifies which edit insertion table entry is inserted. If IF = 0, the 9 bits immediately following the INSN micro operation are treated as a 9-bit character (not a MOP) and are moved or skipped according to SN.
 * − If SN is OFF, then edit insertion table entry 1 is moved to the receiving field. If IF = 0, then the next 9 bits are also skipped. If IF is not 0, the next 9 bits are treated as a MOP.
 * − If SN is ON and IF = 0, then the 9-bit character immediately following the INSN micro-instruction is moved to the receiving field.
 * − If SN is ON and IF <> 0, then IF specifies which edit insertion table entry (1-8) is to be moved to the receiving field.
 * FLAGS: None affected
 * NOTE: If C(IF) = 9-15, an IPR fault occurs.
 */
int mopINSN(EISstruct *e)
{
    // If C(IF) = 9-15, an IPR fault occurs.
    if (e->mopIF >= 9 && e->mopIF <= 15)
    {
        e->_faults |= FAULT_IPR;
        return -1;
    }
    
    // If IF = 0, the 9 bits immediately following the INSN micro operation are treated as a 9-bit character (not a MOP) and are moved or skipped according to SN.
    
    if (e->mopIF == 0)
    {
        if (!e->mopSN)
        {
            //If SN is OFF, then edit insertion table entry 1 is moved to the receiving field. If IF = 0, then the next 9 bits are also skipped. If IF is not 0, the next 9 bits are treated as a MOP.
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[0]);
        } else {
            // If SN is ON and IF = 0, then the 9-bit character immediately following the INSN micro-instruction is moved to the receiving field.
            //writeToOutputBuffer(e, &e->out, 9, e->dstSZ, get49(e, &e->mopAddr, &e->mopPos, CTN9));
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, EISget49(e->mopAddress, &e->mopPos, CTN9));

            e->mopTally -= 1;   // I think
        }
    }
    else
    {
        if (e->mopSN)
        {
            //If SN is ON and IF <> 0, then IF specifies which edit insertion table entry (1-8) is to be moved to the receiving field.
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[e->mopIF - 1]);
        }
    }
    return 0;
}

/*!
 * INSP Micro Operation - Insert on Positive
 * EXPLANATION:
 * INSP is the same as INSN except that the responses for the SN values are reversed.
 * FLAGS: None affected
 * NOTE: If C(IF) = 9-15, an IPR fault occurs.
 */
int mopINSP(EISstruct *e)
{
    // If C(IF) = 9-15, an IPR fault occurs.
    if (e->mopIF >= 9 && e->mopIF <= 15)
    {
        e->_faults |= FAULT_IPR;
        return -1;
    }
    
    if (e->mopIF == 0)
    {
        if (e->mopSN)
        {
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[0]);
        } else {
            //writeToOutputBuffer(e, &e->out, 9, e->dstSZ, get49(e, &e->mopAddr, &e->mopPos, CTN9));
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, EISget49(e->mopAddress, &e->mopPos, CTN9));
            e->mopTally -= 1;
        }
    }
    else
    {
        if (!e->mopSN)
        {
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[e->mopIF - 1]);
        }
    }

    return 0;
}

/*!
 * LTE Micro Operation - Load Table Entry
 * EXPLANATION:
 * IF specifies the edit insertion table entry to be replaced.
 * The edit insertion table entry specified by IF is replaced by the 9-bit character immediately following the LTE microinstruction.
 * FLAGS: None affected
 * NOTE: If C(IF) = 0 or C(IF) = 9-15, an Illegal Procedure fault occurs.
 */
int mopLTE(EISstruct *e)
{
    if (e->mopIF == 0 || (e->mopIF >= 9 && e->mopIF <= 15))
    {
        e->_faults |= FAULT_IPR;
        return -1;
    }
    //word9 next = get49(e, &e->mopAddr, &e->mopPos, CTN9);
    word9 next = EISget49(e->mopAddress, &e->mopPos, CTN9);
    e->mopTally -= 1;
    
    e->editInsertionTable[e->mopIF - 1] = next;
    
    return 0;
}

/*!
 * MFLC Micro Operation - Move with Floating Currency Symbol Insertion
 * EXPLANATION:
 * IF specifies the number of characters of the sending field upon which the operation is performed, where IF = 0 specifies 16 characters.
 * Starting with the next available sending field character, the next IF characters are individually fetched and the following conditional actions occur.
 * − If ES is OFF and the character is zero, edit insertion table entry 1 is moved to the receiving field in place of the character.
 * − If ES is OFF and the character is not zero, then edit insertion table entry 5 is moved to the receiving field, the character is also moved to the receiving field, and ES is set ON.
 * − If ES is ON, the character is moved to the receiving field.
 * The number of characters placed in the receiving field is data-dependent. If the entire sending field is zero, IF characters are placed in the receiving field. However, if the sending field contains a nonzero character, IF+1 characters (the insertion character plus the characters from the sending field) are placed in the receiving field.
 * An IPR fault occurs when the sending field is exhausted before the receiving field is filled. In order to provide space in the receiving field for an inserted currency symbol, the receiving field must have a string length one character longer than the sending field. When the sending field is all zeros, no currency symbol is inserted by the MFLC micro operation and the receiving field is not filled when the sending field is exhausted. The user should provide an ENF (ENF,12) micro operation after a MFLC micro operation that has as its character count the number of characters in the sending field. The ENF micro operation is engaged only when the MFLC micro operation fails to fill the receiving field. Then it supplies a currency symbol to fill the receiving field and blanks out the entire field.
 * FLAGS: (Flags not listed are not affected.)
 * ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise, it is
 unchanged.
 * NOTE: Since the number of characters moved to the receiving string is data-dependent, a possible IPR fault may be avoided by ensuring that the Z and BZ flags are ON.
 */
int mopMFLC(EISstruct *e)
{
    if (e->mopIF == 0)
        e->mopIF = 16;

    //  Starting with the next available sending field character, the next IF characters are individually fetched and the following conditional actions occur.
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->srcTally == 0 || e->dstTally == 0)
        {
            e->_faults |= FAULT_IPR;
            return -1;
        }
        // If ES is OFF and the character is zero, edit insertion table entry 1 is moved to the receiving field in place of the character.
        // If ES is OFF and the character is not zero, then edit insertion table entry 5 is moved to the receiving field, the character is also moved to the receiving field, and ES is set ON.
        
        int c = *(e->in);
        if (!e->mopES) { // e->mopES is OFF
            if (c == 0) {
                // edit insertion table entry 1 is moved to the receiving field in place of the character.
                writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[0]);
                e->in += 1;
                e->srcTally -= 1;
            } else {
                // then edit insertion table entry 5 is moved to the receiving field, the character is also moved to the receiving field, and ES is set ON.
                writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[4]);
                
                e->in += 1;
                e->srcTally -= 1;
                if (e->srcTally == 0 || e->dstTally == 0)
                {
                    e->_faults |= FAULT_IPR;
                    return -1;
                }
                
                writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, c);
                
                e->mopES = true;
            }
        } else {
            // If ES is ON, the character is moved to the receiving field.
            writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, c);
            
            e->in += 1;
            e->srcTally -= 1;
        }
    }
    // ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise, it is unchanged.
    // NOTE: Since the number of characters moved to the receiving string is data-dependent, a possible IPR fault may be avoided by ensuring that the Z and BZ flags are ON.
    // XXX Not certain how to interpret either one of these!
    
    return 0;
}

/*!
 * MFLS Micro Operation - Move with Floating Sign Insertion
 * EXPLANATION:
 * IF specifies the number of characters of the sending field upon which the operation is performed, where IF = 0 specifies 16 characters.
 * Starting with the next available sending field character, the next IF characters are individually fetched and the following conditional actions occur.
 * − If ES is OFF and the character is zero, edit insertion table entry 1 is moved to the receiving field in place of the character.
 * − If ES is OFF, the character is not zero, and SN is OFF; then edit insertion table entry 3 is moved to the receiving field; the character is also moved to the receiving field, and ES is set ON.
 * − If ES is OFF, the character is nonzero, and SN is ON; edit insertion table entry 4 is moved to the receiving field; the character is also moved to the receiving field, and ES is set ON.
 * − If ES is ON, the character is moved to the receiving field.
 * The number of characters placed in the receiving field is data-dependent. If the entire sending field is zero, IF characters are placed in the receiving field. However, if the sending field contains a nonzero character, IF+1 characters (the insertion character plus the characters from the sending field) are placed in the receiving field.
 * An IPR fault occurs when the sending field is exhausted before the receiving field is filled. In order to provide space in the receiving field for an inserted sign, the receiving field must have a string length one character longer than the sending field. When the sending field is all zeros, no sign is inserted by the MFLS micro operation and the receiving field is not filled when the sending field is exhausted. The user should provide an ENF (ENF,4) micro operation after a MFLS micro operation that has as its character count the number of characters in the sending field. The ENF micro operation is engaged only when the MFLS micro operation fails to fill the receiving field; then, it supplies a sign character to fill the receiving field and blanks out the entire field.
 *
 * FLAGS: (Flags not listed are not affected.)
 *     ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise, it is unchanged.
 * NOTE: Since the number of characters moved to the receiving string is data-dependent, a possible Illegal Procedure fault may be avoided by ensuring that the Z and BZ flags are ON.
 */
int mopMFLS(EISstruct *e)
{
    if (e->mopIF == 0)
        e->mopIF = 16;
    
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->srcTally == 0 || e->dstTally == 0)
        {
            e->_faults = FAULT_IPR;
            return -1;
        }
        
        int c = *(e->in);

        if (!e->mopES) { // e->mopES is OFF
            if (c == 0) {
                // edit insertion table entry 1 is moved to the receiving field in place of the character.
                writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[0]);
                e->in += 1;
                e->srcTally -= 1;
            } else {
                // c is non-zero
                if (!e->mopSN)
                {
                    // then edit insertion table entry 3 is moved to the receiving field; the character is also moved to the receiving field, and ES is set ON.
                    writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[2]);

                    e->in += 1;
                    e->srcTally -= 1;
                    if (e->srcTally == 0 || e->dstTally == 0)
                    {
                        e->_faults |= FAULT_IPR;
                        return -1;
                    }
                    
                    writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, c);

                    e->mopES = true;
                } else {
                    //  SN is ON; edit insertion table entry 4 is moved to the receiving field; the character is also moved to the receiving field, and ES is set ON.
                    writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[3]);
                    
                    e->in += 1;
                    e->srcTally -= 1;
                    if (e->srcTally == 0 || e->dstTally == 0)
                    {
                        e->_faults |= FAULT_IPR;
                        return -1;
                    }
                    
                    writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, c);
                    
                    e->mopES = true;
                }
            }
        } else {
            // If ES is ON, the character is moved to the receiving field.
            writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, c);
            
            e->in += 1;
            e->srcTally -= 1;
        }
    }
    
    // ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise, it is unchanged.
    // NOTE: Since the number of characters moved to the receiving string is data-dependent, a possible Illegal Procedure fault may be avoided by ensuring that the Z and BZ flags are ON.

    // XXX Have no idea how to interpret either one of these statements.
    
    return 0;
}

/*!
 * MORS Micro Operation - Move and OR Sign
 * EXPLANATION:
 * IF specifies the number of characters of the sending field upon which the operation is performed, where IF = 0 specifies 16 characters.
 * Starting with the next available sending field character, the next IF characters are individually fetched and the following conditional actions occur.
 * − If SN is OFF, the next IF characters in the source data field are moved to the receiving data field and, during the move, edit insertion table entry 3 is ORed to each character.
 * − If SN is ON, the next IF characters in the source data field are moved to the receiving data field and, during the move, edit insertion table entry 4 is ORed to each character.
 * MORS can be used to generate a negative overpunch for a receiving field to be used later as a sending field.
 * FLAGS: None affected
 */
int mopMORS(EISstruct *e)
{
    if (e->mopIF == 0)
        e->mopIF = 16;
    
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->srcTally == 0 || e->dstTally == 0)
        {
            e->_faults |= FAULT_IPR;
            return -1;
        }
        
        // XXX this is probably wrong regarding the ORing, but it's a start ....
        int c = (*e->in | (!e->mopSN ? e->editInsertionTable[2] : e->editInsertionTable[3]));
        e->in += 1;
        e->srcTally -= 1;
        
        writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, c);
    }

    return 0;
}

/*!
 * MSES Micro Operation - Move and Set Sign
 * EXPLANATION:
 * IF specifies the number of characters of the sending field upon which the operation is performed, where IF = 0 specifies 16 characters. For MVE, starting with the next available sending field character, the next IF characters are individually fetched and the following conditional actions occur.
 * Starting with the first character during the move, a comparative AND is made first with edit insertion table entry 3. If the result is nonzero, the first character and the rest of the characters are moved without further comparative ANDs. If the result is zero, a comparative AND is made between the character being moved and edit insertion table entry 4 If that result is nonzero, the SN indicator is set ON (indicating negative) and the first character and the rest of the characters are moved without further comparative ANDs. If the result is zero, the second character is treated like the first. This process continues until one of the comparative AND results is nonzero or until all characters are moved.
 * For MVNE instruction, the sign (SN) flag is already set and IF characters are moved to the destination field (MSES is equivalent to the MVC instruction).
 * FLAGS: (Flags not listed are not affected.)
 * SN If edit insertion table entry 4 is found in C(Y-1), then ON; otherwise, it is unchanged.
 */
int mopMSES(EISstruct *e)
{
    if (e->mvne == true)
        return mopMVC(e);   // XXX I think!
        
        
    if (e->mopIF == 0)
        e->mopIF = 16;
    
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->srcTally == 0 || e->dstTally == 0)
        {
            e->_faults |= FAULT_IPR;
            return -1;
        }
        
        //Starting with the first character during the move, a comparative AND is made first with edit insertion table entry 3. If the result is nonzero, the first character and the rest of the characters are moved without further comparative ANDs. If the result is zero, a comparative AND is made between the character being moved and edit insertion table entry 4 If that result is nonzero, the SN indicator is set ON (indicating negative) and the first character and the rest of the characters are moved without further comparative ANDs. If the result is zero, the second character is treated like the first. This process continues until one of the comparative AND results is nonzero or until all characters are moved.
        
        int c = *(e->in);

        // a comparative AND is made first with edit insertion table entry 3.
        int cmpAnd = (c & e->editInsertionTable[2]);  // only lower 4-bits are considered
        //If the result is nonzero, the first character and the rest of the characters are moved without further comparative ANDs.
        if (cmpAnd)
        {
            for(int n2 = n ; n2 < e->mopIF ; n2 += 1)
            {
                if (e->srcTally == 0 || e->dstTally == 0)
                {
                    e->_faults |= FAULT_IPR;
                    return -1;
                }
                writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, *e->in);
                e->in += 1;
            
                e->srcTally -= 1;
            }
            return 0;
        }
        
        //If the result is zero, a comparative AND is made between the character being moved and edit insertion table entry 4 If that result is nonzero, the SN indicator is set ON (indicating negative) and the first character and the rest of the characters are moved without further comparative ANDs.
        
        cmpAnd = (c & e->editInsertionTable[3]);  // XXX only lower 4-bits are considered
        if (cmpAnd)
        {
            e->mopSN = true;
            for(int n2 = n ; n2 < e->mopIF ; n2 += 1)
            {
                if (e->srcTally == 0 || e->dstTally == 0)
                {
                    e->_faults |= FAULT_IPR;
                    return -1;
                }
                writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, *e->in);

                e->in += 1;
                e->srcTally -= 1;
            }
            return 0;
        }
        //If the result is zero, the second character is treated like the first. This process continues until one of the comparative AND results is nonzero or until all characters are moved.
        e->in += 1;
        e->srcTally -= 1;   // XXX is this correct? No chars have been consumed, but ......
    }
    
    return 0;
}

/*!
 * MVC Micro Operation - Move Source Characters
 * EXPLANATION:
 * IF specifies the number of characters to be moved, where IF = 0 specifies 16 characters.
 * The next IF characters in the source data field are moved to the receiving data field.
 * FLAGS: None affected
 */
int mopMVC(EISstruct *e)
{
    if (e->mopIF == 0)
        e->mopIF = 16;
    
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->srcTally == 0 || e->dstTally == 0)
        {
            e->_faults |= FAULT_IPR;
            return -1;
        }
        
        writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, *e->in);
        e->in += 1;
        
        e->srcTally -= 1;
    }
    
    return 0;
}

/*!
 * MVZA Micro Operation - Move with Zero Suppression and Asterisk Replacement
 * EXPLANATION:
 * MVZA is the same as MVZB except that if ES is OFF and the character is zero, then edit insertion table entry 2 is moved to the receiving field.
 * FLAGS: (Flags not listed are not affected.)
 * ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise, it is unchanged.
 */
int mopMVZA(EISstruct *e)
{
    if (e->mopIF == 0)
        e->mopIF = 16;
    
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->srcTally == 0 || e->dstTally == 0)
        {
            e->_faults |= FAULT_IPR;
            return -1;
        }
        
        int c = *e->in;
        e->in += 1;
        e->srcTally -= 1;
        
        if (!e->mopES && c == 0)
        {
            //If ES is OFF and the character is zero, then edit insertion table entry 2 is moved to the receiving field in place of the character.
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[1]);
        } else if (!e->mopES && c != 0)
        {
            //If ES is OFF and the character is not zero, then the character is moved to the receiving field and ES is set ON.
            writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, c);
            
            e->mopES = true;
        } else if (e->mopES)
        {
            //If ES is ON, the character is moved to the receiving field.
            writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, c);
        }
    }
    
    // XXX have no idea how to interpret this
    // ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise, it is unchanged.

    return 0;
}

/*!
 * MVZB Micro Operation - Move with Zero Suppression and Blank Replacement
 * EXPLANATION:
 * IF specifies the number of characters of the sending field upon which the operation is performed, where IF = 0 specifies 16 characters.
 * Starting with the next available sending field character, the next IF characters are individually fetched and the following conditional actions occur.
 * − If ES is OFF and the character is zero, then edit insertion table entry 1 is moved to the receiving field in place of the character.
 * − If ES is OFF and the character is not zero, then the character is moved to the receiving field and ES is set ON.
 * − If ES is ON, the character is moved to the receiving field. 
 * FLAGS: (Flags not listed are not affected.)
 *   ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise, it is unchanged.
 */
int mopMVZB(EISstruct *e)
{
    if (e->mopIF == 0)
        e->mopIF = 16;
    
    for(int n = 0 ; n < e->mopIF ; n += 1)
    {
        if (e->srcTally == 0 || e->dstTally == 0)
        {
            e->_faults |= FAULT_IPR;
            return -1;
        }
        
        int c = *e->in;
        e->srcTally -= 1;
        e->in += 1;
        
        if (!e->mopES && c == 0)
        {
            //If ES is OFF and the character is zero, then edit insertion table entry 1 is moved to the receiving field in place of the character.
            writeToOutputBuffer(e, &e->out, 9, e->dstSZ, e->editInsertionTable[0]);
        } else if (!e->mopES && c != 0)
        {
            //If ES is OFF and the character is not zero, then the character is moved to the receiving field and ES is set ON.
            writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, c);
            
            e->mopES = true;
        } else if (e->mopES)
        {
            //If ES is ON, the character is moved to the receiving field.
            writeToOutputBuffer(e, &e->out, e->srcSZ, e->dstSZ, c);
        }
    }

    // XXX have no idea how to interpret this......
    // ES If OFF and any of C(Y) is less than decimal zero, then ON; otherwise, it is unchanged.
    return 0;
}

/*!
 * SES Micro Operation - Set End Suppression
 * EXPLANATION:
 * Bit 0 of IF (IF(0)) specifies the setting of the ES switch.
 * If IF(0) = 0, the ES flag is set OFF. If IF(0) = 1, the ES flag is set ON.
 * Bit 1 of IF (IF(1)) specifies the setting of the blank-when-zero option.
 * If IF(1) = 0, no action is taken.
 * If IF(1) = 1, the BZ flag is set ON.
 * FLAGS: (Flags not listed are not affected.)
 * ES set by this micro operation
 * BZ If bit 1 of C(IF) = 1, then ON; otherwise, it is unchanged.
 */
int mopSES(EISstruct *e)
{
    if (e->mopIF & 010)
        e->mopES = true;
    else
        e->mopES = false;
    
    if (e->mopIF & 04)
        e->mopBZ = true;
    
    return 0;
}

/*!
 * fetch MOP from e->mopAddr/e->mopCN ...
 */
MOPstruct* EISgetMop(EISstruct *e)
{
    //static word18 lastAddress;  // try to keep memory access' down
    //static word36 data;
    
    
    if (e == NULL)
    //{
    //    p->lastAddress = -1;
    //    p->data = 0;
        return NULL;
    //}
   
    EISaddr *p = e->mopAddress;
    
    //if (p->lastAddress != p->address)                 // read from memory if different address
        p->data = EISRead(p);   // read data word from memory
    
    if (e->mopPos > 3)   // overflows to next word?
    {   // yep....
        e->mopPos = 0;   // reset to 1st byte
        e->mopAddress->address = (e->mopAddress->address + 1) & AMASK;     // bump source to next address
        p->data = EISRead(e->mopAddress);   // read it from memory
    }
    
    e->mop9  = (word9)get9(p->data, e->mopPos);       // get 9-bit mop
    e->mop   = (e->mop9 >> 4) & 037;
    e->mopIF = e->mop9 & 0xf;
    
    MOPstruct *m = &mopTab[e->mop];
    e->m = m;
    if (e->m == NULL || e->m->f == NULL)
    {
        fprintf(stderr, "getMop(e->m == NULL || e->m->f == NULL): mop:%d IF:%d\n", e->mop, e->mopIF);
        return NULL;
    }
    
    e->mopPos += 1;
    e->mopTally -= 1;
    
    //p->lastAddress = p->address;
    
    return m;
}

/*!
 * This is the Micro Operation Executor/Interpreter
 */
PRIVATE
void mopExecutor(EISstruct *e, int kMop)
{
    //e->mopAddr = e->YChar9[kMop-1];    // get address of microoperations
    e->mopAddress = &e->addr[kMop-1];
    e->mopTally = e->N[kMop-1];        // number of micro-ops
    e->mopCN   = e->CN[kMop-1];        // starting at char pos CN
    e->mopPos  = e->mopCN;
    
    word9 *p9 = e->editInsertionTable; // re-initilize edit insertion table
    char *q = defaultEditInsertionTable;
    while((*p9++ = *q++))
        ;
    
    e->in = e->inBuffer;    // reset input buffer pointer
    e->out = e->outBuffer;  // reset output buffer pointer
    e->outPos = 0;
    
    e->_faults = 0; // No faults (yet!)
    
    //getMop(NULL);   // initialize mop getter
    EISgetMop(NULL);   // initialize mop getter
    //get49(NULL, 0, 0, 0); // initialize byte getter

    // execute dstTally micro operations
    // The micro operation sequence is terminated normally when the receiving string length becomes exhausted. The micro operation sequence is terminated abnormally (with an illegal procedure fault) if a move from an exhausted sending string or the use of an exhausted MOP string is attempted.
    
    //fprintf(stderr, "(I) mopTally=%d srcTally=%d\n", e->mopTally, e->srcTally);

    //while (e->mopTally && e->srcTally && e->dstTally)
    while (e->dstTally && e->mopTally)
    {
        //MOPstruct *m = getMop(e);
        MOPstruct *m = EISgetMop(e);
        
        int mres = m->f(e);    // execute mop
        if (mres)
            break;        
    }
    
    // XXX this stuff should probably best be done in the mop's themselves. We'll see.
    if (e->dstTally == 0)  // normal termination
        return;
   
    // mop string exhausted?
    if (e->mopTally != 0)
        e->_faults |= FAULT_IPR;   // XXX ill proc fault
    
    if (e->srcTally != 0)  // sending string exhausted?
        e->_faults |= FAULT_IPR;   // XXX ill proc fault
}

void mvne(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    setupOperandDescriptor(3, e);
    
    parseNumericOperandDescriptor(1, e);
    parseAlphanumericOperandDescriptor(2, e);
    parseAlphanumericOperandDescriptor(3, e);
    
    // initialize mop flags. Probably best done elsewhere.
    e->mopES = false; // End Suppression flag
    e->mopSN = false; // Sign flag
    e->mopZ  = true;  // Zero flag
    e->mopBZ = false; // Blank-when-zero flag
    
    e->srcTally = e->N1;  // number of chars in src (max 63)
    e->dstTally = e->N3;  // number of chars in dst (max 63)
    
    e->srcTN = e->TN1;    // type of chars in src
    e->srcCN = e->CN1;    // starting at char pos CN
    switch(e->srcTN)
    {
        case CTN4:
            //e->srcAddr = e->YChar41;
            e->srcSZ = 4;   // stored as 4-bit decimals
            break;
        case CTN9:
            //e->srcAddr = e->YChar91;
            e->srcSZ = 4;   // 'cause everything is stored as 4-bit decimals
            break;
    }

    e->dstTA = e->TA3;    // type of chars in dst
    e->dstCN = e->CN3;    // starting at char pos CN
    switch(e->dstTA)
    {
        case CTA4:
            //e->dstAddr = e->YChar43;
            e->dstSZ = 4;
            break;
        case CTA6:
            //e->dstAddr = e->YChar63;
            e->dstSZ = 6;
            break;
        case CTA9:
            //e->dstAddr = e->YChar93;
            e->dstSZ = 9;
            break;
    }
    
    // 1. load sending string into inputBuffer
    EISloadInputBufferNumeric(ins, 1);   // according to MF1
    
    // 2. Test sign and, if required, set the SN flag. (Sign flag; initially set OFF if the sending string has an alphanumeric descriptor or an unsigned numeric descriptor. If the sending string has a signed numeric descriptor, the sign is initially read from the sending string from the digit position defined by the sign and the decimal type field (S); SN is set OFF if positive, ON if negative. If all digits are zero, the data is assumed positive and the SN flag is set OFF, even when the sign is negative.)

    int sum = 0;
    for(int n = 0 ; n < e->srcTally ; n += 1)
        sum += e->inBuffer[n];
    if ((e->sign == -1) && sum)
        e->mopSN = true;
    
    // 3. Execute micro operation string, starting with first (4-bit) digit.
    e->mvne = true;
    
    mopExecutor(e, 2);

    e->dstTally = e->N3;  // restore dstTally for output
    
    EISwriteOutputBufferToMemory(&e->ADDR3);
}

void mve(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    setupOperandDescriptor(3, e);
    
    parseAlphanumericOperandDescriptor(1, e);
    parseAlphanumericOperandDescriptor(2, e);
    parseAlphanumericOperandDescriptor(3, e);
    
    // initialize mop flags. Probably best done elsewhere.
    e->mopES = false; // End Suppression flag
    e->mopSN = false; // Sign flag
    e->mopZ  = true;  // Zero flag
    e->mopBZ = false; // Blank-when-zero flag
    
    e->srcTally = e->N1;  // number of chars in src (max 63)
    e->dstTally = e->N3;  // number of chars in dst (max 63)
    
    e->srcTA = e->TA1;    // type of chars in src
    e->srcCN = e->CN1;    // starting at char pos CN
    switch(e->srcTA)
    {
        case CTA4:
            //e->srcAddr = e->YChar41;
            e->srcSZ = 4;
            break;
        case CTA6:
            //e->srcAddr = e->YChar61;
            e->srcSZ = 6;
            break;
        case CTA9:
            //e->srcAddr = e->YChar91;
            e->srcSZ = 9;
            break;
    }
    
    e->dstTA = e->TA3;    // type of chars in dst
    e->dstCN = e->CN3;    // starting at char pos CN
    switch(e->dstTA)
    {
        case CTA4:
            //e->dstAddr = e->YChar43;
            e->dstSZ = 4;
            break;
        case CTA6:
            //e->dstAddr = e->YChar63;
            e->dstSZ = 6;
            break;
        case CTA9:
            //e->dstAddr = e->YChar93;
            e->dstSZ = 9;
            break;
    }
    
    // 1. load sending string into inputBuffer
    EISloadInputBufferAlphnumeric(e, 1);   // according to MF1
    
    // 2. Execute micro operation string, starting with first (4-bit) digit.
    e->mvne = false;
    
    mopExecutor(e, 2);
    
    e->dstTally = e->N3;  // restore dstTally for output
    
    EISwriteOutputBufferToMemory(&e->ADDR3);
}

/*!
 * does 6-bit char represent a GEBCD negative overpuch? if so, whice numeral?
 * Refer to Bull NovaScale 9000 RJ78 Rev2 p11-178
 */
PRIVATE
bool isOvp(int c, int *on)
{
    // look for GEBCD -' 'A B C D E F G H I (positive overpunch)
    // look for GEBCD - ^ J K L M N O P Q R (negative overpunch)
    
    int c2 = c & 077;   // keep to 6-bits
    *on = 0;
    
    if (c2 >= 020 && c2 <= 031)            // positive overpunch
    {
        *on = c2 - 020;                    // return GEBCD number 0-9 (020 is +0)
        return false;                      // well, it's not a negative overpunch is it?
    }
    if (c2 >= 040 && c2 <= 052)            // negative overpunch
    {
        *on = c2 - 040;  // return GEBCD number 0-9 (052 is just a '-' interpreted as -0)
        return true;
    }
    return false;
}

/*!
 * MLR - Move Alphanumeric Left to Right
 *
 * (Nice, simple instruction if it weren't for the stupid overpunch stuff that ruined it!!!!)
 */
void mlr(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For i = 1, 2, ..., minimum (N1,N2)
    //     C(Y-charn1)N1-i → C(Y-charn2)N2-i
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    C(FILL) → C(Y-charn2)N2-i
    // Indicators: Truncation. If N1 > N2 then ON; otherwise OFF
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseAlphanumericOperandDescriptor(1, e);
    parseAlphanumericOperandDescriptor(2, e);
    
    e->srcCN = e->CN1;    // starting at char pos CN
    e->dstCN = e->CN2;    // starting at char pos CN
    
    switch(e->TA1)
    {
        case CTA4:
            //e->srcAddr = e->YChar41;
            e->srcSZ = 4;
            break;
        case CTA6:
            //e->srcAddr = e->YChar61;
            e->srcSZ = 6;
            break;
        case CTA9:
            //e->srcAddr = e->YChar91;
            e->srcSZ = 9;
            break;
    }
    
    switch(e->TA2)
    {
        case CTA4:
            //e->dstAddr = e->YChar42;
            e->dstSZ = 4;
            break;
        case CTA6:
            //e->dstAddr = e->YChar62;
            e->dstSZ = 6;
            break;
        case CTA9:
            //e->dstAddr = e->YChar92;
            e->dstSZ = 9;
            break;
    }
    
    e->T = bitfieldExtract36(e->op0, 26, 1);  // truncation bit
    
    int fill = (int)bitfieldExtract36(e->op0, 27, 9);
    int fillT = fill;  // possibly truncated fill pattern
    // play with fill if we need to use it
    switch(e->dstSZ)
    {
        case 4:
            fillT = fill & 017;    // truncate upper 5-bits
            break;
        case 6:
            fillT = fill & 077;    // truncate upper 3-bits
            break;
    }
    
    // If N1 > N2, then (N1-N2) leading characters of C(Y-charn1) are not moved and the truncation indicator is set ON.
    // If N1 < N2 and TA2 = 2 (4-bit data) or 1 (6-bit data), then FILL characters are high-order truncated as they are moved to C(Y-charn2). No character conversion takes place.
    //The user of string replication or overlaying is warned that the decimal unit addresses the main memory in unaligned (not on modulo 8 boundary) units of Y-block8 words and that the overlayed string, C(Y-charn2), is not returned to main memory until the unit of Y-block8 words is filled or the instruction completes.
    //If T = 1 and the truncation indicator is set ON by execution of the instruction, then a truncation (overflow) fault occurs.
    //Attempted execution with the xed instruction causes an illegal procedure fault.
    //Attempted repetition with the rpt, rpd, or rpl instructions causes an illegal procedure fault.
    
    /// XXX when do we do a truncation fault?
    
    SCF(e->N1 > e->N2, rIR, I_TRUNC);
    
    bool ovp = (e->N1 < e->N2) && (fill & 0400) && (e->TA1 == 1) && (e->TA2 == 2); // (6-4 move)
    int on;     // number overpunch represents (if any)
    bool bOvp = false;  // true when a negative overpunch character has been found @ N1-1 

    //get469(NULL, 0, 0, 0);    // initialize char getter buffer
    
    for(int i = 0 ; i < min(e->N1, e->N2); i += 1)
    {
        //int c = get469(e, &e->srcAddr, &e->srcCN, e->TA1); // get src char
        int c = EISget469(&e->ADDR1, &e->srcCN, e->TA1); // get src char
        int cout = 0;
        
        if (e->TA1 == e->TA2) 
            //write469(e, &e->dstAddr, &e->dstCN, e->TA1, c);
            EISwrite469(&e->ADDR2, &e->dstCN, e->TA1, c);
        else
        {
            // If data types are dissimilar (TA1 ≠ TA2), each character is high-order truncated or zero filled, as appropriate, as it is moved. No character conversion takes place.
            cout = c;
            switch (e->srcSZ)
            {
                case 6:
                    switch(e->dstSZ)
                    {
                        case 4:
                            cout = c & 017;    // truncate upper 2-bits
                            break;
                        case 9:
                            break;              // should already be 0-filled
                    }
                    break;
                case 9:
                    switch(e->dstSZ)
                    {
                        case 4:
                            cout = c & 017;    // truncate upper 5-bits
                            break;
                        case 6:
                            cout = c & 077;    // truncate upper 3-bits
                            break;
                    }
                    break;
            }

            // If N1 < N2, C(FILL)0 = 1, TA1 = 1, and TA2 = 2 (6-4 move), then C(Y-charn1)N1-1 is examined for a GBCD overpunch sign. If a negative overpunch sign is found, then the minus sign character is placed in C(Y-charn2)N2-1; otherwise, a plus sign character is placed in C(Y-charn2)N2-1.
            
            if (ovp && (i == e->N1-1))
            {
                // this is kind of wierd. I guess that C(FILL)0 = 1 means that there *is* an overpunch char here.
                bOvp = isOvp(c, &on);
                cout = on;      // replace char with the digit the overpunch represents
            }
            //write469(e, &e->dstAddr, &e->dstCN, e->TA2, cout);
            EISwrite469(&e->ADDR2, &e->dstCN, e->TA2, cout);
        }
    }
    
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    C(FILL) → C(Y-charn2)N2-i
    // If N1 < N2 and TA2 = 2 (4-bit data) or 1 (6-bit data), then FILL characters are high-order truncated as they are moved to C(Y-charn2). No character conversion takes place.

    if (e->N1 < e->N2)
    {
        for(int i = e->N1 ; i < e->N2 ; i += 1)
            if (ovp && (i == e->N2-1))    // if there's an overpunch then the sign will be the last of the fill
            {
                if (bOvp)   // is c an GEBCD negative overpunch? and of what?
                    //write469(e, &e->dstAddr, &e->dstCN, e->TA2, 015);  // 015 is decimal -
                    EISwrite469(&e->ADDR2, &e->dstCN, e->TA2, 015);  // 015 is decimal -
                else
                    //write469(e, &e->dstAddr, &e->dstCN, e->TA2, 014);  // 014 is decimal +
                    EISwrite469(&e->ADDR2, &e->dstCN, e->TA2, 014);  // 014 is decimal +
            }
            else
                //write469(e, &e->dstAddr, &e->dstCN, e->TA2, fillT);
                EISwrite469(&e->ADDR2, &e->dstCN, e->TA2, fillT);
    }
}

/*
 * return CN (char position) and word offset given:
 *  'n' # of chars/bytes/bits, etc ....
 *  'initPos' initial char position (CN)
 *  'ta' alphanumeric type (if ta < 0 then size = abs(ta))
 */
PRIVATE
void getOffsets(int n, int initCN, int ta, int *nWords, int *newCN)
{
    int maxPos = 0;
    
    int size = 0;
    if (ta < 0)
        size = -ta;
    else
        switch (ta)
        {
            case CTA4:
                size = 4;
                maxPos = 8;
                break;
            case CTA6:
                size = 6;
                maxPos = 6;
                break;
            case CTA9:
                size = 9;
                maxPos = 4;
                break;
        }
    
    int numBits = size * n;
    
    int numWords =  (numBits + (initCN * size)) / ((size == 4) ? 32 : 36);  ///< how many additional words will the N chars take up?
                                                                            ///< (remember there are 4 slop bits in a 36-bit word when dealing with BCD)
    int lastChar = (initCN + n - 1) % maxPos;       ///< last character number
    
    if (numWords > 1)          // more that the 1 word needed?
        *nWords = numWords-1;  // # of additional words
    else
        *nWords = 0;           // no additional words needed
    *newCN = lastChar;         // last character number
}

/*!
 * MRL - Move Alphanumeric Right to Left
 *
 * (Like MLR, nice, simple instruction if it weren't for the stupid overpunch stuff that ruined it!!!!)
 */
void mrl(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For i = 1, 2, ..., minimum (N1,N2)
    //   C(Y-charn1)N1-i → C(Y-charn2)N2-i
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //   C(FILL) → C(Y-charn2)N2-i
  
    // Indicators: Truncation. If N1 > N2 then ON; otherwise OFF
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseAlphanumericOperandDescriptor(1, e);
    parseAlphanumericOperandDescriptor(2, e);
    
    e->srcCN = e->CN1;    // starting at char pos CN
    e->dstCN = e->CN2;    // starting at char pos CN
    
    e->srcTA = e->TA1;
    e->dstTA = e->TA2;
    
    switch(e->TA1)
    {
        case CTA4:
            //e->srcAddr = e->YChar41;
            e->srcSZ = 4;
            break;
        case CTA6:
            //e->srcAddr = e->YChar61;
            e->srcSZ = 6;
            break;
        case CTA9:
            //e->srcAddr = e->YChar91;
            e->srcSZ = 9;
            break;
    }
    
    switch(e->TA2)
    {
        case CTA4:
            //e->dstAddr = e->YChar42;
            e->dstSZ = 4;
            break;
        case CTA6:
            //e->dstAddr = e->YChar62;
            e->dstSZ = 6;
            break;
        case CTA9:
            //e->dstAddr = e->YChar92;
            e->dstSZ = 9;
            break;
    }
    
    // adjust addresses & offsets for writing in reverse ....
    int nSrcWords = 0;
    int newSrcCN = 0;
    getOffsets(e->N1, e->srcCN, e->srcTA, &nSrcWords, &newSrcCN);

    int nDstWords = 0;
    int newDstCN = 0;
    getOffsets(e->N2, e->dstCN, e->dstTA, &nDstWords, &newDstCN);

    //word18 newSrcAddr = e->srcAddr + nSrcWords;
    //word18 newDstAddr = e->dstAddr + nDstWords;
    e->ADDR1.address += nSrcWords;
    e->ADDR2.address += nDstWords;
    
    e->T = bitfieldExtract36(e->op0, 26, 1);  // truncation bit
    
    int fill = (int)bitfieldExtract36(e->op0, 27, 9);
    int fillT = fill;  // possibly truncated fill pattern
    
    switch(e->dstSZ)
    {
        case 4:
            fillT = fill & 017;    // truncate upper 5-bits
            break;
        case 6:
            fillT = fill & 077;    // truncate upper 3-bits
            break;
    }
    
    // If N1 > N2, then (N1-N2) leading characters of C(Y-charn1) are not moved and the truncation indicator is set ON.
    // If N1 < N2 and TA2 = 2 (4-bit data) or 1 (6-bit data), then FILL characters are high-order truncated as they are moved to C(Y-charn2). No character conversion takes place.
    //The user of string replication or overlaying is warned that the decimal unit addresses the main memory in unaligned (not on modulo 8 boundary) units of Y-block8 words and that the overlayed string, C(Y-charn2), is not returned to main memory until the unit of Y-block8 words is filled or the instruction completes.
    //If T = 1 and the truncation indicator is set ON by execution of the instruction, then a truncation (overflow) fault occurs.
    //Attempted execution with the xed instruction causes an illegal procedure fault.
    //Attempted repetition with the rpt, rpd, or rpl instructions causes an illegal procedure fault.
    
    /// XXX when do we do a truncation fault?
    
    SCF(e->N1 > e->N2, rIR, I_TRUNC);
    
    bool ovp = (e->N1 < e->N2) && (fill & 0400) && (e->TA1 == 1) && (e->TA2 == 2); // (6-4 move)
    int on;     // number overpunch represents (if any)
    bool bOvp = false;  // true when a negative overpunch character has been found @ N1-1
   
    //get469r(NULL, 0, 0, 0);    // initialize char getter buffer
    
    for(int i = 0 ; i < min(e->N1, e->N2); i += 1)
    {
        //int c = get469r(e, &newSrcAddr, &newSrcCN, e->srcTA); // get src char
        int c = EISget469r(&e->ADDR1, &newSrcCN, e->srcTA); // get src char
        int cout = 0;
        
        if (e->TA1 == e->TA2)
            EISwrite469r(&e->ADDR2, &newDstCN, e->dstTA, c);
        else
        {
            // If data types are dissimilar (TA1 ≠ TA2), each character is high-order truncated or zero filled, as appropriate, as it is moved. No character conversion takes place.
            cout = c;
            switch (e->srcSZ)
            {
                case 6:
                    switch(e->dstSZ)
                    {
                        case 4:
                            cout = c & 017;    // truncate upper 2-bits
                            break;
                        case 9:
                            break;              // should already be 0-filled
                    }
                    break;
                case 9:
                    switch(e->dstSZ)
                    {
                        case 4:
                            cout = c & 017;    // truncate upper 5-bits
                            break;
                        case 6:
                            cout = c & 077;    // truncate upper 3-bits
                            break;
                    }
                    break;
            }
            
            // If N1 < N2, C(FILL)0 = 1, TA1 = 1, and TA2 = 2 (6-4 move), then C(Y-charn1)N1-1 is examined for a GBCD overpunch sign. If a negative overpunch sign is found, then the minus sign character is placed in C(Y-charn2)N2-1; otherwise, a plus sign character is placed in C(Y-charn2)N2-1.
            
            //if (ovp && (i == e->N1-1))
            if (ovp && (i == 0))    // since we're going backwards, we actually test the 1st char for overpunch
            {
                // this is kind of wierd. I guess that C(FILL)0 = 1 means that there *is* an overpunch char here.
                bOvp = isOvp(c, &on);
                cout = on;      // replace char with the digit the overpunch represents
            }
            EISwrite469r(&e->ADDR2, &newDstCN, e->dstTA, cout);
        }
    }
    
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    C(FILL) → C(Y-charn2)N2-i
    // If N1 < N2 and TA2 = 2 (4-bit data) or 1 (6-bit data), then FILL characters are high-order truncated as they are moved to C(Y-charn2). No character conversion takes place.
    
    if (e->N1 < e->N2)
    {
        for(int i = e->N1 ; i < e->N2 ; i += 1)
            if (ovp && (i == e->N2-1))    // if there's an overpunch then the sign will be the last of the fill
            {
                if (bOvp)   // is c an GEBCD negative overpunch? and of what?
                    EISwrite469r(&e->ADDR2, &newDstCN, e->dstTA, 015);  // 015 is decimal -
                else
                    EISwrite469r(&e->ADDR2, &newDstCN, e->dstTA, 014);  // 014 is decimal +
            }
            else
                EISwrite469r(&e->ADDR2, &newDstCN, e->dstTA, fillT);
    }
}

word8 xlate(word36 *xlatTbl, int dstTA, int c)
{
    int idx = (c / 4) & 0177;      // max 128-words (7-bit index)
    word36 entry = xlatTbl[idx];
    
    int pos9 = c % 4;      // lower 2-bits
    int cout = (int)GETBYTE(entry, pos9);
    //int cout = getByte(pos9, entry);
    switch(dstTA)
    {
        case CTA4:
            return cout & 017;
        case CTA6:
            return cout & 077;
        case CTA9:
            return cout;
    }

    return 0;
}

/*  
 * MVT - Move Alphanumeric with Translation
 */
void mvt(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For i = 1, 2, ..., minimum (N1,N2)
    //    m = C(Y-charn1)i-1
    //    C(Y-char93)m → C(Y-charn2)i-1
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    m = C(FILL)
    //    C(Y-char93)m → C(Y-charn2)i-1
    
    // Indicators: Truncation. If N1 > N2 then ON; otherwise OFF
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseAlphanumericOperandDescriptor(1, e);
    parseAlphanumericOperandDescriptor(2, e);
    
    e->srcCN = e->CN1;    // starting at char pos CN
    e->dstCN = e->CN2;    // starting at char pos CN
    
    e->srcTA = e->TA1;
    e->dstTA = e->TA2;
    
    switch(e->TA1)
    {
        case CTA4:
            //e->srcAddr = e->YChar41;
            e->srcSZ = 4;
            break;
        case CTA6:
            //e->srcAddr = e->YChar61;
            e->srcSZ = 6;
            break;
        case CTA9:
            //e->srcAddr = e->YChar91;
            e->srcSZ = 9;
            break;
    }
    
    switch(e->TA2)
    {
        case CTA4:
            //e->dstAddr = e->YChar42;
            e->dstSZ = 4;
            break;
        case CTA6:
            //e->dstAddr = e->YChar62;
            e->dstSZ = 6;
            break;
        case CTA9:
            //e->dstAddr = e->YChar92;
            e->dstSZ = 9;
            break;
    }
    
    word36 xlat = e->op[2];                         // 3rd word is a pointer to a translation table
    int xA = (int)bitfieldExtract36(xlat, 6, 1);    // 'A' bit - indirect via pointer register
    int xREG = xlat & 0xf;

    word18 r = (word18)getMFReg(xREG, true);

    word18 xAddress = GETHI(xlat);

    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    if (xA)
    {
        // if 3rd operand contains A (bit-29 set) then it Means Y-char93 is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(xAddress, 15, 3);
        int offset = xAddress & 077777;  // 15-bit signed number
        xAddress = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;
        
        if (get_addr_mode() == APPEND_mode)
        {
            //TPR.TSR = PR[n].SNR;
            //TPR.TRR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            e->ADDR3.SNR = PR[n].SNR;
            e->ADDR3.RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            
            e->ADDR3.mat = viaPR;
        }
    }
    
    xAddress +=  ((9*ARn_CHAR + 36*r + ARn_BITNO) / 36);
    xAddress &= AMASK;
    e->ADDR3.address = xAddress;
    
    // XXX I think this is where prepage mode comes in. Need to ensure that the translation table's page is im memory.
    // XXX handle, later. (Yeah, just like everything else hard.)
    //  Prepage Check in a Multiword Instruction
    //  The MVT, TCT, TCTR, and CMPCT instruction have a prepage check. The size of the translate table is determined by the TA1 data type as shown in the table below. Before the instruction is executed, a check is made for allocation in memory for the page for the translate table. If the page is not in memory, a Missing Page fault occurs before execution of the instruction. (Bull RJ78 p.7-75)
        
    // TA1              TRANSLATE TABLE SIZE
    // 4-BIT CHARACTER      4 WORDS
    // 6-BIT CHARACTER     16 WORDS
    // 9-BIT CHARACTER    128 WORDS
    
    int xlatSize = 0;   // size of xlation table in words .....
    switch(e->TA1)
    {
        case CTA4:
            xlatSize = 4;
            break;
        case CTA6:
            xlatSize = 16;
            break;
        case CTA9:
            xlatSize = 128;
            break;
    }
    
    word36 xlatTbl[128];
    memset(xlatTbl, 0, sizeof(xlatTbl));    // 0 it out just in case
    
    // XXX here is where we probably need to to the prepage thang...
    //ReadNnoalign(currentInstruction,  xlatSize, xAddress, xlatTbl, OperandRead, 0);
    EISReadN(&e->ADDR3, xlatSize, xlatTbl);
    
    int fill = (int)bitfieldExtract36(e->op0, 27, 9);
    int fillT = fill;  // possibly truncated fill pattern
    // play with fill if we need to use it
    switch(e->srcSZ)
    {
        case 4:
            fillT = fill & 017;    // truncate upper 5-bits
            break;
        case 6:
            fillT = fill & 077;    // truncate upper 3-bits
            break;
    }
    
    //int xlatAddr = 0;
    //int xlatCN = 0;

    /// XXX when do we do a truncation fault?
    
    SCF(e->N1 > e->N2, rIR, I_TRUNC);
    
    //get469(NULL, 0, 0, 0);    // initialize char getter buffer
    
    for(int i = 0 ; i < min(e->N1, e->N2); i += 1)
    {
        //int c = get469(e, &e->srcAddr, &e->srcCN, e->TA1); // get src char
        int c = EISget469(&e->ADDR1, &e->srcCN, e->TA1); // get src char
        int cidx = 0;
    
        if (e->TA1 == e->TA2)
            //write469(e, &e->dstAddr, &e->dstCN, e->TA1, xlate(xlatTbl, e->dstTA, c));
            EISwrite469(&e->ADDR2, &e->dstCN, e->TA1, xlate(xlatTbl, e->dstTA, c));
        else
        {
            // If data types are dissimilar (TA1 ≠ TA2), each character is high-order truncated or zero filled, as appropriate, as it is moved. No character conversion takes place.
            cidx = c;
            
            int cout = xlate(xlatTbl, e->dstTA, cidx);

//            switch(e->dstSZ)
//            {
//                case 4:
//                    cout &= 017;    // truncate upper 5-bits
//                    break;
//                case 6:
//                    cout &= 077;    // truncate upper 3-bits
//                    break;
//            }

            switch (e->srcSZ)
            {
                case 6:
                    switch(e->dstSZ)
                    {
                        case 4:
                            cout &= 017;    // truncate upper 2-bits
                            break;
                        case 9:
                            break;              // should already be 0-filled
                    }
                    break;
                case 9:
                    switch(e->dstSZ)
                    {
                        case 4:
                            cout &= 017;    // truncate upper 5-bits
                            break;
                        case 6:
                            cout &= 077;    // truncate upper 3-bits
                            break;
                    }
                    break;
            }
            
            //write469(e, &e->dstAddr, &e->dstCN, e->TA2, cout);
            EISwrite469(&e->ADDR2, &e->dstCN, e->TA2, cout);
        }
    }
    
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    C(FILL) → C(Y-charn2)N2-i
    // If N1 < N2 and TA2 = 2 (4-bit data) or 1 (6-bit data), then FILL characters are high-order truncated as they are moved to C(Y-charn2). No character conversion takes place.
    
    if (e->N1 < e->N2)
    {
        int cfill = xlate(xlatTbl, e->dstTA, fillT);
        switch (e->srcSZ)
        {
            case 6:
                switch(e->dstSZ)
                {
                    case 4:
                        cfill &= 017;    // truncate upper 2-bits
                        break;
                    case 9:
                        break;              // should already be 0-filled
                }
                break;
            case 9:
                switch(e->dstSZ)
                {
                    case 4:
                        cfill &= 017;    // truncate upper 5-bits
                        break;
                    case 6:
                        cfill &= 077;    // truncate upper 3-bits
                        break;
                }
                break;
        }
        
//        switch(e->dstSZ)
//        {
//            case 4:
//                cfill &= 017;    // truncate upper 5-bits
//                break;
//            case 6:
//                cfill &= 077;    // truncate upper 3-bits
//                break;
//        }
        
        for(int j = e->N1 ; j < e->N2 ; j += 1)
            //write469(e, &e->dstAddr, &e->dstCN, e->TA2, cfill);
            EISwrite469(&e->ADDR2, &e->dstCN, e->TA2, cfill);
    }
}

PRIVATE
word18 getMF2Reg(int n, word18 data)
{
    switch (n)
    {
        case 0: ///< n
            return 0;
        case 1: ///< au
            return GETHI(rA);
        case 2: ///< qu
            return GETHI(rQ);
        case 3: ///< du
            return GETHI(data);
        case 5: ///< al
            return GETLO(rA);
        case 6: ///< ql / a
            return GETLO(rQ);
        case 8:
        case 9:
        case 10:
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
            return rX[n - 8];
        default:
            // XXX: IPR generate Illegal Procedure Fault
            return 0;
    }
    fprintf(stderr, "getMF2Reg(): How'd we get here? n=%d\n", n);
    return 0;
}

/*
 * SCM - Scan with Mask
 */
void scm(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For characters i = 1, 2, ..., N1
    //   For bits j = 0, 1, ..., 8
    //      C(Z)j = ~C(MASK)j & ((C(Y-charn1)i-1 )j ⊕ (C(Y-charn2)0)j)
    //      If C(Z)0,8 = 00...0, then
    //           00...0 → C(Y3)0,11
    //           i-1 → C(Y3)12,35
    //      otherwise, continue scan of C(Y-charn1)
    // If a masked character match was not found, then
    //   00...0 → C(Y3)0,11
    //   N1 → C(Y3)12,35
    
    // Starting at location YC1, the L1 type TA1 characters are masked and compared with the assumed type TA1 character contained either in location YC2 or in bits 0-8 or 0-5 of the address field of operand descriptor 2 (when the REG field of MF2 specifies DU modification). The mask is right-justified in bit positions 0-8 of the instruction word. Each bit position of the mask that is a 1 prevents that bit position in the two characters from entering into the compare.
    // The masked compare operation continues until either a match is found or the tally (L1) is exhausted. For each unsuccessful match, a count is incremented by 1. When a match is found or when the L1 tally runs out, this count is stored right-justified in bits 12-35 of location Y3 and bits 0-11 of Y3 are zeroed. The contents of location YC2 and the source string remain unchanged. The RL bit of the MF2 field is not used.
    
    // The REG field of MF1 is checked for a legal code. If DU is specified in the REG field of MF2 in one of the four multiword instructions (SCD, SCDR, SCM, or SCMR) for which DU is legal, the CN field is ignored and the character or characters are arranged within the 18 bits of the word address portion of the operand descriptor.
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
  
    parseAlphanumericOperandDescriptor(1, e);
    parseAlphanumericOperandDescriptor(2, e);
    
    e->srcCN = e->CN1;  ///< starting at char pos CN
    e->srcCN2= e->CN2;  ///< character number
    
    //Both the string and the test character pair are treated as the data type given for the string, TA1. A data type given for the test character pair, TA2, is ignored.
    e->srcTA = e->TA1;
    e->srcTA2 = e->TA1;

    switch(e->TA1)
    {
        case CTA4:
            //e->srcAddr = e->YChar41;
            e->srcSZ = 4;
            break;
        case CTA6:
            //e->srcAddr = e->YChar61;
            e->srcSZ = 6;
            break;
        case CTA9:
            //e->srcAddr = e->YChar91;
            e->srcSZ = 9;
            break;
    }
    
    switch(e->TA1)      // assumed same type
    {
        case CTA4:
            //e->srcAddr2 = e->YChar42;
            break;
        case CTA6:
            //e->srcAddr2 = e->YChar62;
            break;
        case CTA9:
            //e->srcAddr2 = e->YChar92;
            break;
    }
    //e->srcAddr2 = GETHI(e->OP2);
    
    // get 'mask'
    int mask = (int)bitfieldExtract36(e->op0, 27, 9);
    
    // fetch 'test' char
    //If MF2.ID = 0 and MF2.REG = du, then the second word following the instruction word does not contain an operand descriptor for the test character; instead, it contains the test character as a direct upper operand in bits 0,8.
    
    int ctest = 0;
    if (!(e->MF2 & MFkID) && ((e->MF2 & MFkREGMASK) == 3))  // MF2.du
    {
        int duo = GETHI(e->OP2);
        // per Bull RJ78, p. 5-45
        switch(e->srcTA)
        {
            case CTA4:
                ctest = (duo >> 13) & 017;
                break;
            case CTA6:
                ctest = (duo >> 12) & 077;
                break;
            case CTA9:
                ctest = (duo >> 9) & 0777;
                break;

        }
    }
    else
    {
        //get469(NULL, 0, 0, 0);
        //ctest = get469(e, &e->srcAddr2, &e->srcCN2, e->TA2);
        ctest = EISget469(&e->ADDR2, &e->srcCN2, e->TA2);
    }
    switch(e->srcTA2)
    {
        case CTA4:
            ctest &= 017;    // keep 4-bits
            break;
        case CTA6:
            ctest &= 077;    // keep 6-bits
            break;
        case CTA9:
            ctest &= 0777;   // keep 9-bits
    }

    word18 y3 = GETHI(e->OP3);
    int y3A = (int)bitfieldExtract36(e->OP3, 6, 1); // 'A' bit - indirect via pointer register
    int y3REG = e->OP3 & 0xf;
    
    word18 r = (word18)getMFReg(y3REG, true);
    
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    if (y3A)
    {
        // if 3rd operand contains A (bit-29 set) then it Means Y-char93 is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(y3A, 15, 3);
        int offset = y3 & 077777;  // 15-bit signed number
        y3 = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;
        
        if (get_addr_mode() == APPEND_mode)
        {
            //TPR.TSR = PR[n].SNR;
            //TPR.TRR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            e->ADDR3.SNR = PR[n].SNR;
            e->ADDR3.RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            
            e->ADDR3.mat = viaPR;
        }
    }
    
    y3 +=  ((9*ARn_CHAR + 36*r + ARn_BITNO) / 36);
    y3 &= AMASK;
    
    e->ADDR3.address = y3;
    
    //get469(NULL, 0, 0, 0);    // initialize char getter
    
    int i = 0;
    for(; i < e->N1; i += 1)
    {
        //int yCharn1 = get469(e, &e->srcAddr, &e->srcCN, e->srcTA2);
        int yCharn1 = EISget469(&e->ADDR1, &e->srcCN, e->srcTA2);
        
        int c = (~mask & (yCharn1 ^ ctest)) & 0777;
        if (c == 0)
        {
            //00...0 → C(Y3)0,11
            //i-1 → C(Y3)12,35
            //Y3 = bitfieldInsert36(Y3, i, 0, 24);
            break;
        }
    }
    word36 CY3 = bitfieldInsert36(0, i, 0, 24);
    
    SCF(i == e->N1, rIR, I_TALLY);
    
    // write Y3 .....
    //Write(e->ins, y3, CY3, OperandWrite, 0);
    EISWrite(&e->ADDR3, CY3);
}

/*
 * SCMR - Scan with Mask Reverse
 */
void scmr(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For characters i = 1, 2, ..., N1
    //   For bits j = 0, 1, ..., 8
    //      C(Z)j = ~C(MASK)j & ((C(Y-charn1)i-1 )j ⊕ (C(Y-charn2)0)j)
    //      If C(Z)0,8 = 00...0, then
    //           00...0 → C(Y3)0,11
    //           i-1 → C(Y3)12,35
    //      otherwise, continue scan of C(Y-charn1)
    // If a masked character match was not found, then
    //   00...0 → C(Y3)0,11
    //   N1 → C(Y3)12,35
    
    // Starting at location YC1, the L1 type TA1 characters are masked and compared with the assumed type TA1 character contained either in location YC2 or in bits 0-8 or 0-5 of the address field of operand descriptor 2 (when the REG field of MF2 specifies DU modification). The mask is right-justified in bit positions 0-8 of the instruction word. Each bit position of the mask that is a 1 prevents that bit position in the two characters from entering into the compare.
    // The masked compare operation continues until either a match is found or the tally (L1) is exhausted. For each unsuccessful match, a count is incremented by 1. When a match is found or when the L1 tally runs out, this count is stored right-justified in bits 12-35 of location Y3 and bits 0-11 of Y3 are zeroed. The contents of location YC2 and the source string remain unchanged. The RL bit of the MF2 field is not used.
    
    // The REG field of MF1 is checked for a legal code. If DU is specified in the REG field of MF2 in one of the four multiword instructions (SCD, SCDR, SCM, or SCMR) for which DU is legal, the CN field is ignored and the character or characters are arranged within the 18 bits of the word address portion of the operand descriptor.
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseAlphanumericOperandDescriptor(1, e);
    parseAlphanumericOperandDescriptor(2, e);
    
    e->srcCN = e->CN1;  ///< starting at char pos CN
    e->srcCN2= e->CN2;  ///< character number
    
    //Both the string and the test character pair are treated as the data type given for the string, TA1. A data type given for the test character pair, TA2, is ignored.
    e->srcTA = e->TA1;
    e->srcTA2 = e->TA1;
    
    switch(e->TA1)
    {
        case CTA4:
            //e->srcAddr = e->YChar41;
            e->srcSZ = 4;
            break;
        case CTA6:
            //e->srcAddr = e->YChar61;
            e->srcSZ = 6;
            break;
        case CTA9:
            //e->srcAddr = e->YChar91;
            e->srcSZ = 9;
            break;
    }
    
    // adjust addresses & offsets for reading in reverse ....
    int nSrcWords = 0;
    int newSrcCN = 0;
    getOffsets(e->N1, e->srcCN, e->srcTA, &nSrcWords, &newSrcCN);

    //word18 newSrcAddr = e->srcAddr + nSrcWords;
    e->ADDR1.address += nSrcWords;
    
    switch(e->TA1)      // assumed same type
    {
        case CTA4:
            //e->srcAddr2 = e->YChar42;
            break;
        case CTA6:
            //e->srcAddr2 = e->YChar62;
            break;
        case CTA9:
            //e->srcAddr2 = e->YChar92;
            break;
    }
    
    // get 'mask'
    int mask = (int)bitfieldExtract36(e->op0, 27, 9);
    
    // fetch 'test' char
    //If MF2.ID = 0 and MF2.REG = du, then the second word following the instruction word does not contain an operand descriptor for the test character; instead, it contains the test character as a direct upper operand in bits 0,8.
    
    int ctest = 0;
    if (!(e->MF2 & MFkID) && ((e->MF2 & MFkREGMASK) == 3))  // MF2.du
    {
        int duo = GETHI(e->OP2);
        // per Bull RJ78, p. 5-45
        switch(e->srcTA)
        {
            case CTA4:
                ctest = (duo >> 13) & 017;
                break;
            case CTA6:
                ctest = (duo >> 12) & 077;
                break;
            case CTA9:
                ctest = (duo >> 9) & 0777;
                break;
        }
    }
    else
    {
        //if (!(e->MF2 & MFkID))  // if id is set then we don't bother with MF2 reg as an address modifier
        //{
        //    int y2offset = getMF2Reg(e->MF2 & MFkREGMASK, (word18)bitfieldExtract36(e->OP2, 27, 9));
        //    e->srcAddr2 += y2offset;
        //}
        //get469(NULL, 0, 0, 0);
        //ctest = get469(e, &e->srcAddr2, &e->srcCN2, e->TA2);
        ctest = EISget469(&e->ADDR2, &e->srcCN2, e->TA2);
    }
    switch(e->srcTA2)
    {
        case CTA4:
            ctest &= 017;    // keep 4-bits
            break;
        case CTA6:
            ctest &= 077;    // keep 6-bits
            break;
        case CTA9:
            ctest &= 0777;   // keep 9-bits
    }
    
    word18 y3 = GETHI(e->OP3);
    int y3A = (int)bitfieldExtract36(e->OP3, 6, 1); // 'A' bit - indirect via pointer register
    int y3REG = e->OP3 & 0xf;
    
    word18 r = (word18)getMFReg(y3REG, true);
    
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    if (y3A)
    {
        // if 3rd operand contains A (bit-29 set) then it Means Y-char93 is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(y3A, 15, 3);
        int offset = y3 & 077777;  // 15-bit signed number
        y3 = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;

        if (get_addr_mode() == APPEND_mode)
        {
            //TPR.TSR = PR[n].SNR;
            //TPR.TRR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            e->ADDR3.SNR = PR[n].SNR;
            e->ADDR3.RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            
            e->ADDR3.mat = viaPR;
        }
    }
    
    y3 +=  ((9*ARn_CHAR + 36*r + ARn_BITNO) / 36);
    y3 &= AMASK;
    
    e->ADDR3.address = y3;
    
    //get469r(NULL, 0, 0, 0);    // initialize char getter
    
    int i = 0;
    for(; i < e->N1; i += 1)
    {
        //int yCharn1 = get469r(e, &newSrcAddr, &newSrcCN, e->srcTA2);
        int yCharn1 = EISget469r(&e->ADDR1, &newSrcCN, e->srcTA2);
        
        int c = (~mask & (yCharn1 ^ ctest)) & 0777;
        if (c == 0)
        {
            //00...0 → C(Y3)0,11
            //i-1 → C(Y3)12,35
            //Y3 = bitfieldInsert36(Y3, i, 0, 24);
            break;
        }
    }
    word36 CY3 = bitfieldInsert36(0, i, 0, 24);
    
    SCF(i == e->N1, rIR, I_TALLY);
    
    // write Y3 .....
    //Write(e->ins, y3, CY3, OperandWrite, 0);
    EISWrite(&e->ADDR3, CY3);
}

/*
 * TCT - Test Character and Translate
 */
void tct(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For i = 1, 2, ..., N1
    //   m = C(Y-charn1)i-1
    //   If C(Y-char92)m ≠ 00...0, then
    //     C(Y-char92)m → C(Y3)0,8
    //     000 → C(Y3)9,11
    //     i-1 → C(Y3)12,35
    //   otherwise, continue scan of C(Y-charn1)
    // If a non-zero table entry was not found, then 00...0 → C(Y3)0,11
    // N1 → C(Y3)12,35
    //
    // Indicators: Tally Run Out. If the string length count exhausts, then ON; otherwise, OFF
    //
    // If the data type of the string to be scanned is not 9-bit (TA1 ≠ 0), then characters from C(Y-charn1) are high-order zero filled in forming the table index, m.
    // Instruction execution proceeds until a non-zero table entry is found or the string length count is exhausted.
    // The character number of Y-char92 must be zero, i.e., Y-char92 must start on a word boundary.
    
    
    setupOperandDescriptor(1, e);
    
    parseAlphanumericOperandDescriptor(1, e);
    
    e->srcCN = e->CN1;    // starting at char pos CN
    e->srcTA = e->TA1;
    
    switch(e->TA1)
    {
        case CTA4:
            //e->srcAddr = e->YChar41;
            e->srcSZ = 4;
            break;
        case CTA6:
            //e->srcAddr = e->YChar61;
            e->srcSZ = 6;
            break;
        case CTA9:
            //e->srcAddr = e->YChar91;
            e->srcSZ = 9;
            break;
    }
    
    
    // fetch 2nd operand ...
    
    word36 xlat = e->OP2;   //op[1];                 // 2nd word is a pointer to a translation table
    int xA = (int)bitfieldExtract36(xlat, 6, 1); // 'A' bit - indirect via pointer register
    int xREG = xlat & 0xf;
    
    word18 r = (word18)getMFReg(xREG, true);
    
    word18 xAddress = GETHI(xlat);
    
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    if (xA)
    {
        // if 2nd operand contains A (bit-29 set) then it Means Y-char92 is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(xAddress, 15, 3);
        int offset = xAddress & 077777;  // 15-bit signed number
        xAddress = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;

        if (get_addr_mode() == APPEND_mode)
        {
            //TPR.TSR = PR[n].SNR;
            //TPR.TRR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            e->ADDR2.SNR = PR[n].SNR;
            e->ADDR2.RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            
            e->ADDR2.mat = viaPR;
        }
    }
    
    xAddress +=  ((9*ARn_CHAR + 36*r + ARn_BITNO) / 36);
    xAddress &= AMASK;
    
    e->ADDR2.address = xAddress;
    
    // XXX I think this is where prepage mode comes in. Need to ensure that the translation table's page is im memory.
    // XXX handle, later. (Yeah, just like everything else hard.)
    //  Prepage Check in a Multiword Instruction
    //  The MVT, TCT, TCTR, and CMPCT instruction have a prepage check. The size of the translate table is determined by the TA1 data type as shown in the table below. Before the instruction is executed, a check is made for allocation in memory for the page for the translate table. If the page is not in memory, a Missing Page fault occurs before execution of the instruction. (Bull RJ78 p.7-75)
    
    // TA1              TRANSLATE TABLE SIZE
    // 4-BIT CHARACTER      4 WORDS
    // 6-BIT CHARACTER     16 WORDS
    // 9-BIT CHARACTER    128 WORDS
    
    int xlatSize = 0;   // size of xlation table in words .....
    switch(e->TA1)
    {
        case CTA4:
            xlatSize = 4;
            break;
        case CTA6:
            xlatSize = 16;
            break;
        case CTA9:
            xlatSize = 128;
            break;
    }
    
    word36 xlatTbl[128];
    memset(xlatTbl, 0, sizeof(xlatTbl));    // 0 it out just in case
    
    // XXX here is where we probably need to to the prepage thang...
    //ReadNnoalign(currentInstruction,  xlatSize, xAddress, xlatTbl, OperandRead, 0);
    EISReadN(&e->ADDR2, xlatSize, xlatTbl);
    
    // fetch 3rd operand ...
    
    word18 y3 = GETHI(e->OP3);
    int y3A = (int)bitfieldExtract36(e->OP3, 6, 1); // 'A' bit - indirect via pointer register
    int y3REG = e->OP3 & 0xf;
    
    r = (word18)getMFReg(y3REG, true);
    
    ARn_CHAR = 0;
    ARn_BITNO = 0;
    if (y3A)
    {
        // if 3rd operand contains A (bit-29 set) then it Means Y-char93 is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(y3A, 15, 3);
        int offset = y3 & 077777;  // 15-bit signed number
        y3 = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;
        
        if (get_addr_mode() == APPEND_mode)
        {
            //TPR.TSR = PR[n].SNR;
            //TPR.TRR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            e->ADDR3.SNR = PR[n].SNR;
            e->ADDR3.RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            
            e->ADDR3.mat = viaPR;
        }
    }
    
    y3 +=  ((9*ARn_CHAR + 36*r + ARn_BITNO) / 36);
    y3 &= AMASK;

    e->ADDR3.address = y3;
    
    
    word36 CY3 = 0;
    
    //get469(NULL, 0, 0, 0);    // initialize char getter
    
    int i = 0;
    for(; i < e->N1 ; i += 1)
    {
        //int c = get469(e, &e->srcAddr, &e->srcCN, e->TA1); // get src char
        int c = EISget469(&e->ADDR1, &e->srcCN, e->TA1); // get src char

        int m = 0;
        
        switch (e->srcSZ)
        {
            case 4:
                m = c & 017;    // truncate upper 2-bits
                break;
            case 6:
                m = c & 077;    // truncate upper 3-bits
                break;
            case 9:
                m = c;          // keep all 9-bits
                break;              // should already be 0-filled
        }
        
        int cout = xlate(xlatTbl, e->srcTA, m);
        if (cout)
        {
            CY3 = bitfieldInsert36(0, cout, 27, 9); // C(Y-char92)m → C(Y3)0,8
            break;
        }
    }
    
    SCF(i == e->N1, rIR, I_TALLY);
    
    CY3 = bitfieldInsert36(CY3, i, 0, 24);
    
    // write Y3 .....
    //Write(e->ins, y3, CY3, OperandWrite, 0);
    EISWrite(&e->ADDR3, CY3);
}

/*
 * TCTR - Test Character and Translate Reverse
 */
void tctr(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For i = 1, 2, ..., N1
    //   m = C(Y-charn1)N1-i
    //   If C(Y-char92)m ≠ 00...0, then
    //     C(Y-char92)m → C(Y3)0,8
    //     000 → C(Y3)9,11
    //     i-1 → C(Y3)12,35
    //   otherwise, continue scan of C(Y-charn1) If a non-zero table entry was not found, then
    // 00...0 → C(Y3)0,11
    // N1 → C(Y3)12,35
    //
    // Indicators: Tally Run Out. If the string length count exhausts, then ON; otherwise, OFF
    //
    // If the data type of the string to be scanned is not 9-bit (TA1 ≠ 0), then characters from C(Y-charn1) are high-order zero filled in forming the table index, m.
    // Instruction execution proceeds until a non-zero table entry is found or the string length count is exhausted.
    // The character number of Y-char92 must be zero, i.e., Y-char92 must start on a word boundary.
    
    
    setupOperandDescriptor(1, e);
    parseAlphanumericOperandDescriptor(1, e);
    
    e->srcCN = e->CN1;    // starting at char pos CN
    e->srcTA = e->TA1;
    
    switch(e->TA1)
    {
        case CTA4:
            //e->srcAddr = e->YChar41;
            e->srcSZ = 4;
            break;
        case CTA6:
            //e->srcAddr = e->YChar61;
            e->srcSZ = 6;
            break;
        case CTA9:
            //e->srcAddr = e->YChar91;
            e->srcSZ = 9;
            break;
    }
    
    // adjust addresses & offsets for reading in reverse ....
    int nSrcWords = 0;
    int newSrcCN = 0;
    getOffsets(e->N1, e->srcCN, e->srcTA, &nSrcWords, &newSrcCN);
    
    //word18 newSrcAddr = e->srcAddr + nSrcWords;
    e->ADDR1.address += nSrcWords;

    // fetch 2nd operand ...
    
    word36 xlat = e->OP2;   //op[1];                 // 2nd word is a pointer to a translation table
    int xA = (int)bitfieldExtract36(xlat, 6, 1); // 'A' bit - indirect via pointer register
    int xREG = xlat & 0xf;
    
    word18 r = (word18)getMFReg(xREG, true);
    
    word18 xAddress = GETHI(xlat);
    
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    if (xA)
    {
        // if 2nd operand contains A (bit-29 set) then it Means Y-char92 is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(xAddress, 15, 3);
        int offset = xAddress & 077777;  // 15-bit signed number
        xAddress = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;
        
        if (get_addr_mode() == APPEND_mode)
        {
            //TPR.TSR = PR[n].SNR;
            //TPR.TRR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            e->ADDR2.SNR = PR[n].SNR;
            e->ADDR2.RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);

            e->ADDR2.mat = viaPR;
        }
    }
    
    xAddress +=  ((9*ARn_CHAR + 36*r + ARn_BITNO) / 36);
    xAddress &= AMASK;
    
    e->ADDR2.address = xAddress;
    
    // XXX I think this is where prepage mode comes in. Need to ensure that the translation table's page is im memory.
    // XXX handle, later. (Yeah, just like everything else hard.)
    //  Prepage Check in a Multiword Instruction
    //  The MVT, TCT, TCTR, and CMPCT instruction have a prepage check. The size of the translate table is determined by the TA1 data type as shown in the table below. Before the instruction is executed, a check is made for allocation in memory for the page for the translate table. If the page is not in memory, a Missing Page fault occurs before execution of the instruction. (Bull RJ78 p.7-75)
    
    // TA1              TRANSLATE TABLE SIZE
    // 4-BIT CHARACTER      4 WORDS
    // 6-BIT CHARACTER     16 WORDS
    // 9-BIT CHARACTER    128 WORDS
    
    int xlatSize = 0;   // size of xlation table in words .....
    switch(e->TA1)
    {
        case CTA4:
            xlatSize = 4;
            break;
        case CTA6:
            xlatSize = 16;
            break;
        case CTA9:
            xlatSize = 128;
            break;
    }
    
    word36 xlatTbl[128];
    memset(xlatTbl, 0, sizeof(xlatTbl));    // 0 it out just in case
    
    // XXX here is where we probably need to to the prepage thang...
    //ReadNnoalign(currentInstruction,  xlatSize, xAddress, xlatTbl, OperandRead, 0);
    EISReadN(&e->ADDR2, xlatSize, xlatTbl);
    
    // fetch 3rd operand ...
    
    word18 y3 = GETHI(e->OP3);
    int y3A = (int)bitfieldExtract36(e->OP3, 6, 1); // 'A' bit - indirect via pointer register
    int y3REG = e->OP3 & 0xf;
    
    r = (word18)getMFReg(y3REG, true);
    
    ARn_CHAR = 0;
    ARn_BITNO = 0;
    if (y3A)
    {
        // if 3rd operand contains A (bit-29 set) then it Means Y-char93 is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(y3A, 15, 3);
        int offset = y3 & 077777;  // 15-bit signed number
        y3 = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;
        
        if (get_addr_mode() == APPEND_mode)
        {
            //TPR.TSR = PR[n].SNR;
            //TPR.TRR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            e->ADDR3.SNR = PR[n].SNR;
            e->ADDR3.RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            
            e->ADDR3.mat = viaPR;
        }
    }
    
    y3 +=  ((9*ARn_CHAR + 36*r + ARn_BITNO) / 36);
    y3 &= AMASK;
    
    e->ADDR3.address = y3;
    
    word36 CY3 = 0;
    
    //get469r(NULL, 0, 0, 0);    // initialize char getter
    
    int i = 0;
    for(; i < e->N1 ; i += 1)
    {
        //int c = get469r(e, &newSrcAddr, &newSrcCN, e->TA1); // get src char
        int c = EISget469r(&e->ADDR1, &newSrcCN, e->TA1); // get src char
        
        int m = 0;
        
        switch (e->srcSZ)
        {
            case 4:
                m = c & 017;    // truncate upper 2-bits
                break;
            case 6:
                m = c & 077;    // truncate upper 3-bits
                break;
            case 9:
                m = c;          // keep all 9-bits
                break;          // should already be 0-filled
        }
        
        int cout = xlate(xlatTbl, e->srcTA, m);

        if (cout)
        {
            CY3 = bitfieldInsert36(0, cout, 27, 9); // C(Y-char92)m → C(Y3)0,8
            break;
        }
    }
    
    SCF(i == e->N1, rIR, I_TALLY);
    
    CY3 = bitfieldInsert36(CY3, i, 0, 24);
    
    // write Y3 .....
    //Write(e->ins, y3, CY3, OperandWrite, 0);
    EISWrite(&e->ADDR3, CY3);
}


void cmpc(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For i = 1, 2, ..., minimum (N1,N2)
    //    C(Y-charn1)i-1 :: C(Y-charn2)i-1
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //    C(FILL) :: C(Y-charn2)i-1
    // If N1 > N2, then for i = N2+1, N2+2, ..., N1
    //    C(Y-charn1)i-1 :: C(FILL)
    //
    // Indicators:
    //     Zero: If C(Y-charn1)i-1 = C(Y-charn2)i-1 for all i, then ON; otherwise, OFF
    //     Carry: If C(Y-charn1)i-1 < C(Y-charn2)i-1 for any i, then OFF; otherwise ON
    
    // Both strings are treated as the data type given for the left-hand string, TA1. A data type given for the right-hand string, TA2, is ignored.
    // Comparison is made on full 9-bit fields. If the given data type is not 9-bit (TA1 ≠ 0), then characters from C(Y-charn1) and C(Y-charn2) are high- order zero filled. All 9 bits of C(FILL) are used.
    // Instruction execution proceeds until an inequality is found or the larger string length count is exhausted.
    
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseAlphanumericOperandDescriptor(1, e);
    parseAlphanumericOperandDescriptor(2, e);
    
    e->srcCN = e->CN1;  ///< starting at char pos CN
    e->srcCN2= e->CN2;  ///< character number
    
    e->srcTA = e->TA1;
    
    switch(e->srcTA)
    {
        case CTA4:
            //e->srcAddr = e->YChar41;
            break;
        case CTA6:
            //e->srcAddr = e->YChar61;
            break;
        case CTA9:
            //e->srcAddr = e->YChar91;
            break;
    }
    
    switch(e->srcTA)      // assumed same type
    {
        case CTA4:
            //e->srcAddr2 = e->YChar42;
            break;
        case CTA6:
            //e->srcAddr2 = e->YChar62;
            break;
        case CTA9:
            //e->srcAddr2 = e->YChar92;
            break;
    }

    int fill = (int)bitfieldExtract36(e->op0, 27, 9);
    
    //get469(NULL, 0, 0, 0);    // initialize src1 getter
    //get4692(NULL, 0, 0, 0);   // initialize src2 getter
    
    
    SETF(rIR, I_ZERO);  // set ZERO flag assuming strings are are equal ...
    SETF(rIR, I_CARRY); // set CARRY flag assuming strings are are equal ...
    
    int i = 0;
    for(; i < min(e->N1, e->N2); i += 1)
    {
        //int c1 = get469 (e, &e->srcAddr,  &e->srcCN,  e->TA1);   // get Y-char1n
        int c1 = EISget469(&e->ADDR1,  &e->srcCN,  e->TA1);   // get Y-char1n
        //int c2 = get4692(e, &e->srcAddr2, &e->srcCN2, e->TA2);   // get Y-char2n
        int c2 = EISget469(&e->ADDR2, &e->srcCN2, e->TA2);   // get Y-char2n
        
        if (c1 != c2)
        {
            CLRF(rIR, I_ZERO);  // an inequality found
            
            SCF(c1 < c2, rIR, I_CARRY);
            
            return;
        }
    }
    if (e->N1 < e->N2)
        for(; i < e->N2; i += 1)
        {
            int c1 = fill;                                      // use fill for Y-char1n
            //int c2 = get4692(e, &e->srcAddr2, &e->srcCN2, e->TA2); // get Y-char2n
            int c2 = EISget469(&e->ADDR2, &e->srcCN2, e->TA2); // get Y-char2n
            
            if (c1 != c2)
            {
                CLRF(rIR, I_ZERO);  // an inequality found
                
                SCF(c1 < c2, rIR, I_CARRY);
                
                return;
            }
        }
    else if (e->N1 > e->N2)
        for(; i < e->N1; i += 1)
        {
            //int c1 = get469 (e, &e->srcAddr,  &e->srcCN,  e->TA1);   // get Y-char1n
            int c1 = EISget469 (&e->ADDR1,  &e->srcCN,  e->TA1);   // get Y-char1n
            int c2 = fill;                                        // use fill for Y-char2n
            
            if (c1 != c2)
            {
                CLRF(rIR, I_ZERO);  // an inequality found
                
                SCF(c1 < c2, rIR, I_CARRY);
                
                return;
            }
        }
}

/*
 * SCD - Scan Characters Double
 */
void scd(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For i = 1, 2, ..., N1-1
    //   C(Y-charn1)i-1,i :: C(Y-charn2)0,1
    // On instruction completion, if a match was found:
    //   00...0 → C(Y3)0,11
    //   i-1 → C(Y3)12,35
    // If no match was found:
    //   00...0 → C(Y3)0,11
    //      N1-1→ C(Y3)12,35
    //
    
    // The REG field of MF1 is checked for a legal code. If DU is specified in the REG field of MF2 in one of the four multiword instructions (SCD, SCDR, SCM, or SCMR) for which DU is legal, the CN field is ignored and the character or characters are arranged within the 18 bits of the word address portion of the operand descriptor.
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseAlphanumericOperandDescriptor(1, e);
    parseAlphanumericOperandDescriptor(2, e);
    
    e->srcCN = e->CN1;  ///< starting at char pos CN
    e->srcCN2= e->CN2;  ///< character number
    
    //Both the string and the test character pair are treated as the data type given for the string, TA1. A data type given for the test character pair, TA2, is ignored.
    e->srcTA = e->TA1;
    e->srcTA2 = e->TA1;
    
    switch(e->TA1)
    {
        case CTA4:
            //e->srcAddr = e->YChar41;
            break;
        case CTA6:
            //e->srcAddr = e->YChar61;
            break;
        case CTA9:
            //e->srcAddr = e->YChar91;
            break;
    }
    
    //e->srcAddr2 = GETHI(e->OP2);
    e->ADDR2.address = GETHI(e->OP2);
    
    // fetch 'test' char - double
    //If MF2.ID = 0 and MF2.REG = du, then the second word following the instruction word does not contain an operand descriptor for the test character; instead, it contains the test character as a direct upper operand in bits 0,8.
    
    int c1 = 0;
    int c2 = 0;
    
#ifndef QUIET_UNUSED
    int ctest = 0;
#endif
    if (!(e->MF2 & MFkID) && ((e->MF2 & MFkREGMASK) == 3))  // MF2.du
    {
        // per Bull RJ78, p. 5-45
        switch(e->srcTA)
        {
            case CTA4:
                //c1 = (e->srcAddr2 >> 13) & 017;
                //c2 = (e->srcAddr2 >>  9) & 017;
                c1 = (e->ADDR2.address >> 13) & 017;
                c2 = (e->ADDR2.address >>  9) & 017;
                break;
            case CTA6:
                //c1 = (e->srcAddr2 >> 12) & 077;
                //c2 = (e->srcAddr2 >>  6) & 077;
                c1 = (e->ADDR2.address >> 12) & 077;
                c2 = (e->ADDR2.address >>  6) & 077;
                break;
            case CTA9:
                //c1 = (e->srcAddr2 >> 9) & 0777;
                //c2 = (e->srcAddr2     ) & 0777;
                c1 = (e->ADDR2.address >> 9) & 0777;
                c2 = (e->ADDR2.address     ) & 0777;
                break;
        }
    }
    else
    {
        if (!(e->MF2 & MFkID))  // if id is set then we don't bother with MF2 reg as an address modifier
        {
            int y2offset = getMF2Reg(e->MF2 & MFkREGMASK, (word18)bitfieldExtract36(e->OP2, 27, 9));
            //e->srcAddr2 += y2offset;
            e->ADDR2.address += y2offset;
        }
        //get469(NULL, 0, 0, 0);
        //c1 = get469(e, &e->srcAddr2, &e->srcCN2, e->TA2);
        //c2 = get469(e, &e->srcAddr2, &e->srcCN2, e->TA2);
        c1 = EISget469(&e->ADDR2, &e->srcCN2, e->TA2);
        c2 = EISget469(&e->ADDR2, &e->srcCN2, e->TA2);

    }
    switch(e->srcTA2)
    {
        case CTA4:
            c1 &= 017;    // keep 4-bits
            c2 &= 017;    // keep 4-bits
            break;
        case CTA6:
            c1 &= 077;    // keep 6-bits
            c2 &= 077;    // keep 6-bits
            break;
        case CTA9:
            c1 &= 0777;   // keep 9-bits
            c2 &= 0777;   // keep 9-bits
            break;
    }
    
    word18 y3 = GETHI(e->OP3);
    int y3A = (int)bitfieldExtract36(e->OP3, 6, 1); // 'A' bit - indirect via pointer register
    int y3REG = e->OP3 & 0xf;
    
    word18 r = (word18)getMFReg(y3REG, true);
    
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    if (y3A)
    {
        // if 3rd operand contains A (bit-29 set) then it Means Y-char93 is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(y3A, 15, 3);
        int offset = y3 & 077777;  // 15-bit signed number
        y3 = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;
        
        if (get_addr_mode() == APPEND_mode)
        {
            //TPR.TSR = PR[n].SNR;
            //TPR.TRR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            e->ADDR3.SNR = PR[n].SNR;
            e->ADDR3.RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            
            e->ADDR3.mat = viaPR;
        }
    }
    
    y3 +=  ((9*ARn_CHAR + 36*r + ARn_BITNO) / 36);
    y3 &= AMASK;
    
    e->ADDR3.address = y3;
    
    
    //get469(NULL, 0, 0, 0);    // initialize char getter
    
    int yCharn11 = 0;
    int yCharn12 = 0;
    
    int i = 0;
    for(; i < e->N1-1; i += 1)
    {
        
        if (i == 0)
        {
            //yCharn11 = get469(e, &e->srcAddr, &e->srcCN, e->srcTA2);
            //yCharn12 = get469(e, &e->srcAddr, &e->srcCN, e->srcTA2);
            yCharn11 = EISget469(&e->ADDR1, &e->srcCN, e->srcTA2);
            yCharn12 = EISget469(&e->ADDR1, &e->srcCN, e->srcTA2);
        }
        else
        {
            yCharn11 = yCharn12;
            //yCharn12 = get469(e, &e->srcAddr, &e->srcCN, e->srcTA2);
            yCharn12 = EISget469(&e->ADDR1, &e->srcCN, e->srcTA2);
        }
        
        if (yCharn11 == c1 && yCharn12 == c2)
            break;
    }
    
    word36 CY3 = bitfieldInsert36(0, i, 0, 24);
    
    SCF(i == e->N1-1, rIR, I_TALLY);
    
    // write Y3 .....
    //Write(e->ins, y3, CY3, OperandWrite, 0);
    EISWrite(&e->ADDR3, CY3);
}
/*
 * SCDR - Scan Characters Double Reverse
 */
void scdr(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For i = 1, 2, ..., N1-1
    //   C(Y-charn1)N1-i-1,N1-i :: C(Y-charn2)0,1
    // On instruction completion, if a match was found:
    //   00...0 → C(Y3)0,11
    //   i-1 → C(Y3)12,35
    // If no match was found:
    //   00...0 → C(Y3)0,11
    //      N1-1→ C(Y3)12,35
    //
    
    // The REG field of MF1 is checked for a legal code. If DU is specified in the REG field of MF2 in one of the four multiword instructions (SCD, SCDR, SCM, or SCMR) for which DU is legal, the CN field is ignored and the character or characters are arranged within the 18 bits of the word address portion of the operand descriptor.
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseAlphanumericOperandDescriptor(1, e);
    parseAlphanumericOperandDescriptor(2, e);
    
    e->srcCN = e->CN1;  ///< starting at char pos CN
    e->srcCN2= e->CN2;  ///< character number
    
    //Both the string and the test character pair are treated as the data type given for the string, TA1. A data type given for the test character pair, TA2, is ignored.
    e->srcTA = e->TA1;
    e->srcTA2 = e->TA1;
    
    switch(e->TA1)
    {
        case CTA4:
            //e->srcAddr = e->YChar41;
            break;
        case CTA6:
            //e->srcAddr = e->YChar61;
            break;
        case CTA9:
            //e->srcAddr = e->YChar91;
            break;
    }
    
    // adjust addresses & offsets for reading in reverse ....
    int nSrcWords = 0;
    int newSrcCN = 0;
    getOffsets(e->N1, e->srcCN, e->srcTA, &nSrcWords, &newSrcCN);
    
    //word18 newSrcAddr = e->srcAddr + nSrcWords;
    e->ADDR1.address += nSrcWords;

    
    //e->srcAddr2 = GETHI(e->OP2);
    e->ADDR2.address = GETHI(e->OP2);
    
    // fetch 'test' char - double
    //If MF2.ID = 0 and MF2.REG = du, then the second word following the instruction word does not contain an operand descriptor for the test character; instead, it contains the test character as a direct upper operand in bits 0,8.
    
    int c1 = 0;
    int c2 = 0;
    
#ifndef QUIET_UNUSED
    int ctest = 0;
#endif
    if (!(e->MF2 & MFkID) && ((e->MF2 & MFkREGMASK) == 3))  // MF2.du
    {
        // per Bull RJ78, p. 5-45
        switch(e->srcTA)
        {
            case CTA4:
                //c1 = (e->srcAddr2 >> 13) & 017;
                //c2 = (e->srcAddr2 >>  9) & 017;
                c1 = (e->ADDR2.address >> 13) & 017;
                c2 = (e->ADDR2.address >>  9) & 017;
                break;
            case CTA6:
                //c1 = (e->srcAddr2 >> 12) & 077;
                //c2 = (e->srcAddr2 >>  6) & 077;
                c1 = (e->ADDR2.address >> 12) & 077;
                c2 = (e->ADDR2.address >>  6) & 077;

                break;
            case CTA9:
                //c1 = (e->srcAddr2 >> 9) & 0777;
                //c2 = (e->srcAddr2     ) & 0777;
                c1 = (e->ADDR2.address >> 9) & 0777;
                c2 = (e->ADDR2.address     ) & 0777;
                
                break;
        }
    }
    else
    {
        if (!(e->MF2 & MFkID))  // if id is set then we don't bother with MF2 reg as an address modifier
        {
            int y2offset = getMF2Reg(e->MF2 & MFkREGMASK, (word18)bitfieldExtract36(e->OP2, 27, 9));
            //e->srcAddr2 += y2offset;
            e->ADDR2.address += y2offset;
        }
        //get469(NULL, 0, 0, 0);    // initialize char getter
        //c1 = get469(e, &e->srcAddr2, &e->srcCN2, e->TA2);
        //c2 = get469(e, &e->srcAddr2, &e->srcCN2, e->TA2);
        c1 = EISget469(&e->ADDR2, &e->srcCN2, e->TA2);
        c2 = EISget469(&e->ADDR2, &e->srcCN2, e->TA2);
    }
    switch(e->srcTA2)
    {
        case CTA4:
            c1 &= 017;    // keep 4-bits
            c2 &= 017;    // keep 4-bits
            break;
        case CTA6:
            c1 &= 077;    // keep 6-bits
            c2 &= 077;    // keep 6-bits
            break;
        case CTA9:
            c1 &= 0777;   // keep 9-bits
            c2 &= 0777;   // keep 9-bits
            break;
    }
    
    word18 y3 = GETHI(e->OP3);
    int y3A = (int)bitfieldExtract36(e->OP3, 6, 1); // 'A' bit - indirect via pointer register
    int y3REG = e->OP3 & 0xf;
    
    word18 r = (word18)getMFReg(y3REG, true);
    
    word8 ARn_CHAR = 0;
    word6 ARn_BITNO = 0;
    if (y3A)
    {
        // if 3rd operand contains A (bit-29 set) then it Means Y-char93 is not the memory address of the data but is a reference to a pointer register pointing to the data.
        int n = (int)bitfieldExtract36(y3A, 15, 3);
        int offset = y3 & 077777;  // 15-bit signed number
        y3 = (AR[n].WORDNO + SIGNEXT15(offset)) & 0777777;
        
        ARn_CHAR = AR[n].CHAR;
        ARn_BITNO = AR[n].BITNO;
        
        if (get_addr_mode() == APPEND_mode)
        {
            //TPR.TSR = PR[n].SNR;
            //TPR.TRR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            e->ADDR3.SNR = PR[n].SNR;
            e->ADDR3.RNR = max3(PR[n].RNR, TPR.TRR, PPR.PRR);
            
            e->ADDR3.mat = viaPR;
        }
    }
    
    y3 +=  ((9*ARn_CHAR + 36*r + ARn_BITNO) / 36);
    y3 &= AMASK;
    
    e->ADDR3.address = y3;
    
    //get469r(NULL, 0, 0, 0);    // initialize char getter
    
    int yCharn11 = 0;
    int yCharn12 = 0;
    
    int i = 0;
    for(; i < e->N1-1; i += 1)
    {
        // since we're going in reverse things are a bit different
        if (i == 0)
        {
            //yCharn12 = get469r(e, &newSrcAddr, &newSrcCN, e->srcTA2);
            //yCharn11 = get469r(e, &newSrcAddr, &newSrcCN, e->srcTA2);
            yCharn12 = EISget469r(&e->ADDR1, &newSrcCN, e->srcTA2);
            yCharn11 = EISget469r(&e->ADDR1, &newSrcCN, e->srcTA2);
        }
        else
        {
            yCharn12 = yCharn11;
            //yCharn11 = get469r(e, &newSrcAddr, &newSrcCN, e->srcTA2);
            yCharn11 = EISget469r(&e->ADDR1, &newSrcCN, e->srcTA2);
        }
        
        if (yCharn11 == c1 && yCharn12 == c2)
            break;
    }
    
    word36 CY3 = bitfieldInsert36(0, i, 0, 24);
    
    SCF(i == e->N1-1, rIR, I_TALLY);
    
    // write Y3 .....
    //Write(e->ins, y3, CY3, OperandWrite, 0);
    EISWrite(&e->ADDR3, CY3);
}

/*
 * get a bit from memory ....
 */
// XXX this is terribly ineffecient, but it'll do for now ......
PRIVATE
bool EISgetBit(EISaddr *p, int *cpos, int *bpos)
{
    //static word18 lastAddress;  // try to keep memory access' down
    
    if (!p)
    {
        //lastAddress = -1;
        return 0;
    }
    
    if (*bpos > 8)      // bits 0-8
    {
        *bpos = 0;
        *cpos += 1;
        if (*cpos > 3)  // chars 0-3
        {
            *cpos = 0;
            p->address += 1;
            p->address &= 0777777;
        }
    }
    
    //static word36 data;
    //if (p->lastAddress != p->address)                     // read from memory if different address
        p->data = EISRead(p); // read data word from memory
    
    int charPosn = ((3 - *cpos) * 9);     // 9-bit char bit position
    int bitPosn = charPosn + (8 - *bpos);
    
    bool b = (bool)bitfieldExtract36(p->data, bitPosn, 1);
    
    *bpos += 1;
    //p->lastAddress = p->address;
    
    return b;
}


/*
 * write a bit to memory (in the most ineffecient way possible)
 */

#ifndef QUIET_UNUSED
PRIVATE
void EISwriteBit(EISaddr *p, int *cpos, int *bpos, bool bit)
{
    if (*bpos > 8)      // bits 0-8
    {
        *bpos = 0;
        *cpos += 1;
        if (*cpos > 3)  // chars 0-3
        {
            *cpos = 0;
            p->address += 1;
            p->address &= 0777777;
        }
    }
    
    p->data = EISRead(p); // read data word from memory
    
    int charPosn = ((3 - *cpos) * 9);     // 9-bit char bit position
    int bitPosn = charPosn + (8 - *bpos);
    
    p->data = bitfieldInsert36(p->data, bit, bitPosn, 1);
    
    EISWrite(p, p->data); // write data word to memory
    
    *bpos += 1;
}
#endif

PRIVATE
bool EISgetBitRW(EISaddr *p)
{
    // make certain we have a valid address
    if (p->bPos > 8)      // bits 0-8
    {
        p->bPos = 0;
        p->cPos += 1;
        if (p->cPos > 3)  // chars 0-3
        {
            p->cPos = 0;
            p->address += 1;
            p->address &= 0777777;
        }
    }
    else if (p->bPos < 0)      // bits 0-8
    {
        p->bPos = 8;
        p->cPos -= 1;
        if (p->cPos < 0)  // chars 0-3
        {
            p->cPos = 3;
            p->address -= 1;
            p->address &= 0777777;
        }
    }
    
    int charPosn = ((3 - p->cPos) * 9);     // 9-bit char bit position
    int bitPosn = charPosn + (8 - p->bPos);
    
    //if (p->lastAddress != p->address)                     // read from memory if different address
        //Read(NULL, p->addr, &p->data, OperandRead, 0); // read data word from memory
    p->data = EISRead(p); // read data word from memory
    
    if (p->mode == eRWreadBit)
    {
        p->bit = (bool)bitfieldExtract36(p->data, bitPosn, 1);
        
        //if (p->lastAddress != p->address)                     // read from memory if different address
            p->data = EISRead(p); // read data word from memory
        
        // increment address after use
        if (p->incr)
            p->bPos += 1;
        // decrement address after use
        if (p->decr)
            p->bPos -= 1;
        
        
    } else if (p->mode == eRWwriteBit)
    {
        p->data = bitfieldInsert36(p->data, p->bit, bitPosn, 1);
        
        EISWrite(p, p->data); // write data word to memory
        
        if (p->incr)
            p->bPos += 1;
        if (p->decr)
            p->bPos -= 1;
    }
    //p->lastAddress = p->address;
    return p->bit;
}


/*
 * CMPB - Compare Bit Strings
 */
void cmpb(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    
    // For i = 1, 2, ..., minimum (N1,N2)
    //   C(Y-bit1)i-1 :: C(Y-bit2)i-1
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //   C(FILL) :: C(Y-bit2)i-1
    // If N1 > N2, then for i = N2+l, N2+2, ..., N1
    //   C(Y-bit1)i-1 :: C(FILL)
    //
    // Indicators:
    //    Zero:  If C(Y-bit1)i = C(Y-bit2)i for all i, then ON; otherwise, OFF
    //    Carry: If C(Y-bit1)i < C(Y-bit2)i for any i, then OFF; otherwise ON
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseBitstringOperandDescriptor(1, e);
    parseBitstringOperandDescriptor(2, e);
    
    //word18 srcAddr1 = e->YBit1;
    //word18 srcAddr2 = e->YBit2;
    
    int charPosn1 = e->C1;
    int charPosn2 = e->C2;
    
    int bitPosn1 = e->B1;
    int bitPosn2 = e->B2;
    
    e->F = (bool)bitfieldExtract36(e->op0, 25, 1) & 1;     // fill bit

    SETF(rIR, I_ZERO);  // assume all =
    SETF(rIR, I_CARRY); // assume all >=
    
    //getBit (0, 0, 0);   // initialize bit getter 1
    //getBit2(0, 0, 0);   // initialize bit getter 2
    
    for(int i = 0 ; i < min(e->N1, e->N2) ; i += 1)
    {
        //bool b1 = getBit (&srcAddr1, &charPosn1, &bitPosn1);
        //bool b2 = getBit2(&srcAddr2, &charPosn2, &bitPosn2);
        bool b1 = EISgetBit (&e->ADDR1, &charPosn1, &bitPosn1);
        bool b2 = EISgetBit (&e->ADDR2, &charPosn2, &bitPosn2);
        
        if (b1 != b2)
        {
            CLRF(rIR, I_ZERO);
            if (!b1 && b2)  // 0 < 1
                CLRF(rIR, I_CARRY);
            return;
        }
        
    }
    if (e->N1 < e->N2)
    {
        bool b1 = e->F;
        //bool b2 = getBit2(&srcAddr2, &charPosn2, &bitPosn2);
        bool b2 = EISgetBit(&e->ADDR2, &charPosn2, &bitPosn2);
        
        if (b1 != b2)
        {
            CLRF(rIR, I_ZERO);
            if (!b1 && b2)  // 0 < 1
                CLRF(rIR, I_CARRY);
            return;
        }
        
    } else if (e->N1 > e->N2)
    {
        //bool b1 = getBit(&srcAddr1, &charPosn1, &bitPosn1);
        bool b1 = EISgetBit(&e->ADDR1, &charPosn1, &bitPosn1);
        bool b2 = e->F;
        
        if (b1 != b2)
        {
            CLRF(rIR, I_ZERO);
            if (!b1 && b2)  // 0 < 1
                CLRF(rIR, I_CARRY);
            return;
        }
    }
}

void csl(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For i = bits 1, 2, ..., minimum (N1,N2)
    //   m = C(Y-bit1)i-1 || C(Y-bit2)i-1 (a 2-bit number)
    //   C(BOLR)m → C(Y-bit2)i-1
    // If N1 < N2, then for i = N1+l, N1+2, ..., N2
    //   m = C(F) || C(Y-bit2)i-1 (a 2-bit number)
    //   C(BOLR)m → C(Y-bit2)i-1
    //
    // INDICATORS: (Indicators not listed are not affected)
    //     Zero If C(Y-bit2) = 00...0, then ON; otherwise OFF
    //     Truncation If N1 > N2, then ON; otherwise OFF
    //
    // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not processed and the truncation indicator is set ON.
    //
    // If T = 1 and the truncation indicator is set ON by execution of the instruction, then a truncation (overflow) fault occurs.
    //
    // BOLR
    // If first operand    and    second operand    then result
    // bit is:                    bit is:           is from bit:
    //        0                          0                      5
    //        0                          1                      6
    //        1                          0                      7
    //        1                          1                      8
    //
    // The Boolean operations most commonly used are
    //                  BOLR Field Bits
    // Operation        5      6      7      8
    //
    // MOVE             0      0      1      1
    // AND              0      0      0      1
    // OR               0      1      1      1
    // NAND             1      1      1      0
    // EXCLUSIVE OR     0      1      1      0
    // Clear            0      0      0      0
    // Invert           1      1      0      0
    //
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseBitstringOperandDescriptor(1, e);
    parseBitstringOperandDescriptor(2, e);
    
    e->ADDR1.cPos = e->C1;
    e->ADDR2.cPos = e->C2;
    
    e->ADDR1.bPos = e->B1;
    e->ADDR2.bPos = e->B2;
    
    e->F = (bool)bitfieldExtract36(e->op0, 35, 1);   // fill bit
    e->T = (bool)bitfieldExtract36(e->op0, 26, 1);   // T (enablefault) bit
    
    e->BOLR = (int)bitfieldExtract36(e->op0, 27, 4);  // BOLR field
    bool B5 = (bool)((e->BOLR >> 3) & 1);
    bool B6 = (bool)((e->BOLR >> 2) & 1);
    bool B7 = (bool)((e->BOLR >> 1) & 1);
    bool B8 = (bool)( e->BOLR      & 1);
    
    e->ADDR1.incr = true;
    e->ADDR1.mode = eRWreadBit;

    SETF(rIR, I_ZERO);      // assume all Y-bit2 == 0
    CLRF(rIR, I_TRUNC);     // assume N1 <= N2
    
    bool bR = false; // result bit
    
    int i = 0;
    for(i = 0 ; i < min(e->N1, e->N2) ; i += 1)
    {
        bool b1 = EISgetBitRW(&e->ADDR1);  // read w/ addt incr from src 1
        
        e->ADDR2.incr = false;
        e->ADDR2.mode = eRWreadBit;
        bool b2 = EISgetBitRW(&e->ADDR2);  // read w/ no addr incr from src2 to in anticipation of a write
        
        if (b2)
            CLRF(rIR, I_ZERO);
        
        if (!b1 && !b2)
            bR = B5;
        else if (!b1 && b2)
            bR = B6;
        else if (b1 && !b2)
            bR = B7;
        else if (b1 && b2)
            bR = B8;
        
        // write out modified bit
        e->ADDR2.bit = bR;              // set bit contents to write
   
        e->ADDR2.incr = true;           // we want address incrementing
        e->ADDR2.mode = eRWwriteBit;    // we want to write the bit
        EISgetBitRW(&e->ADDR2);            // write bit w/ addr increment to memory
    }
    
    if (e->N1 < e->N2)
    {
        for(; i < e->N2 ; i += 1)
        {
            bool b1 = e->F;
            
            e->ADDR2.incr = false;
            e->ADDR2.mode = eRWreadBit;
            bool b2 = EISgetBitRW(&e->ADDR2); // read w/ no addr incr from src2 to in anticipation of a write
            
            if (b1)
                CLRF(rIR, I_ZERO);
            
            if (!b1 && !b2)
                bR = B5;
            else if (!b1 && b2)
                bR = B6;
            else if (b1 && !b2)
                bR = B7;
            else if (b1 && b2)
                bR = B8;
            
            // write out modified bit
            e->ADDR2.bit = bR;
            
            e->ADDR2.mode = eRWwriteBit;
            e->ADDR2.incr = true;
            EISgetBitRW(&e->ADDR2);
        }
    } else if (e->N1 > e->N2)
    {
        // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not processed and the truncation indicator is set ON.
        //
        // If T = 1 and the truncation indicator is set ON by execution of the instruction, then a truncation (overflow) fault occurs.
        
        SETF(rIR, I_TRUNC);
        if (e->T)
        {
            // TODO: enable when things are working
            // doFault(ins, overflow_fault, 0, "csl truncation fault");
            sim_printf("fault: 0 0 'csl truncation fault'\n");
        }
    }
}


/*
 * return B (bit position), C (char position) and word offset given:
 *  'length' # of bits, etc ....
 *  'initC' initial char position (C)
 *  'initB' initial bit position
 */
PRIVATE
void getBitOffsets(int length, int initC, int initB, int *nWords, int *newC, int *newB)
{
    if (length == 0)
        return;
    
    int endBit = (length + 9 * initC + initB - 1) % 36;
    
    int numWords = length / 36;  ///< how many additional words will the bits take up?
    
    if (numWords > 1)          // more that the 1 word needed?
        *nWords = numWords-1;  // # of additional words
    else
        *nWords = 0;    // no additional words needed
    
    *newC = endBit / 9; // last character number
    *newB = endBit % 9; // last bit number
}

void csr(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For i = bits 1, 2, ..., minimum (N1,N2)
    //   m = C(Y-bit1)N1-i || C(Y-bit2)N2-i (a 2-bit number)
    //   C(BOLR)m → C( Y-bit2)N2-i
    // If N1 < N2, then for i = N1+i, N1+2, ..., N2
    //   m = C(F) || C(Y-bit2)N2-i (a 2-bit number)
    //    C(BOLR)m → C( Y-bit2)N2-i
    // INDICATORS: (Indicators not listed are not affected)
    //     Zero If C(Y-bit2) = 00...0, then ON; otherwise OFF
    //     Truncation If N1 > N2, then ON; otherwise OFF
    //
    // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not processed and the truncation indicator is set ON.
    //
    // If T = 1 and the truncation indicator is set ON by execution of the instruction, then a truncation (overflow) fault occurs.
    //
    // BOLR
    // If first operand    and    second operand    then result
    // bit is:                    bit is:           is from bit:
    //        0                          0                      5
    //        0                          1                      6
    //        1                          0                      7
    //        1                          1                      8
    //
    // The Boolean operations most commonly used are
    //                  BOLR Field Bits
    // Operation        5      6      7      8
    //
    // MOVE             0      0      1      1
    // AND              0      0      0      1
    // OR               0      1      1      1
    // NAND             1      1      1      0
    // EXCLUSIVE OR     0      1      1      0
    // Clear            0      0      0      0
    // Invert           1      1      0      0
    //
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseBitstringOperandDescriptor(1, e);
    parseBitstringOperandDescriptor(2, e);
    
    e->ADDR1.cPos = e->C1;
    e->ADDR2.cPos = e->C2;
    
    e->ADDR1.bPos = e->B1;
    e->ADDR2.bPos = e->B2;
    
    // get new char/bit offsets
    int numWords1=0, numWords2=0;
    
    getBitOffsets(e->N1, e->C1, e->B1, &numWords1, &e->ADDR1.cPos, &e->ADDR1.bPos);
    e->ADDR1.address += numWords1;
        
    getBitOffsets(e->N2, e->C2, e->B2, &numWords2, &e->ADDR2.cPos, &e->ADDR2.bPos);
    e->ADDR2.address += numWords2;
    
    e->F = (bool)bitfieldExtract36(e->op0, 35, 1);   // fill bit
    e->T = (bool)bitfieldExtract36(e->op0, 26, 1);   // T (enablefault) bit
    
    e->BOLR = (int)bitfieldExtract36(e->op0, 27, 4);  // BOLR field
    bool B5 = (bool)((e->BOLR >> 3) & 1);
    bool B6 = (bool)((e->BOLR >> 2) & 1);
    bool B7 = (bool)((e->BOLR >> 1) & 1);
    bool B8 = (bool)( e->BOLR      & 1);
    
    
    e->ADDR1.decr = true;
    e->ADDR1.mode = eRWreadBit;
    
    SETF(rIR, I_ZERO);      // assume all Y-bit2 == 0
    CLRF(rIR, I_TRUNC);     // assume N1 <= N2
    
    bool bR = false; // result bit
    
    int i = 0;
    for(i = 0 ; i < min(e->N1, e->N2) ; i += 1)
    {
        bool b1 = EISgetBitRW(&e->ADDR1);  // read w/ addt incr from src 1
        
        e->ADDR2.decr = false;
        e->ADDR2.mode = eRWreadBit;
        bool b2 = EISgetBitRW(&e->ADDR2);  // read w/ no addr incr from src2 to in anticipation of a write
        
        if (b2)
            CLRF(rIR, I_ZERO);
        
        if (!b1 && !b2)
            bR = B5;
        else if (!b1 && b2)
            bR = B6;
        else if (b1 && !b2)
            bR = B7;
        else if (b1 && b2)
            bR = B8;
        
        // write out modified bit
        e->ADDR2.bit = bR;              // set bit contents to write
        
        e->ADDR2.decr = true;           // we want address incrementing
        e->ADDR2.mode = eRWwriteBit;    // we want to write the bit
        EISgetBitRW(&e->ADDR2);         // write bit w/ addr increment to memory
    }
    
    if (e->N1 < e->N2)
    {
        for(; i < e->N2 ; i += 1)
        {
            bool b1 = e->F;
            
            e->ADDR2.decr = false;
            e->ADDR2.mode = eRWreadBit;
            bool b2 = EISgetBitRW(&e->ADDR2); // read w/ no addr incr from src2 to in anticipation of a write
            
            if (b1)
                CLRF(rIR, I_ZERO);
            
            if (!b1 && !b2)
                bR = B5;
            else if (!b1 && b2)
                bR = B6;
            else if (b1 && !b2)
                bR = B7;
            else if (b1 && b2)
                bR = B8;
            
            // write out modified bit
            e->ADDR2.bit = bR;
            
            e->ADDR2.mode = eRWwriteBit;
            e->ADDR2.decr = true;
            EISgetBitRW(&e->ADDR2);
        }
    } else if (e->N1 > e->N2)
    {
        // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not processed and the truncation indicator is set ON.
        //
        // If T = 1 and the truncation indicator is set ON by execution of the instruction, then a truncation (overflow) fault occurs.
        
        SETF(rIR, I_TRUNC);
        if (e->T)
        {
            // TODO enable when things are working
            //doFault(ins, FAULT_OFL, 0, "csr truncation fault");
            sim_printf("fault: 0 0 'csr truncation fault'\n");
        }
    }
}


void sztl(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    // For i = bits 1, 2, ..., minimum (N1,N2)
    //    m = C(Y-bit1)i-1 || C(Y-bit2)i-1 (a 2-bit number)
    //    If C(BOLR)m ≠ 0, then terminate
    // If N1 < N2, then for i = N1+i, N1+2, ..., N2
    //    m = C(F) || C(Y-bit2)i-1 (a 2-bit number)
    //    If C(BOLR)m ≠ 0, then terminate
    //
    // INDICATORS: (Indicators not listed are not affected)
    //     Zero If C(BOLR)m = 0 for all i, then ON; otherwise OFF
    //     Truncation If N1 > N2, then ON; otherwise OFF
    //
    // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not processed and the truncation indicator is set ON.
    //
    // If T = 1 and the truncation indicator is set ON by execution of the instruction, then a truncation (overflow) fault occurs.
    //
    // BOLR
    // If first operand    and    second operand    then result
    // bit is:                    bit is:           is from bit:
    //        0                          0                      5
    //        0                          1                      6
    //        1                          0                      7
    //        1                          1                      8
    //
    // The Boolean operations most commonly used are
    //                  BOLR Field Bits
    // Operation        5      6      7      8
    //
    // MOVE             0      0      1      1
    // AND              0      0      0      1
    // OR               0      1      1      1
    // NAND             1      1      1      0
    // EXCLUSIVE OR     0      1      1      0
    // Clear            0      0      0      0
    // Invert           1      1      0      0
    //
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseBitstringOperandDescriptor(1, e);
    parseBitstringOperandDescriptor(2, e);
    
    e->ADDR1.cPos = e->C1;
    e->ADDR2.cPos = e->C2;
    
    e->ADDR1.bPos = e->B1;
    e->ADDR2.bPos = e->B2;
    
    e->F = (bool)bitfieldExtract36(e->op0, 35, 1);   // fill bit
    e->T = (bool)bitfieldExtract36(e->op0, 26, 1);   // T (enablefault) bit
    
    e->BOLR = (int)bitfieldExtract36(e->op0, 27, 4);  // BOLR field
    bool B5 = (bool)((e->BOLR >> 3) & 1);
    bool B6 = (bool)((e->BOLR >> 2) & 1);
    bool B7 = (bool)((e->BOLR >> 1) & 1);
    bool B8 = (bool)( e->BOLR      & 1);
    
    e->ADDR1.incr = true;
    e->ADDR1.mode = eRWreadBit;
    e->ADDR2.incr = false;
    e->ADDR2.mode = eRWreadBit;
    
    SETF(rIR, I_ZERO);      // assume all C(BOLR) == 0
    CLRF(rIR, I_TRUNC);     // N1 >= N2
    
    bool bR = false; // result bit
    
    int i = 0;
    for(i = 0 ; i < min(e->N1, e->N2) ; i += 1)
    {
        bool b1 = EISgetBitRW(&e->ADDR1);  // read w/ addr incr from src 1
        bool b2 = EISgetBitRW(&e->ADDR2);  // read w/ addr incr from src 2
        
        if (!b1 && !b2)
            bR = B5;
        else if (!b1 && b2)
            bR = B6;
        else if (b1 && !b2)
            bR = B7;
        else if (b1 && b2)
            bR = B8;
        
        if (bR)
        {
            CLRF(rIR, I_ZERO);
            break;
        }
    }
    
    if (e->N1 < e->N2)
    {
        for(; i < e->N2 ; i += 1)
        {
            bool b1 = e->F;
            bool b2 = EISgetBitRW(&e->ADDR2); // read w/ addr incr from src2
                        
            if (!b1 && !b2)
                bR = B5;
            else if (!b1 && b2)
                bR = B6;
            else if (b1 && !b2)
                bR = B7;
            else if (b1 && b2)
                bR = B8;
            
            if (bR)
            {
                CLRF(rIR, I_ZERO);
                break;
            }
        }
    } else if (e->N1 > e->N2)
    {
        // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not processed and the truncation indicator is set ON.
        //
        // If T = 1 and the truncation indicator is set ON by execution of the instruction, then a truncation (overflow) fault occurs.
        
        SETF(rIR, I_TRUNC);
        if (e->T)
        {
            // XXX enable when things are working
            doFault(ins, 0, 0, "sztl truncation fault");
        }
    }
}


void sztr(DCDstruct *ins)
{
    EISstruct *e = ins->e;

    //
    // For i = bits 1, 2, ..., minimum (N1,N2)
    //   m = C(Y-bit1)N1-i || C(Y-bit2)N2-i (a 2-bit number)
    //   If C(BOLR)m ≠ 0, then terminate
    // If N1 < N2, then for i = N1+1, N1+2, ..., N2
    //   m = C(F) || C(Y-bit2)N2-i (a 2-bit number)
    //   If C(BOLR)m ≠ 0, then terminate
    //
    // INDICATORS: (Indicators not listed are not affected)
    //     Zero If C(Y-bit2) = 00...0, then ON; otherwise OFF
    //     Truncation If N1 > N2, then ON; otherwise OFF
    //
    // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not processed and the truncation indicator is set ON.
    //
    // If T = 1 and the truncation indicator is set ON by execution of the instruction, then a truncation (overflow) fault occurs.
    //
    // BOLR
    // If first operand    and    second operand    then result
    // bit is:                    bit is:           is from bit:
    //        0                          0                      5
    //        0                          1                      6
    //        1                          0                      7
    //        1                          1                      8
    //
    // The Boolean operations most commonly used are
    //                  BOLR Field Bits
    // Operation        5      6      7      8
    //
    // MOVE             0      0      1      1
    // AND              0      0      0      1
    // OR               0      1      1      1
    // NAND             1      1      1      0
    // EXCLUSIVE OR     0      1      1      0
    // Clear            0      0      0      0
    // Invert           1      1      0      0
    //
    
    setupOperandDescriptor(1, e);
    setupOperandDescriptor(2, e);
    
    parseBitstringOperandDescriptor(1, e);
    parseBitstringOperandDescriptor(2, e);
    
    e->ADDR1.cPos = e->C1;
    e->ADDR2.cPos = e->C2;
    
    e->ADDR1.bPos = e->B1;
    e->ADDR2.bPos = e->B2;
    
    // get new char/bit offsets
    int numWords1=0, numWords2=0;
    
    getBitOffsets(e->N1, e->C1, e->B1, &numWords1, &e->ADDR1.cPos, &e->ADDR1.bPos);
    e->ADDR1.address += numWords1;
    
    getBitOffsets(e->N2, e->C2, e->B2, &numWords2, &e->ADDR2.cPos, &e->ADDR2.bPos);
    e->ADDR2.address += numWords2;
    
    e->F = (bool)bitfieldExtract36(e->op0, 35, 1);   // fill bit
    e->T = (bool)bitfieldExtract36(e->op0, 26, 1);   // T (enablefault) bit
    
    e->BOLR = (int)bitfieldExtract36(e->op0, 27, 4);  // BOLR field
    bool B5 = (bool)((e->BOLR >> 3) & 1);
    bool B6 = (bool)((e->BOLR >> 2) & 1);
    bool B7 = (bool)((e->BOLR >> 1) & 1);
    bool B8 = (bool)( e->BOLR      & 1);
    
    e->ADDR1.decr = true;
    e->ADDR1.mode = eRWreadBit;
    e->ADDR2.decr = true;
    e->ADDR2.mode = eRWreadBit;
    
    SETF(rIR, I_ZERO);      // assume all Y-bit2 == 0
    CLRF(rIR, I_TRUNC);     // assume N1 <= N2
    
    bool bR = false; // result bit
    
    int i = 0;
    for(i = 0 ; i < min(e->N1, e->N2) ; i += 1)
    {
        bool b1 = EISgetBitRW(&e->ADDR1);  // read w/ addr incr from src 1
        bool b2 = EISgetBitRW(&e->ADDR2);  // read w/ addr incr from src 2
        
        if (!b1 && !b2)
            bR = B5;
        else if (!b1 && b2)
            bR = B6;
        else if (b1 && !b2)
            bR = B7;
        else if (b1 && b2)
            bR = B8;
        
        if (bR)
        {
            CLRF(rIR, I_ZERO);
            break;
        }
    }
    
    if (e->N1 < e->N2)
    {
        for(; i < e->N2 ; i += 1)
        {
            bool b1 = e->F;
            bool b2 = EISgetBitRW(&e->ADDR2); // read w/ no addr incr from src2 to in anticipation of a write
            
            if (!b1 && !b2)
                bR = B5;
            else if (!b1 && b2)
                bR = B6;
            else if (b1 && !b2)
                bR = B7;
            else if (b1 && b2)
                bR = B8;
            
            if (bR)
            {
                CLRF(rIR, I_ZERO);
                break;
            }
        }
    } else if (e->N1 > e->N2)
    {
        // NOTES: If N1 > N2, the low order (N1-N2) bits of C(Y-bit1) are not processed and the truncation indicator is set ON.
        //
        // If T = 1 and the truncation indicator is set ON by execution of the instruction, then a truncation (overflow) fault occurs.
        
        SETF(rIR, I_TRUNC);
        if (e->T)
        {
            // XXX enable when things are working
            doFault(ins, overflow_fault, 0, "sztr truncation fault");
        }
    }
}

/*
 * EIS decimal arithmetic routines are to be found in dps8_decimal.c .....
 */

