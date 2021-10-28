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

enum StrLengths
{
	cmdStrLength = 64,
	helpStrLength = 64,
	maxStrLength = cmdStrLength + helpStrLength,
};

/**
 * @brief Command type implementation
 * 
 */
typedef struct command_t
{
	char cmdName[cmdStrLength];
	char cmdHelp[helpStrLength];
	void (*cmdFunction)(void *);
	void *cmdArg;
} command_t;

/**
 * @brief Command and command count
 * 
 */
typedef struct shell_t
{
	command_t *cmd;
	size_t numCmds;
} shell_t;

/**
 * @brief Small C string type
 * 
 */
typedef struct string_t
{
	char cStr[maxStrLength];
	uint8_t cIndex;
} string_t;

shell_t shell;
string_t str;

void initSubshell(void)
{
	shell.cmd = NULL;
	shell.numCmds = 0;
	str.cStr[0] = '\0';
	str.cIndex = 0;
}

void printError(const char *cmdName, const char *cmdError)
{
	printf("Subshell Error\r\nCommand Name: %s\r\nError: %s\r\n", cmdName, cmdError);
}

/**
 * @brief Internal function to implement the addition of a new command
 *
 * @param[in] cmdName Name tied to the command to be created
 * @param[in] cmdHelp Null-terminated help / description string
 * @param[in] cmdFunction Function to be called upon receiving the command
 * @param[in] cmdArg nullptr
 * @return None
 */
bool _newCmd(const char *cmdName, const char *cmdHelp, void (*cmdFunction)(void *))
{
	// Increase the size of cmd by sizeof(command_t)
	command_t *cmd = NULL;
	if ((cmd = (command_t *)realloc(shell.cmd, (shell.numCmds + 1) * sizeof(command_t))))
	{
		shell.cmd = cmd; // Update cmd with the new address
	}
	else
	{
		printError(cmdName, "realloc failure");
		return false;
	}

	if (snprintf(shell.cmd[shell.numCmds].cmdName, cmdStrLength, "%s", cmdName) < 0)
	{
		printError(cmdName, "snprintf failure");
		return false;
	}

	if (snprintf(shell.cmd[shell.numCmds].cmdHelp, helpStrLength, "%s", cmdHelp) < 0)
	{
		printError(cmdName, "snprintf failure");
		return false;
	}

	shell.cmd[shell.numCmds].cmdFunction = cmdFunction;
	shell.cmd[shell.numCmds].cmdArg = NULL;
	shell.numCmds++;

	return true;
}

/**
 * @brief Add a name, help / info, function pointer, and function argument set to the command table
 *
 * @param[in] cmdName Name tied to the command to be created
 * @param[in] cmdHelp Null-terminated help / description string
 * @param[in] cmdFunction Function to be called upon receiving the command
 * @return None
 */
void newCmd(const char *cmdName, const char *cmdHelp, void (*cmdFunction)(void *))
{
	if (shell.numCmds == 0 && !(strncmp(cmdName, "shPrompt", cmdStrLength) == 0))
	{
		printError(cmdName, "First command must have name 'shPrompt'");
		exit(EXIT_FAILURE);
	}
	if (shell.numCmds == 1 && !(strncmp(cmdName, "unkCmd", cmdStrLength) == 0))
	{
		printError(cmdName, "Second command must have name 'unkCmd'");
		exit(EXIT_FAILURE);
	}
	if (!(_newCmd(cmdName, cmdHelp, cmdFunction)))
	{
		free(shell.cmd); // realloc failed to allocate new memory but didn't deallocate the original memory
		exit(EXIT_FAILURE);
	}
}

/**
 * @brief Turns C strings into space-seperated tokens
 *
 * @param[in] strStart C string to be tokenized at the expense of getting clobbered
 * @return Returns pointer to a null-terminated token from strStart
 */
