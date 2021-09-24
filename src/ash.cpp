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
	subshell.newCmd("hostname", "", this->hostname, nullptr); // Set hostname / prompt
	subshell.newCmd("unknown", "", this->unknownCommand, nullptr); 
	subshell.newCmd("help", "Show help for the specified command", this->showHelp, nullptr); 

	subshell.newCmd("togglepower", "Toggle power on the D3", this->togglePower, nullptr);
	subshell.newCmd("togglemode", "Toggle D3 input mode", this->switchInput, nullptr);
	subshell.newCmd("pttoggle", "Toggle audio passthrough", this->audioPassthrough, nullptr);
	subshell.newCmd("status", "Get status of the D3", this->currentStatus, nullptr);
	subshell.newCmd("sangle", "Set HRIR angle", this->setAngle, nullptr);
	subshell.newCmd("audiomemory", "View current and maximum audio memory", this->audioMemory, nullptr);
	subshell.newCmd("reboot", "Reboot Auricle", this->reboot, nullptr);
	subshell.newCmd("clear", "Clear screen", this->clear, nullptr);
	subshell.newCmd("memuse", "View amount of RAM free", this->memoryUse, nullptr);
	subshell.newCmd("lscmd", "List all commands", this->listCommands, nullptr);

	if (subshell.status < 0)
	{
		if (subshell.status == SUBSHELL_REALLOC_FAILURE)
		{
			SerialUSB.printf(flashmem("Error: realloc"));
		}
		if (subshell.status == SUBSHELL_SNPRINTF_FAILURE)
		{
			SerialUSB.printf(flashmem("Error: snprintf"));
		}
		subshell.~Subshell();
	}

	this->motd();
}

void ASH::execLoop(void)
{
	subshell.run();
}

void ASH::togglePower(void *)
{
	tpd3io.togglePower();
	SerialUSB.printf("Done\r\n");
}

void ASH::setAngle(void *)
{
	char *cmdArg = nullptr;
	if (subshell.getArg(&cmdArg))
	{
		uint16_t angle = static_cast<uint16_t>(atoi(cmdArg));
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

void ASH::showHelp(void *)
{
	subshell.showHelp();
}

void ASH::memoryUse(void *)
{
	extern unsigned long _heap_end;
	extern char *__brkval;
	SerialUSB.printf("Memory free: %8d\r\n", (reinterpret_cast<char *>(&_heap_end) - __brkval));
}

void ASH::reboot(void *)
{
	usb_serial_flush_output();
	usb_serial_flush_input();
	SerialUSB.printf("Auricle Rebooting\r\n");
	*reinterpret_cast<volatile uint32_t *>(0xE000ED0C) = 0x05FA0004;
}

void ASH::unknownCommand(void *)
{
	ASH::hostname(nullptr);
	SerialUSB.printf("Unknown command\r\n");
	SerialUSB.flush();
}

void ASH::clear(void *)
{
	const char clrSeq[] = {0x1B, 0x5B, 0x32, 0x4A, 0x00};
	SerialUSB.printf("%s", clrSeq);
}

void ASH::hostname(void *)
{
	SerialUSB.printf("ash %% ");
	SerialUSB.flush();
}

void ASH::motd(void)
{
	this->clear(nullptr);
	SerialUSB.printf("Auricle Shell\r\n");
	SerialUSB.printf("Version 0.1\r\n");
	SerialUSB.printf("Copyright (c) 2021 Jason Conway\r\n\r\n");
	this->hostname(nullptr);
}
