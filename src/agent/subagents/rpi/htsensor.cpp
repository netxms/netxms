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
 * Collector Thread data
 */
static bool volatile m_stopCollectorThread = false;
static THREAD m_collector = INVALID_THREAD_HANDLE;

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
	//usleep(500000);
	ThreadSleepMs(500);
	bcm2835_gpio_write(PIN, LOW);
	ThreadSleepMs(20);
	//usleep(20000);

	bcm2835_gpio_fsel(PIN, BCM2835_GPIO_FSEL_INPT);

	data[0] = data[1] = data[2] = data[3] = data[4] = 0;

	// wait for pin to drop
	counter = 0;
	while (bcm2835_gpio_lev(PIN) == 1 && counter < 1000) 
	{
		struct timespec ts;
		ts.tv_sec = 0;
		ts.tv_nsec = 1000;
		nanosleep(&ts, NULL);
		//usleep(1);
		//ThreadSleepMs(1);
		counter++;
	}

	if (counter == 1000)
	{
		return false;
	}

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

		printf("t=%f, h=%f\n", t, h);
		return true;
	}

	return 0;
}

/**
 * Sensor polling thread
 */
THREAD_RESULT THREAD_CALL SensorPollingThread(void *)
{
	AgentWriteDebugLog(1, _T("RPI: sensor polling thread started"));

	while(!m_stopCollectorThread) 
	{
		if (ReadSensor())
		{
			g_sensorUpdateTime = time(NULL);
		}
		ThreadSleepMs(1500);
	}
	
	return THREAD_OK;
}

BOOL StartSensorCollector()
{
	if (!bcm2835_init()) 
	{
		AgentWriteLog(NXLOG_ERROR, _T("RPI: call to bcm2835_init failed"));
		return FALSE;
	}
	m_collector = ThreadCreateEx(SensorPollingThread, 0, NULL);
	return TRUE;
}

void StopSensorCollector()
{
	m_stopCollectorThread = true;
	if (m_collector != INVALID_THREAD_HANDLE)
	{
		ThreadJoin(m_collector);
		bcm2835_close();
	}
}
