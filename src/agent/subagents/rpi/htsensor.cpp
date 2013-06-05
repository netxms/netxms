/*
 ** Raspberry Pi subagent
 ** Copyright (C) 2013 Victor Kirhenshtein
 **
 ** This program is free software; you can redistribute it and/or modify
 ** it under the terms of the GNU General Public License as published by
 ** the Free Software Foundation; either version 2 of the License, or
 ** (at your option) any later version.
 **
 ** This program is distributed in the hope that it will be useful,
 ** but WITHOUT ANY WARRANTY; without even the implied warranty of
 ** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 ** GNU General Public License for more details.
 **
 ** You should have received a copy of the GNU General Public License
 ** along with this program; if not, write to the Free Software
 ** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 **
 **/

#include <nms_common.h>
#include <nms_agent.h>
#include <bcm2835.h>

#define GPIO_BASE 0x20200000
#define PIN 4

#define MAXTIMINGS 100

/**
 * Sensor data 
 */
float g_sensorData[2];
time_t g_sensorUpdateTime;

/**
 * Read data from sensor
 */
static bool ReadSensor()
{
	int data[MAXTIMINGS];
	int counter = 0;
	int laststate = HIGH;
	int bits = 0;

	// Set GPIO pin to output
	bcm2835_gpio_fsel(PIN, BCM2835_GPIO_FSEL_OUTP);

	bcm2835_gpio_write(PIN, HIGH);
	usleep(500000);  // 500 ms
	bcm2835_gpio_write(PIN, LOW);
	usleep(20000);

	bcm2835_gpio_fsel(PIN, BCM2835_GPIO_FSEL_INPT);

	data[0] = data[1] = data[2] = data[3] = data[4] = 0;

	// wait for pin to drop
	counter = 0;
	while (bcm2835_gpio_lev(PIN) == 1 && counter < 1000) 
	{
		usleep(1);
		counter++;
	}

	if (counter == 1000)
		return false;

	// read data
	for(int i = 0; i< MAXTIMINGS; i++) 
	{
		counter = 0;
		while(bcm2835_gpio_lev(PIN) == laststate) 
		{
			counter++;
			if (counter == 1000)
				break;
		}
		if (counter == 1000)
			break;
		laststate = bcm2835_gpio_lev(PIN);

		if ((i > 3) && (i % 2 == 0))
		{
			// shove each bit into the storage bytes
			data[bits / 8] <<= 1;
			if (counter > 200)
			{
				data[bits / 8] |= 1;
			}
			bits++;
		}
	}

	if ((bits >= 39) && (data[4] == ((data[0] + data[1] + data[2] + data[3]) & 0xFF))) 
	{
		float h = (data[0] * 256 + data[1]) / 10.0;
		float t = ((data[2] & 0x7F) * 256 + data[3]) / 10.0;
		if (data[2] & 0x80)
			t *= -1;
			
		g_sensorData[0] = h;
		g_sensorData[1] = t;
		return true;
	}

	return 0;
}

/**
 * Sensor polling thread
 */
THREAD_RESULT THREAD_CALL SensorPollingThread(void *)
{
	if (!bcm2835_init()) 
	{
		AgentWriteLog(NXLOG_ERROR, _T("RPI: call to bcm2835_init failed"));
		return THREAD_OK;
	}

	AgentWriteDebugLog(1, _T("RPI: sensor polling thread started"));

	while(1) 
	{
		if (ReadSensor())
			g_sensorUpdateTime = time(NULL);
		ThreadSleepMs(500);
	}
	
	return THREAD_OK;
}
