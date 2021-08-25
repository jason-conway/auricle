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

void __attribute__((section(".flashmem"))) USCP::init(void)
{
	memset(strBuffer, 0, sizeof(strBuffer));
}

/**
 * @brief Add a name and function pointer pair to the command table
 * 
 * @param cmdName Name tied to the command created
 * @param cmdFunction Function to be called upon receiving the command
 * @param cmdArg Argument to be passed to cmdFunction
 */
void __attribute__((section(".flashmem"))) USCP::newCmd(const char *cmdName, void (*cmdFunction)(void *), void *cmdArg)
{
	if (command_t *cmdPtr = static_cast<command_t *>(realloc(cmd, (numCmds + 1) * sizeof(command_t))))
	{
		cmd = cmdPtr; // Update cmd with the new address
	}
	else
	{
		free(cmd); // realloc failed to allocate new memory but didn't deallocate the original memory
		SerialUSB.printf("Error: realloc failure\r\n");
		return;
	}

	if (snprintf(cmd[numCmds].cmdName, commandLength, "%s", cmdName) < 0)
	{
		SerialUSB.printf("Error: snprintf failure\r\n");
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
void __attribute__((section(".flashmem"))) USCP::checkStream(void)
{
	bool EOL = false;
	while (usb_serial_available())
	{
		char serialChar = static_cast<char>(usb_serial_getchar());

		// Backspace
		if (serialChar == '\b')
		{
			if (strBufferIndex > 0) // Something backspace-able
			{
				// Chop last char from array and move index back
				strBuffer[strBufferIndex--] = '\0';

				SerialUSB.printf("%c", '\b'); // Move left
				SerialUSB.printf("%c", ' ');  // Overwrite char with a space
				SerialUSB.printf("%c", '\b'); // Move back left
				usb_serial_flush_output();
			}
			continue; // Don't add \b to the string buffer
		}

		// Echo printable characters
		if (((unsigned)serialChar - 0x20) < 0x5F)
		{
			SerialUSB.printf("%c", serialChar);
		}

		// Line feed
		if (serialChar == '\n')
		{
			// Set flag, print EOL sequence, and exit loop
			EOL = true;
			SerialUSB.printf("\r\n");
			break;
		}

		// Carriage return or carriage return + line feed combo
		if (serialChar == '\r')
		{
			EOL = true;
			SerialUSB.printf("\r\n");
			if (usb_serial_available())
			{
				if (usb_serial_peekchar() == '\n') // CR+LF
				{
					usb_serial_getchar();
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
void __attribute__((section(".flashmem"))) USCP::parseCmdString(void)
{
	char *command = this->tokenize(strBuffer, &scratchPad);
	if (command != NULL)
	{
		bool cmdKnown = false;
		for (size_t i = 2; i < numCmds; i++) // cmd[0] is the default, cmd[1] always runs
		{
			if (strncmp(command, cmd[i].cmdName, commandLength) == 0)
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
	(cmd[0].cmdFunction)(cmd[0].cmdArg);
}

/**
 * @brief Return the command arguments if there are any
 * 
 * @return char* 
 */
char __attribute__((section(".flashmem"))) *USCP::getArg(void)
{
	return this->tokenize(NULL, &scratchPad);
}

/**
 * @brief Print registered commands
 * 
 */
void __attribute__((section(".flashmem"))) USCP::listCmds(void)
{
	SerialUSB.printf("Available Commands: \r\n");
	for (size_t i = 2; i < numCmds; i++)
	{
		SerialUSB.printf("%s\r\n", cmd[i].cmdName);
	}
}

/**
 * @brief Thread-safe strtok clone... strtok_r but without custom delimitors?
 * 
 * @param inputString 
 * @param scratchPad 
 * @return char* 
 */
char __attribute__((section(".flashmem"))) *USCP::tokenize(char *__restrict inputString, char **__restrict scratchPad)
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
