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

Subshell subshell(SerialUSB);
D3io d3io;
usb_serial_class *Ash::stream = nullptr;

Ash::Ash(usb_serial_class &ioStream)
{
	stream = &ioStream;
}

void Ash::init(void)
{
	subshell.newCmd("shPrompt", "", this->hostname); // Set hostname / prompt
	subshell.newCmd("unkCmd", "", this->unknownCommand);
	subshell.newCmd("help", "Show help for the specified command", this->showHelp);

	subshell.newCmd("togglepower", "Toggle power on the D3", this->togglePower);
	subshell.newCmd("togglemode", "Toggle D3 input mode", this->switchInput);
	subshell.newCmd("pttoggle", "Toggle audio passthrough", this->audioPassthrough);
	subshell.newCmd("status", "Get status of the D3", this->currentStatus);
	subshell.newCmd("sangle", "Set HRIR angle", this->setAngle);
	subshell.newCmd("audiomemory", "View current and maximum audio memory", this->audioMemory);
	subshell.newCmd("reboot", "Reboot Auricle", this->reboot);
	subshell.newCmd("clear", "Clear screen", this->clear);
	subshell.newCmd("memuse", "View amount of RAM free", this->memoryUse);
	subshell.newCmd("lscmd", "List all commands", this->listCommands);

	this->motd();
}

void Ash::execLoop(void)
{
	subshell.run();
}

void Ash::togglePower(void *)
{
	d3io.togglePower();
	stream->printf("Done\r\n");
}

void Ash::setAngle(void *)
{
	char *cmdArg = nullptr;
	if (subshell.getArg(&cmdArg))
	{
		uint16_t angle = (uint16_t)(atoi(cmdArg));
		stream->printf("Setting angle: %d degrees\r\n", angle);
		convolvIR.convertIR(__builtin_round((float32_t)(angle) / 3.6));
		stream->printf("Done\r\n");
	}
	else
	{
		stream->printf("Error: incorrect syntax\r\n");
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

void Ash::listCommands(void *)
{
	subshell.listCmds();
}

void Ash::audioMemory(void *)
{
	stream->printf("Active memory usage: %u bytes\r\n", AudioStream::memory_used);
	stream->printf("Maximum memory usage: %u bytes\r\n", AudioStream::memory_used_max);
}

void Ash::audioPassthrough(void *)
{
	stream->printf("Audio Passthough %s\r\n", convolvIR.togglePassthrough() ? "Enabled" : "Disabled");
}

void Ash::showHelp(void *)
{
	subshell.showHelp();
}

void Ash::memoryUse(void *)
{
	extern unsigned long _heap_end;
	extern char *__brkval;
	stream->printf("Memory free: %8d\r\n", (char *)(&_heap_end) - __brkval);
}

void Ash::reboot(void *)
{
	stream->flush();
	stream->clear();
	stream->printf("Auricle Rebooting\r\n");
	*(volatile uint32_t *)0xE000ED0C = 0x05FA0004;
}

void Ash::unknownCommand(void *)
{
	Ash::hostname(nullptr);
	stream->printf("Unknown command\r\n");
	stream->flush();
}

void Ash::clear(void *)
{
	const char clrSeq[] = {0x1B, 0x5B, 0x32, 0x4A, 0x00};
	stream->printf("%s", clrSeq);
}

void Ash::hostname(void *)
{
	stream->printf("ash %% ");
	stream->flush();
}

void Ash::motd(void)
{
	this->clear(nullptr);
	stream->printf("Auricle Shell\r\n");
	stream->printf("Version 0.1\r\n");
	stream->printf("Copyright (c) 2021 Jason Conway\r\n\r\n");
	this->hostname(nullptr);
}
