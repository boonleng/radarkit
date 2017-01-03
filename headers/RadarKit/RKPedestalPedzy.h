//
//  RKPedestalPedestal.h
//  RadarKit
//
//  This is an example implementation of hardware interaction through RKPedestal
//
//  Created by Boon Leng Cheong on 1/3/17.
//  Copyright © 2017 Boon Leng Cheong. All rights reserved.
//

#ifndef __RadarKit_RKPedestalPedestal__
#define __RadarKit_RKPedestalPedestal__

#include <RadarKit/RKTypes.h>
#include <RadarKit/RKClient.h>

typedef struct rk_pedzy RKPedestalPedzy;

struct rk_pedzy {
    // User set variables
    char                   hostname[256];
    // RKClient
};

void *RKPedestalPedzyInit(void *);
int RKPedestalPedzyRead(void *, RKPosition *);
int RKPedestalPedzyExec(void *, const char *);
int RKPedestalPedzyFree(void *);

#endif /* __RadarKit_RKPedestal__ */
