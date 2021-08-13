/**
 * @file ash.cpp
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Auricle Shell
 * @version 0.1
 * @date 2021-08-08
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#include "ash.h"
#include "convolvIR.h"

USCP uscp;
TPD3IO tpd3io;

ASH::ASH(void)
{
	init();
}

void ASH::init(void)
{
	uscp.newCmd("\0", ASH::unknownCommand, NULL); // Default 
	uscp.newCmd("test-cmd", ASH::testFunction, NULL);
	uscp.newCmd("toggle-power", ASH::togglePower, NULL);
	uscp.newCmd("switch-input", ASH::switchInput, NULL);
	uscp.newCmd("get-status", ASH::currentStatus, NULL);
	uscp.newCmd("set-angle", ASH::setAngle, NULL);
}

void ASH::execLoop(void)
{
	uscp.checkStream();
}

void ASH::togglePower(void *)
{
	tpd3io.togglePower();
}

void ASH::setAngle(void *)
{
	char *arg = uscp.getArg();
	if (arg == NULL)
	{
		SerialUSB.printf("Incorrect syntax\n");
	}
	else
	{
		uint16_t angle = atoi(arg);
		SerialUSB.printf("New angle: %d degrees\n", angle);
	}
}

void ASH::switchInput(void *)
{
	tpd3io.switchInput();
}

void ASH::currentStatus(void *)
{
	tpd3io.currentStatus();
}

void ASH::testFunction(void *)
{
	SerialUSB.printf("It works!\n");
}

void ASH::audioPassthrough(void *)
{
	char *arg = uscp.getArg();
	if (strncmp(arg, "true", 16) == 0)
	{
		CONVOLVIR::setPassthrough(true);
	}
	else if (strncmp(arg, "false", 16) == 0)
	{
		CONVOLVIR::setPassthrough(false);
	}
	else
	{
		SerialUSB.printf("Unknown argument: %s\n", arg);
		SerialUSB.printf("Options: [true], [false]\n");
	}
}

void ASH::unknownCommand(void *)
{
	SerialUSB.printf("ash> Unknown command\n");
}

// Stdout progress bar for displaying synthesis completion
void ASH::progressBar(uint8_t progress)
{
	uint8_t percentComplete = uint8_t(100 * progress); // Round to floor

	// Determine needed left and right padding
	uint8_t leftPadding = (uint8_t)(PROGRESS_WIDTH * progress);
	uint8_t rightPadding = PROGRESS_WIDTH - leftPadding;

	// Display ###% [========]
	SerialUSB.printf("\r%3d%% [%.*s%*s]", percentComplete, leftPadding, PROGRESS_BAR, rightPadding, "");

	fflush(stdout); // stdout is buffered so this is needed
}
