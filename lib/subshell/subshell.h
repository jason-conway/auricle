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
#include <usb_serial.h>
#include <stdio.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif
	int _write(int FILE, char *writeBuffer, int writeBufferLength);
	bool _available(void);
	int _getchar(void);
	int _peekchar(void);

	void initSubshell(void);
	void newCmd(const char *cmdName, const char *cmdHelp, void (*cmdFunction)(void *));
	void run(void);
	void listCmds(void);
	void showHelp(void);
	bool getArg(char **cmdArg);
#ifdef __cplusplus
}
#endif
