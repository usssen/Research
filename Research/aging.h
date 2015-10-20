#ifndef AGING_H
#define AGING_H
#include"typedef.h"

//DCC本身的delay
//50%的DCC(就只是個buffer)

double AgingRate(AGINGTYPE status, int year);
void ReadAgingData();

#endif