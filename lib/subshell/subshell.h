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

#include <stdlib.h>
#include <string.h>
#include <Stream.h>

enum Subshell_Status 
{
	SUBSHELL_REALLOC_FAILURE = -2,
	SUBSHELL_SNPRINTF_FAILURE,
	SUBSHELL_SUCCESS
};

class Subshell
{
public: 
	Subshell(Stream &ioStream);
	~Subshell();         

	void newCmd(const char *cmdName, const char *cmdHelp, void (*cmdFunction)(void *), void *cmdArg);

	void run(void);
	void listCmds(void);
	void showHelp(void);
	bool getArg(char **cmdArg); 

	Subshell_Status status = SUBSHELL_SUCCESS;

private:
	void init(void);
	void parseCmdString(void);

	char *tokenize(char *inputString);
	
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
};
