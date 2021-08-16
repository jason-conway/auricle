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
#include "IR.h"

USCP uscp;
TPD3IO tpd3io;

ASH::ASH(void)
{

}

void ASH::init(void)
{
	uscp.newCmd("hostname", this->hostname, NULL); // Set shell name
	uscp.newCmd("unknown", this->unknownCommand, NULL);

	uscp.newCmd("toggle-power", this->togglePower, NULL);
	uscp.newCmd("switch-input", this->switchInput, NULL);
	uscp.newCmd("passthrough", this->audioPassthrough, NULL);
	uscp.newCmd("get-status", this->currentStatus, NULL);
	uscp.newCmd("set-angle", this->setAngle, NULL);
	uscp.newCmd("audiomemory", this->audioMemory, NULL);
	uscp.newCmd("reboot", this->reboot, NULL);
	uscp.newCmd("clear", this->clear, NULL);
	uscp.newCmd("memory-use", this->memoryUse, NULL);
	uscp.newCmd("list", this->listCommands, NULL);

	this->motd();
}

void ASH::execLoop(void)
{
	uscp.checkStream();
}

void ASH::togglePower(void *)
{
	tpd3io.togglePower();
	SerialUSB.printf("Done\r\n");
}

void ASH::setAngle(void *)
{
	char *arg = uscp.getArg();
	if (arg == NULL)
	{
		SerialUSB.printf("Incorrect syntax\r\n");
	}
	else
	{
		uint16_t angle = (uint16_t)atoi(arg);
		SerialUSB.printf("Setting angle: %d degrees\r\n", angle);
		angle = (uint16_t)floor(angle / 45);

		HRIR hrir;
		hrir.leftIR = IR_LUT[angle][0];
		hrir.rightIR = IR_LUT[angle][1];
		convolvIR.convertIR(&hrir);
		
		SerialUSB.printf("Done\r\n");
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

void ASH::listCommands(void *)
{
	uscp.listCmds();
}

void ASH::audioMemory(void *)
{
	char *arg = uscp.getArg();
	if (arg == NULL)
	{
		SerialUSB.printf("Incorrect syntax\r\n");
	}
	else if (strncmp(arg, "current", 16) == 0)
	{
		float32_t usage = AudioStream::memory_used;
		Serial.printf("Memory usage: %.2f%%\n", usage);
	}
	else if (strncmp(arg, "max", 16) == 0)
	{
		float32_t usage = AudioStream::memory_used_max;
		Serial.printf("Maximum memory usage: %.2f%%\n", usage);
	}
	else
	{
		SerialUSB.printf("Unknown argument: %s\r\n", arg);
		SerialUSB.printf("Options: [current], [max]\r\n");
	}
}

void ASH::audioPassthrough(void *)
{
	char *arg = uscp.getArg();
	if (arg == NULL)
	{
		SerialUSB.printf("Incorrect syntax\r\n");
	}
	else if (strncmp(arg, "true", 16) == 0)
	{
		convolvIR.setPassthrough(true);
		SerialUSB.printf("Audio Passthough Enabled\r\n");
	}
	else if (strncmp(arg, "false", 16) == 0)
	{
		convolvIR.setPassthrough(false);
		SerialUSB.printf("Audio Passthough Disabled\r\n");
	}
	else
	{
		SerialUSB.printf("Unknown argument: %s\r\n", arg);
		SerialUSB.printf("Options: [true], [false]\r\n");
	}
}

void ASH::memoryUse(void *)
{
	// Linker hack
	extern char _ebss[];
	extern char _heap_end[];
	extern char *__brkval;

	auto sp = (char *)__builtin_frame_address(0);
	auto stack = sp - _ebss;
	auto heap = _heap_end - __brkval;

	SerialUSB.printf("Stack: %8d b %5d kb\r\n", stack, stack >> 10);
	SerialUSB.printf("Heap:  %8d b %5d kb\r\n", heap, heap >> 10);
}

void ASH::reboot(void *)
{
	usb_serial_flush_output();
	usb_serial_flush_input();
	SerialUSB.printf("Auricle Rebooting\r\n");
	(*(volatile uint32_t *)0xE000ED0C) = 0x05FA0004;
}

void ASH::unknownCommand(void *)
{
	SerialUSB.printf("ash> Unknown command\r\n");
	fflush(stdout);
}

void ASH::clear(void *)
{
	SerialUSB.printf("\e[1;1H\e[2J");
}

void ASH::hostname(void *)
{
	SerialUSB.printf("ash> ");
	usb_serial_flush_input();
}

void ASH::motd(void)
{
	SerialUSB.printf("Auricle Shell\r\n");
	SerialUSB.printf("Version 0.1\r\n");
	SerialUSB.printf("Copyright (c) 2021 Jason Conway\r\n\r\n");
	hostname(NULL);
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