char *tokenize(char *strStart)
{
	static char *tokenEnd; // Hold pointer to previous token between calls

	if (!(strStart)) // strStart is null
	{
		if (!(strStart = tokenEnd)) // Set new starting point
		{
			return NULL; // tokenEnd is null
		}
	}

	strStart = strStart + strspn(strStart, " "); // Characters until a space
	if (!(*strStart))							 // Pointing at a null character
	{
		return tokenEnd = NULL; // Set end pointer null and return
	}

	tokenEnd = strStart + strcspn(strStart, " "); // Number of characters until next space
	if (*tokenEnd)								  // End of token is not null so replace the space with null-terminator
	{
		*tokenEnd++ = '\0';
	}
	else // End of the string
	{
		tokenEnd = NULL;
	}

	return strStart;
}

/**
 * @brief Break string apart at the spaces and check if the first word is a known command. Call the associated function
 * if known, otherwise call the default function at cmd[1]
 *
 */
void parseCmdString(void)
{
	char *command = tokenize(str.cStr);
	if (command)
	{
		bool cmdUnknown = true;
		for (size_t i = 2; i < shell.numCmds; i++) // cmd[1] is the default, cmd[0] always runs
		{
			if (strncmp(command, shell.cmd[i].cmdName, cmdStrLength) == 0)
			{
				(shell.cmd[i].cmdFunction)(shell.cmd[i].cmdArg);
				cmdUnknown = false;
				break;
			}
		}
		if (cmdUnknown)
		{
			(shell.cmd[1].cmdFunction)(shell.cmd[1].cmdArg); // cmd[1] set as an unknown command handler
		}
	}
	(shell.cmd[0].cmdFunction)(shell.cmd[0].cmdArg); // cmd[0] set to print shell prompt
}

/**
 * @brief Main loop
 */
void run(void)
{
	bool EOL = false; // End-of-line flag: true when CR or LF are read from stream buffer
	while (_available())
	{	
		char readChar = _getchar();
		switch (readChar)
		{
		case '\b':				 // Backspace
			if (str.cIndex > 0) // cStr has characters to be backspaced
			{
				// Chop last char from array and move index back
				str.cStr[str.cIndex--] = '\0';
				printf("\b \b");
				fflush(stdout);
			}
			break;

		case '\n': // Line feed
			EOL = true;
			printf("\r\n");
			goto jmpOut;

		case '\r': // Carriage return or carriage return + line feed combo
			EOL = true;
			printf("\r\n");

			if (_available())
			{
				if (_peekchar() == '\n') // CR+LF
				{
					_getchar(); // Pull LF from the RX buffer
				}
			}
			goto jmpOut;

		default:
			// Echo printable characters
			if ((readChar - 0x20) < 0x5F)
			{
				printf("%c", readChar);
				fflush(stdout);
			}

			str.cStr[str.cIndex++] = readChar;
			str.cStr[str.cIndex] = '\0';
			break;
		}
	}

jmpOut:
	if (EOL)
	{
		fflush(stdout);
		parseCmdString();
		str.cStr[0] = '\0';
		str.cIndex = 0;
	}
}

/**
 * @brief Get a tokenized command argument as a C string. If no arguments were entered, the input pointer remains unchanged.
 *
 * @param[inout] cmdArg Pointer to the argument string
 * @return Returns true if an argument was present, otherwise false
 */
bool getArg(char **cmdArg)
{
	char *arg = tokenize(NULL);
	if (arg)
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
void listCmds(void)
{
	printf("Available Commands: \r\n");
	for (size_t i = 2; i < shell.numCmds; i++)
	{
		printf("%s\r\n", shell.cmd[i].cmdName);
	}
}

/**
 * @brief
 *
 */
void showHelp(void)
{
	char *arg = tokenize(NULL);
	if (arg)
	{
		bool invalidCmd = true;
		for (size_t i = 2; i < shell.numCmds; i++) // cmd[1] is the default, cmd[0] always runs
		{
			if (strncmp(arg, shell.cmd[i].cmdName, cmdStrLength) == 0)
			{
				printf("%s\r\n", shell.cmd[i].cmdHelp);
				invalidCmd = false;
				break;
			}
		}
		if (invalidCmd)
		{
			printf("Unknown command: %s\r\n", arg);
		}
	}
	else
	{
		printf("Error: Invalid Syntax\r\n");
	}
}