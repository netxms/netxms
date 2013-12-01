package org.netxms.ui.eclipse.reporter.widgets.helpers;

import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

/**
 * Converter for s
 */
public class ScheduleDateConverter
{
	private Calendar currentCalendar;
	private int year = 0;
	private int month = 0;
	private int day = 0;
	private int hours = 0;
	private int minutes = 0;
	private long offset = 0;

	public static Map<String, Integer> stringValues = new HashMap<String, Integer>(0);
	
	static
	{
		stringValues.put("current", 0);
		stringValues.put("previous", -1);
		stringValues.put("next", 1);
	}

	public ScheduleDateConverter(String[] datesElementsName, String[] dateElements)
	{
		currentCalendar = Calendar.getInstance();
		currentCalendar.setTime(new Date());
		for (int i = 0; i < datesElementsName.length && i < dateElements.length; i++)
			parseDateString(dateElements[i], datesElementsName[i]);
	}

	/**
	 * @param input
	 * @param type
	 */
	private void parseDateString(String input, String type)
	{
		int val = 0;
		int period = 0;
		if (!input.trim().isEmpty())
		{
			try
			{
				val = Integer.valueOf(input.trim());
			} catch (Exception ex)
			{
				val = 0;
			}

			if (val == 0)
			{
				try
				{
					for (Entry<String, Integer> value : stringValues.entrySet())
					{
						if (input.indexOf(value.getKey()) != -1)
						{
							int count = value.getValue();
							input = input.replaceAll(value.getKey(), "");
							if (input.trim().isEmpty())
							{
								period = count;
								break;
							}
							else
							{
								switch (input.trim().charAt(0))
								{
									case '+':
										period = count + Integer.valueOf(Integer.valueOf(input.trim().substring(1).trim()));
										break;
									case '-':
										period = count - Integer.valueOf(Integer.valueOf(input.trim().substring(1).trim()));
										break;
									default:
										break;
								}
							}
						}
					}
				} catch (Exception e)
				{
					e.printStackTrace();
					return;
				}
			}
		}

		if (type.equalsIgnoreCase("year"))
		{
			if (val == 0)
			{
				val = currentCalendar.get(Calendar.YEAR);
				offset += (period * 60000 * 60 * 24 * 365L);
			}
			year = val;
		}
		if (type.equalsIgnoreCase("month"))
		{
			if (val == 0)
			{
				val = currentCalendar.get(Calendar.MONTH);
				offset += (period * 60000L * 60L * 24L * 31L);
			}
			month = val - 1;
		}
		if (type.equalsIgnoreCase("day"))
		{
			if (val == 0)
			{
				val = currentCalendar.get(Calendar.DAY_OF_MONTH);
				offset += (period * 60000L * 60L * 24L);
			}
			day = val;
		}
		if (type.equalsIgnoreCase("hours"))
		{
			if (val == 0)
			{
				val = currentCalendar.get(Calendar.HOUR_OF_DAY);
				offset += (period * 60000L * 60L);
			}
			hours = val;
		}
		if (type.equalsIgnoreCase("minutes"))
		{
			if (val == 0)
			{
				val = currentCalendar.get(Calendar.MINUTE);
				offset += (period * 60000L);
			}
			minutes = val;
		}

	}

	/**
	 * Get selected date
	 * 
	 * @return
	 */
	public Date getDateTime()
	{
		Calendar calendar = Calendar.getInstance();
		calendar.set(year, month, day, hours, minutes, 0);
		return calendar.getTime();
	}

	/**
	 * Get selected offset
	 * 
	 * @return
	 */
	public long getOffset()
	{
		return offset;
	}
}
