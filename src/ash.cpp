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
	uscp.newCmd("pttoggle", this->audioPassthrough, NULL);
	uscp.newCmd("status", this->currentStatus, NULL);
	uscp.newCmd("sangle", this->setAngle, NULL);
	uscp.newCmd("audiomemory", this->audioMemory, NULL);
	uscp.newCmd("reboot", this->reboot, NULL);
	uscp.newCmd("clear", this->clear, NULL);
	uscp.newCmd("memuse", this->memoryUse, NULL);
	uscp.newCmd("lscmd", this->listCommands, NULL);

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
		SerialUSB.printf("Error: incorrect syntax\r\n");
	}
	else
	{
		uint16_t angle = static_cast<uint16_t>(atoi(arg));
		SerialUSB.printf("Setting angle: %d degrees\r\n", angle);
		convolvIR.convertIR(angle / 3.6);						
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
	SerialUSB.printf("Auricle Shell\r\n");
	SerialUSB.printf("Version 0.1\r\n");
	SerialUSB.printf("Copyright (c) 2021 Jason Conway\r\n\r\n");
	hostname(NULL);
}
