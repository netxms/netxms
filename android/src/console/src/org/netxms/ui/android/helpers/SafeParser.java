package org.netxms.ui.android.helpers;

public class SafeParser
{
	/**
	 * Parse int without throwing exception
	 * @param text text to parse
	 * @param defVal default value to be used in case of parse error
	 * @return parsed value
	 */
	static public int parseInt(String text, int defVal)
	{
		try
		{
			return Integer.parseInt(text);
		}
		catch(NumberFormatException e)
		{
		}
		return defVal;
	}
	
	/**
	 * Parse long without throwing exception
	 * @param text text to parse
	 * @param defVal default value to be used in case of parse error
	 * @return parsed value
	 */
	static public long parseLong(String text, long defVal)
	{
		try
		{
			return Long.parseLong(text);
		}
		catch(NumberFormatException e)
		{
		}
		return defVal;
	}
	
}
