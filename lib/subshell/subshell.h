/**
 * @file subshell.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief libSubshell
 * @version 0.9
 * @date 2021-08-09
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#pragma once

#include <WProgram.h>
#include <Stream.h>
#include <string.h>

enum Subshell_Status 
{
	SUBSHELL_REALLOC_FAILURE = -2,
	SUBSHELL_SNPRINTF_FAILURE = -1,
	SUBSHELL_SUCCESS

};

class Subshell
{
public:
	Subshell(Stream &ioStream);              

	Subshell_Status newCmd(const char *cmdName, const char *cmdHelp, void (*cmdFunction)(void *), void *cmdArg);
	Subshell_Status setPrompt(const char *shellPrompt, void (*cmdFunction)(void *), void *cmdArg);

	void checkStream(void);
	void listCmds(void);
	void showCmdHelp(void);
	char *getArg(void); 

private:
	void init(void);
	void parseCmdString(void);

	char *tokenize(char *inputString, char **scratchPad);
	char *scratchPad; // Scratchpad for holding state in between calls to tokenize

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
	
	command_t *cmd; 
	uint8_t numCmds;
	uint8_t cmdIndex;
};
