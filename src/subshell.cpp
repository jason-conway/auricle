/**
 * @file subshell.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief USB Serial Command Parser
 * @version 0.1
 * @date 2021-08-09
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "subshell.h"

Subshell::Subshell(Stream &ioStream)
{
	stream = &ioStream;
	init();
}

void Subshell::init(void)
{
	memset(strBuffer, 0, sizeof(strBuffer));
	strBufferIndex = 0;
	numCmds = 0;
	cmdIndex = -1;
}

/**
 * @brief Add a name, help / info, function pointer, and function argument set to the command table
 * 
 * @param[in] cmdName Name tied to the command to be created
 * @param[in] cmdHelp Null-terminated help string 
 * @param[in] cmdFunction Function to be called upon receiving the command
 * @param[in] cmdArg Argument to be passed to cmdFunction
 */
void Subshell::newCmd(const char *cmdName, const char *cmdHelp, void (*cmdFunction)(void *), void *cmdArg)
{
	if (command_t *cmdPtr = static_cast<command_t *>(realloc(cmd, (numCmds + 1) * sizeof(command_t))))
	{
		cmd = cmdPtr; // Update cmd with the new address
	}
	else
	{
		free(cmd); // realloc failed to allocate new memory but didn't deallocate the original memory
		stream->printf("Error: realloc failure\r\n");
		return;
	}

	if (snprintf(cmd[numCmds].cmdName, commandLength, "%s", cmdName) < 0)
	{
		stream->printf("Error: snprintf failure\r\n");
		return;
	}
	
	if (snprintf(cmd[numCmds].cmdHelp, helpLength, "%s", cmdHelp) < 0)
	{
		stream->printf("Error: snprintf failure\r\n");
		return;
	}

	cmd[numCmds].cmdFunction = cmdFunction;
	cmd[numCmds].cmdArg = cmdArg;
	numCmds++;
}

/**
 * @brief Gets a string from the serial terminal. Echos back characters while typing and handles backspaces.
 * Calls parser once the string is complete.
 *
 * https://en.wikipedia.org/wiki/ANSI_escape_code
 * https://en.wikipedia.org/wiki/C0_and_C1_control_codes
 * 
 */
void Subshell::checkStream(void)
{
	bool EOL = false; // Flag is set true when \r or \n is found
	while (stream->available())
	{
		char serialChar = static_cast<char>(stream->read());

		// Backspace
		if (serialChar == '\b')
		{
			if (strBufferIndex > 0) // Something backspace-able
			{
				// Chop last char from array and move index back
				strBuffer[strBufferIndex--] = '\0';

				stream->printf("%c", '\b'); // Move left
				stream->printf("%c", ' ');	// Overwrite char with a space
				stream->printf("%c", '\b'); // Move back left
				stream->flush();
			}
			continue; // Don't add \b to the string buffer
		}

		// Echo printable characters
		if (((unsigned)serialChar - 0x20) < 0x5F)
		{
			stream->printf("%c", serialChar);
		}

		// Line feed
		if (serialChar == '\n')
		{
			// Set flag, print EOL sequence, and exit loop
			EOL = true;
			stream->printf("\r\n");
			break;
		}

		// Carriage return or carriage return + line feed combo
		if (serialChar == '\r')
		{
			EOL = true;
			stream->printf("\r\n");
			if (stream->available())
			{
				if (stream->peek() == '\n') // CR+LF
				{
					stream->read();
				}
			}
			break;
		}

		strBuffer[strBufferIndex++] = serialChar;
		strBuffer[strBufferIndex] = '\0';
	}

	if (EOL)
	{
		this->parseCmdString();
		strBuffer[0] = '\0';
		strBufferIndex = 0;
	}
}

/**
 * @brief Break string apart at the spaces and check if the first word is a known command. Call the associated function 
 * if known, otherwise call the function associated with cmd[1]. 
 * 
 */
void Subshell::parseCmdString(void)
{
	if (char *command = this->tokenize(strBuffer, &scratchPad))
	{
		bool cmdUnknown = true;
		cmdIndex = 0;
		for (size_t i = 2; i < numCmds; i++) // cmd[1] is the default, cmd[0] always runs
		{
			if (strncmp(command, cmd[i].cmdName, commandLength) == 0)
			{
				cmdIndex = i; // cmdIndex needs set before calling the function in case of help argument
				(cmd[i].cmdFunction)(cmd[i].cmdArg);
				cmdUnknown = false;
				break;
			}
		}
		if (cmdUnknown)
		{
			(cmd[1].cmdFunction)(cmd[1].cmdArg); // cmd[1] set as an unknown command handler
		}
	}
	(cmd[0].cmdFunction)(cmd[0].cmdArg); // cmd[0] set to print hostname
}

/**
 * @brief Return the command arguments if there are any
 * 
 * @return char* 
 */
char *Subshell::getArg(void)
{
	return this->tokenize(NULL, &scratchPad);
}

/**
 * @brief Print registered commands
 * 
 */
void Subshell::listCmds(void)
{
	stream->printf("Available Commands: \r\n");
	for (size_t i = 2; i < numCmds; i++)
	{
		stream->printf("%s\r\n", cmd[i].cmdName);
	}
}

/**
 * @brief Print command help for the previously matched command
 * 
 */
void Subshell::showCmdHelp(void)
{
	if (cmdIndex) // Defaults 0, set on match
	{
		stream->printf("%s\r\n", cmd[cmdIndex].cmdHelp);
	}
}

/**
 * @brief Thread-safe strtok clone... strtok_r but without custom delimitors?
 * 
 * @param[in] inputString 
 * @param[in] scratchPad 
 * @return char* 
 */
char *Subshell::tokenize(char *inputString, char **scratchPad)
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
