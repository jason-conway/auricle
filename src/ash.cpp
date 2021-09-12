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
TPD3IO tpd3io;

ASH::ASH(void)
{

}

void ASH::init(void)
{
	subshell.newCmd("hostname", "", this->hostname, NULL); // Set shell name
	subshell.newCmd("unknown", "", this->unknownCommand, NULL);

	subshell.newCmd("togglepower", "Toggle power on the D3", this->togglePower, NULL);
	subshell.newCmd("togglemode", "Toggle D3 input mode", this->switchInput, NULL);
	subshell.newCmd("pttoggle", "Toggle audio passthrough", this->audioPassthrough, NULL);
	subshell.newCmd("status", "Get status of the D3", this->currentStatus, NULL);
	subshell.newCmd("sangle", "Set HRIR angle", this->setAngle, NULL);
	subshell.newCmd("audiomemory", "View current and maximum audio memory", this->audioMemory, NULL);
	subshell.newCmd("reboot", "Reboot Auricle", this->reboot, NULL);
	subshell.newCmd("clear", "Clear screen", this->clear, NULL);
	subshell.newCmd("memuse", "View amount of RAM free", this->memoryUse, NULL);
	subshell.newCmd("lscmd", "List all commands", this->listCommands, NULL);

	this->motd();
}

void ASH::execLoop(void)
{
	subshell.checkStream();
}

bool ASH::argHelp(void)
{
	if (char *arg = subshell.getArg())
	{
		if (strncmp(arg, "-h", 64) == 0)
		{
			subshell.showCmdHelp();
		}
		else
		{
			SerialUSB.printf("Error: incorrect syntax\r\n");
		}
		return true;
	}
	return false;
}

void ASH::togglePower(void *)
{
	if (argHelp())
	{
		return;
	}

	tpd3io.togglePower();
	SerialUSB.printf("Done\r\n");
}

void ASH::setAngle(void *)
{
	if (char *arg = subshell.getArg())
	{
		uint16_t angle = static_cast<uint16_t>(atoi(arg));
		SerialUSB.printf("Setting angle: %d degrees\r\n", angle);
		convolvIR.convertIR(__builtin_round(static_cast<float32_t>(angle) / 3.6));
		SerialUSB.printf("Done\r\n");
	}
	else
	{
		SerialUSB.printf("Error: incorrect syntax\r\n");
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
	subshell.listCmds();
}

void ASH::audioMemory(void *)
{
	SerialUSB.printf("Active memory usage: %.2u%%\r\n", AudioStream::memory_used);
	SerialUSB.printf("Maximum memory usage: %.2u%%\r\n", AudioStream::memory_used_max);
}

void ASH::audioPassthrough(void *)
{
	SerialUSB.printf("Audio Passthough %s\r\n", convolvIR.togglePassthrough() ? "Enabled" : "Disabled");
}

void ASH::memoryUse(void *)
{
	extern unsigned long _heap_end;
	extern char *__brkval;

	SerialUSB.printf("Memory free: %8d\r\n", ((char *)&_heap_end - __brkval));
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
	SerialUSB.printf("\033[1;1H\033[2J");
}

void ASH::hostname(void *)
{
	SerialUSB.printf("ash> ");
	fflush(stdout);
}

void ASH::motd(void)
{
	this->clear(NULL);
	SerialUSB.printf("Auricle Shell\r\n");
	SerialUSB.printf("Version 0.1\r\n");
	SerialUSB.printf("Copyright (c) 2021 Jason Conway\r\n\r\n");
	this->hostname(NULL);
}
