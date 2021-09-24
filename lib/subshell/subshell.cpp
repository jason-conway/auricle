/**
 * @file subshell.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief libSubshell
 * @version 0.9
 * @date 2021-08-09
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "subshell.h"

/**
 * @brief Construct a new Subshell:: Subshell object
 * 
 * @param[in] ioStream Stream class instance
 */
Subshell::Subshell(Stream &ioStream)
{
	stream = &ioStream;
	init();
}

/**
 * @brief Free any memory that may have been allocated
 * 
 */
Subshell::~Subshell()
{
	if (cmd)
	{
		free(cmd);
	}
}

/**
 * @brief Initialize class variables
 * 
 */
void Subshell::init(void)
{
	cmd = nullptr;
	numCmds = 0;

	memset(strBuffer, 0, sizeof(strBuffer));
	strBufferIndex = 0;
}

/**
 * @brief Add a name, help / info, function pointer, and function argument set to the command table
 * 
 * @param[in] cmdName Name tied to the command to be created
 * @param[in] cmdHelp Null-terminated help / description string
 * @param[in] cmdFunction Function to be called upon receiving the command
 * @param[in] cmdArg Argument to be passed to cmdFunction
 * @return None
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
		status = SUBSHELL_REALLOC_FAILURE;
		return;
	}

	if (snprintf(cmd[numCmds].cmdName, commandLength, "%s", cmdName) < 0)
	{
		status = SUBSHELL_SNPRINTF_FAILURE;
		return;
	}

	if (snprintf(cmd[numCmds].cmdHelp, helpLength, "%s", cmdHelp) < 0)
	{
		status = SUBSHELL_SNPRINTF_FAILURE;
		return;
	}

	cmd[numCmds].cmdFunction = cmdFunction;
	cmd[numCmds].cmdArg = cmdArg;
	numCmds++;
}

/**
 * @brief Main loop
 */
void Subshell::run(void)
{
	bool EOL = false; // End-of-line flag: true when CR or LF are read from stream buffer
	while (stream->available())
	{
		char serialChar = static_cast<char>(stream->read());

		if (serialChar == '\b') // Backspace
		{
			if (strBufferIndex > 0) // strBuffer has characters to be backspaced
			{
				// Chop last char from array and move index back
				strBuffer[strBufferIndex--] = '\0';
				const char delSeq[] = {'\b', ' ', '\b'};
				stream->printf("%s", delSeq);
			}
			continue; // Don't add \b to the string buffer
		}

		if (serialChar == '\n') // Line feed
		{
			// Set flag, print EOL sequence, and exit loop
			EOL = true;
			stream->printf("\r\n");
			break;
		}

		if (serialChar == '\r') // Carriage return or carriage return + line feed combo
		{
			EOL = true;
			stream->printf("\r\n");

			if (stream->available())
			{
				if (stream->peek() == '\n') // CR+LF
				{
					stream->read(); // Pull LF from the RX buffer
				}
			}
			break;
		}

		// Echo printable characters
		if (((unsigned)serialChar - 0x20) < 0x5F)
		{
			stream->write(serialChar);
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
 * if known, otherwise call the default function at cmd[1]
 * 
 */
void Subshell::parseCmdString(void)
{
	if (char *command = this->tokenize(strBuffer))
	{
		bool cmdUnknown = true;
		for (size_t i = 2; i < numCmds; i++) // cmd[1] is the default, cmd[0] always runs
		{
			if (strncmp(command, cmd[i].cmdName, commandLength) == 0)
			{
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
	(cmd[0].cmdFunction)(cmd[0].cmdArg); // cmd[0] set to print shell prompt
}

/**
 * @brief Get a tokenized command argument as a C string. If no arguments were entered, the input pointer remains unchanged.
 * 
 * @param[inout] cmdArg Pointer to the argument string
 * @return Returns true if an argument was present, otherwise false
 */
bool Subshell::getArg(char **cmdArg)
{
	if (char *arg = this->tokenize(nullptr))
	{
		*cmdArg = arg;
		return true;
	}
	return false;
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
 * @brief 
 * 
 */
void Subshell::showHelp(void)
{
	if (char *arg = this->tokenize(nullptr))
	{
		bool invalidCmd = true;
		for (size_t i = 2; i < numCmds; i++) // cmd[1] is the default, cmd[0] always runs
		{
			if (strncmp(arg, cmd[i].cmdName, commandLength) == 0)
			{
				stream->printf("%s\r\n", cmd[i].cmdHelp);
				invalidCmd = false;
				break;
			}
		}
		if (invalidCmd)
		{
			stream->printf("Unknown command: %s\r\n", arg);
		}
	}
	else
	{
		stream->printf("Error: Invalid Syntax\r\n");
	}
}

/**
 * @brief Turns C strings into space-seperated tokens
 *
 * @param[in] tokenStart C string to be tokenized at the expense of getting clobbered 
 * @return Returns pointer to a null-terminated token from tokenStart
 */
char *Subshell::tokenize(char *tokenStart)
{
	static char *tokenEnd; // Hold pointer to previous token between calls

	if (!(tokenStart)) // tokenStart is null
	{
		if (!(tokenStart = tokenEnd)) // Set new starting point
		{
			return nullptr; // tokenEnd is null
		}
	}

	tokenStart = tokenStart + strspn(tokenStart, " "); // Characters until a space
	if (!(*tokenStart)) // Pointing at a null character
	{
		return tokenEnd = nullptr; // Set end pointer null and return
	}

	tokenEnd = tokenStart + strcspn(tokenStart, " "); // Number of characters until next space
	if (*tokenEnd) // End of token is not null so replace the space with null-terminator
	{
		*tokenEnd++ = '\0'; 
	}
	else // End of the string
	{
		tokenEnd = nullptr;
	}

	return tokenStart; 
}
