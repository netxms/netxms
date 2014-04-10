package com.radensolutions.reporting.application;

import java.util.Calendar;
import java.util.Date;
import java.util.HashMap;
import java.util.Map;
import java.util.Map.Entry;

public class PrepareParameterDate
{	
	static public final int TYPE_YEAR = 0;
	static public final int TYPE_MONTH = 1;
	static public final int TYPE_DAY = 2;
	static public final int TYPE_HOUR = 3;
	static public final int TYPE_MINUTE = 4;

	private Calendar calendar;
	
	public static Map<String, Integer> stringValues = new HashMap<String, Integer>(0);
	static
	{
		stringValues.put("current", 0); //$NON-NLS-1$
		stringValues.put("previous", -1); //$NON-NLS-1$
		stringValues.put("next", 1); //$NON-NLS-1$
	}
	
	/**
	 * Base method
	 * 
	 * @param textValue
	 * @return
	 */
	public Date getDateTime(String textValue)
	{
		if (!textValue.isEmpty() && textValue.contains(";"))
		{
			String[] textValues = textValue.split(";");
			if (textValues.length == 5) // format - year;month;day;hour;minute
			{
				calendar = Calendar.getInstance(); 
				for (int i = 0; i < textValues.length; i++)
					parseDateString(textValues[i], i);
				
				return calendar.getTime();
			}
		}
		return null;
	}
	
	/**
	 * @param strValue
	 * @param type
	 */
	private void parseDateString(String strValue, int type)
	{
		boolean stringValue = false;
		int value = 0;
		int offset = 0;
	
		try
		{
			value = Integer.valueOf(strValue.trim());
		} 
		catch (Exception ex)
		{
			stringValue = true;
		}
		
		if (stringValue)
		{
			try
			{
				for (Entry<String, Integer> str : stringValues.entrySet())
				{
					if (strValue.indexOf(str.getKey()) != -1)
					{
						int strVal = str.getValue();
						strValue = strValue.replaceAll(str.getKey(), ""); //$NON-NLS-1$
						if (strValue.trim().isEmpty())
						{
							offset = strVal;
							break;
						}
						else
						{
							int secondVal = Integer.valueOf(Integer.valueOf(strValue.trim().substring(1).trim()));
							switch (strValue.trim().charAt(0))
							{
								case '+':
									offset = strVal + secondVal;
									break;
								case '-':
									offset = strVal - secondVal;
									break;
								case '*':
									offset = strVal * secondVal;
									break;
								case '/':
									offset = strVal / secondVal;
									break;
								default:
									break;
							}
						}
					}
				}
			}
			catch (Exception e)
			{
				e.printStackTrace();
			}
		}
		
		int calendarIndication = -1;
		switch (type)
		{
			case TYPE_YEAR:
				calendarIndication = Calendar.YEAR;
				break;
			case TYPE_MONTH:
				calendarIndication = Calendar.MONTH;
				break;
			case TYPE_DAY:
				calendarIndication = Calendar.DAY_OF_MONTH;
				break;
			case TYPE_HOUR:
				calendarIndication = Calendar.HOUR_OF_DAY;
				break;
			case TYPE_MINUTE:
				calendarIndication = Calendar.MINUTE;
				break;
			default:
				break;
		}
		
		if (stringValue)
			calendar.add(calendarIndication, offset);
		else
			calendar.set(calendarIndication, (type == TYPE_MONTH ? value - 1 : value));
	}
}
