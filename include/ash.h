/**
 * @file ash.h
 * @author Jason Conway (jpc@jasonconway.dev)
 * @brief Auircle Shell
 * @version 0.1
 * @date 2021-08-08
 * 
 * @copyright Copyright (c) 2021 Jason Conway. All rights reserved.
 * 
 */

#pragma once

#include "auricle.h"
#include "subshell.h"
#include "d3io.h"
#include "convolvIR.h"

class Ash
{
public:
	Ash(usb_serial_class &ioStream = SerialUSB);
	void execLoop(void);
	void init(void);

private:
	static usb_serial_class *stream;
	
	void motd(void);

	static void togglePower(void *);
	static void setAngle(void *);
	static void switchInput(void *);
	static void currentStatus(void *);
	static void audioPassthrough(void *);
	static void audioMemory(void *);

	static void reboot(void *);
	static void clear(void *);
	static void memoryUse(void *);
	static void listCommands(void *);

	static void unknownCommand(void *);
	static void hostname(void *);
	static void showHelp(void *);
};
