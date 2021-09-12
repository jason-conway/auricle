/**
 * @file subshell.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief 
 * @version 0.1
 * @date 2021-08-09
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#pragma once

#include "auricle.h"
#include <Stream.h>
#include <string.h>

class Subshell
{
public:
	Subshell(Stream &ioStream);              
	
	void newCmd(const char *cmdName, const char *cmdHelp, void (*cmdFunction)(void *), void *cmdArg);
	void checkStream(void);
	void listCmds(void);
	void showCmdHelp(void);
	char *getArg(void); 

private:
	void init(void);
	void parseCmdString(void);

	char *tokenize(char *inputString, char **scratchPad);
	char *scratchPad = nullptr; // Scratchpad for holding state

	Stream *stream;

	enum strLengths
	{
		bufferLength = 32,
		commandLength = 24,
		helpLength = 64
	};

	char strBuffer[bufferLength]; 
	uint8_t strBufferIndex;   
	
	struct command_t
	{
		char cmdName[commandLength]; 
		char cmdHelp[helpLength];
		void (*cmdFunction)(void*);
		void *cmdArg;
	};
	
	command_t *cmd = nullptr; 
	uint8_t numCmds;
	uint8_t cmdIndex;
};
