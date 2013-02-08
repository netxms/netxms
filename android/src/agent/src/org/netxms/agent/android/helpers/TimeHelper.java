package org.netxms.agent.android.helpers;

import java.util.Calendar;

public class TimeHelper
{
	/**
	 * Get a locale string from specified amount of milliseconds
	 * 
	 * @param millis amount of milliseconds to convert
	 * @return time string in local time
	 */
	@SuppressWarnings("deprecation")
	static public String getTimeString(long millis)
	{
		Calendar cal = Calendar.getInstance();
		if (millis != 0)
			cal.setTimeInMillis(millis);
		return cal.getTime().toLocaleString();
	}
}
