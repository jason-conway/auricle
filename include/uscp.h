/**
 * @file uscp.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief 
 * @version 0.1
 * @date 2021-08-09
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#ifndef USCP_H
#define USCP_H

#include "auricle.h"
#include <Stream.h>
#include <string.h>

class USCP
{
public:
    USCP(void);              
    
    void newCmd(const char *cmdName, void (*cmdFunction)(void *), void *cmdArg);
    void checkStream(void);
    void listCmds(void);
    char *getArg(void); 

protected:

private:
    void init(void);
    char *tokenize(char *__restrict inputString, char **__restrict scratchPad);
    void parseCmdString(void);

    char *scratchPad = NULL; // Scratchpad for tokenize
    
    char streamBuffer[64]; 
    uint8_t streamBufferIndex = 0;   
    
    struct command_t
    {
        char cmdName[16]; 
        void (*cmdFunction)(void*);
        void *cmdArg;
    };
    
    command_t *cmd; 
    uint8_t numCmds = 0;
};

#endif
