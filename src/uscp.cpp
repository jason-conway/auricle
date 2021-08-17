/**
 * @file uscp.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief USB Serial Command Parser
 * @version 0.1
 * @date 2021-08-09
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "uscp.h"

USCP::USCP()
{
	init();
}

void USCP::init(void)
{
	memset(streamBuffer, 0, sizeof(streamBuffer));
	SerialUSB.setTimeout(10000);
}

/**
 * @brief Add a name and function pointer pair to the command table
 * 
 * @param cmdName 
 * @param cmdFunction 
 * @param cmdArg 
 */
void USCP::newCmd(const char *cmdName, void (*cmdFunction)(void *), void *cmdArg)
{
	cmd = (command_t *)realloc(cmd, (numCmds + 1) * sizeof(command_t));

	strncpy(cmd[numCmds].cmdName, cmdName, 24);

	cmd[numCmds].cmdFunction = cmdFunction;
	cmd[numCmds].cmdArg = cmdArg;
	numCmds++;
}

/**
 * @brief 
 * 
 */
void USCP::checkStream()
{
	while (usb_serial_available() > 0)
	{
		size_t charactersRead = (SerialUSB.readBytesUntil('\n', streamBuffer, 32));
		if (charactersRead > 0)
		{
			if (streamBuffer[charactersRead - 1] == '\r');
			{
				streamBuffer[charactersRead - 1] = '\0';
			}

			if (!(SerialUSB.getReadError()))
			{
				parseCmdString();
			}
			else
			{
				SerialUSB.clearReadError();
			}
		}
	}
}

/**
 * @brief 
 * 
 */
void USCP::parseCmdString(void)
{
	char *command = tokenize(streamBuffer, &scratchPad);
	if (command != NULL)
	{
		bool cmdKnown = false;
		for (size_t i = 2; i < numCmds; i++) // cmd[0] is the default, cmd[1] always runs
		{
			if (strncmp(command, cmd[i].cmdName, 24) == 0)
			{
				(cmd[i].cmdFunction)(cmd[i].cmdArg);
				cmdKnown = true;
				break;
			}
		}
		if (!(cmdKnown))
		{
			(cmd[1].cmdFunction)(cmd[1].cmdArg);
		}
	}

	memset(streamBuffer, 0, sizeof(streamBuffer));
	streamBufferIndex = 0;
	(cmd[0].cmdFunction)(cmd[0].cmdArg);
}

/**
 * @brief Return the command arguments if there are any
 * 
 * @return char* 
 */
char *USCP::getArg(void)
{
	return tokenize(NULL, &scratchPad);
}

/**
 * @brief 
 * 
 */
void USCP::listCmds(void)
{
	SerialUSB.printf("Available Commands: \n");
	for (size_t i = 3; i < numCmds; i++)
	{
		SerialUSB.printf("%s\n", cmd[i].cmdName);
	}
}

/**
 * @brief Thread-safe strtok clone... strtok_r but without custom delimitors?
 * 
 * @param inputString 
 * @param spaceString 
 * @param scratchPad 
 * @return char* 
 */
char *USCP::tokenize(char *__restrict inputString, char **__restrict scratchPad)
{
	if (!(inputString) && (!(inputString = *scratchPad)))
	{
		return NULL;
	}

	inputString = inputString + strspn(inputString, " ");
	if (!(*inputString))
	{
		return *scratchPad = 0;
	}

	*scratchPad = inputString + strcspn(inputString, " ");
	if (**scratchPad)
	{
		*(*scratchPad)++ = 0;
	}
	else
	{
		*scratchPad = 0;
	}
	return inputString;
}
