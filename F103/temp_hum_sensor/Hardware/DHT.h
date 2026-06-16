#ifndef _DHT_H
#define _DHT_H
#include "Header.h"
void DHT_out(void);
void DHT_in(void);
void DHT_start(void);
uint8_t  DHT_read(void);
void DHT_R_data(void);
extern uint8_t R_data[4];
#endif
