struct modificationContinuation
{
    bool bActive;   // if true then continuation is active and needs to be considered
    int segment;    // segment of whatever we want to write
    int address;    // address of whatever we'll need to write
    int tally;      // value of tally from dCAF()
    int delta;      // value of delta from sCAF()
    int mod;        // which address modification are we continuing
    int tb;         // character size flag, tb, with the value 0 indicating 6-bit characters and the value 1 indicating 9-bit bytes.
    int cf;         // 3-bit character/byte position value,
    word36 indword; // indirect word
    int tmp18;      // temporary address used by some instructions
};
typedef struct modificationContinuation modificationContinuation;

t_stat doComputedAddressFormation (void);
extern modificationContinuation _modCont, *modCont;
void doComputedAddressContinuation (void);


