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

D3io d3io;

int _write(int FILE, char *writeBuffer, int writeBufferLength)
{
	return usb_serial_write(writeBuffer, writeBufferLength);
}

bool _available(void)
{
	return usb_serial_available() ? true : false;
}

int _getchar(void)
{
	return usb_serial_getchar();
}

int _peekchar(void)
{
	return usb_serial_peekchar();
}

Ash::Ash(void)
{
	//
}

void Ash::init(void)
{
	initSubshell();

	newCmd("shPrompt", "", this->hostname); // Set hostname / prompt
	newCmd("unkCmd", "", this->unknownCommand);
	newCmd("help", "Show help for the specified command", this->help);

	newCmd("togglepower", "Toggle power on the D3", this->togglePower);
	newCmd("togglemode", "Toggle D3 input mode", this->switchInput);
	newCmd("pttoggle", "Toggle audio passthrough", this->audioPassthrough);
	newCmd("status", "Get status of the D3", this->currentStatus);
	newCmd("sangle", "Set HRIR angle", this->setAngle);
	newCmd("audiomemory", "View current and maximum audio memory", this->audioMemory);
	newCmd("reboot", "Reboot Auricle", this->reboot);
	newCmd("clear", "Clear screen", this->clear);
	newCmd("memuse", "View amount of RAM free", this->memoryUse);
	newCmd("lscmd", "List all commands", this->lscmds);

	this->motd();
}

void Ash::execLoop(void)
{
	run();
}

void Ash::togglePower(void *)
{
	d3io.togglePower();
	printf("Done\r\n");
}

void Ash::setAngle(void *)
{
	char *cmdArg = NULL;
	if (getArg(&cmdArg))
	{
		uint16_t angle = (uint16_t)(atoi(cmdArg));
		printf("Setting angle: %d degrees\r\n", angle);
		convolvIR.convertIR(__builtin_round((float32_t)(angle) / 3.6));
		printf("Done\r\n");
	}
	else
	{
		printf("Error: incorrect syntax\r\n");
	}
}

void Ash::switchInput(void *)
{
	d3io.switchInput();
}

void Ash::currentStatus(void *)
{
	d3io.currentStatus();
}

void Ash::lscmds(void *)
{
	listCmds();
}

void Ash::audioMemory(void *)
{
	printf("Active memory usage: %u bytes\r\n", AudioStream::memory_used);
	printf("Maximum memory usage: %u bytes\r\n", AudioStream::memory_used_max);
}

void Ash::audioPassthrough(void *)
{
	printf("Audio Passthough %s\r\n", convolvIR.togglePassthrough() ? "Enabled" : "Disabled");
}

void Ash::help(void *)
{
	showHelp();
}

void Ash::memoryUse(void *)
{
	extern unsigned long _heap_end;
	extern char *__brkval;
	printf("Memory free: %8d\r\n", (char *)(&_heap_end) - __brkval);
}

void Ash::reboot(void *)
{
	printf("Auricle Rebooting\r\n");
	*(volatile uint32_t *)0xE000ED0C = 0x05FA0004;
}

void Ash::unknownCommand(void *)
{
	Ash::hostname(nullptr);
	printf("Unknown command\r\n");
	fflush(stdout);
}

void Ash::clear(void *)
{
	const char clrSeq[] = {0x1B, 0x5B, 0x32, 0x4A, 0x00};
	printf("%s", clrSeq);
}

void Ash::hostname(void *)
{
	printf("ash %% ");
	fflush(stdout);
}

void Ash::motd(void)
{
	this->clear(nullptr);
	printf("Auricle Shell\r\n");
	printf("Version 0.1\r\n");
	printf("Copyright (c) 2021 Jason Conway\r\n\r\n");
	this->hostname(nullptr);
}
