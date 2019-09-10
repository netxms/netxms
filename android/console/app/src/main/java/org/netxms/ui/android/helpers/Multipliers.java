package org.netxms.ui.android.helpers;

/**
 * Helper for multipliers
 * 
 * @author Marco Incalcaterra (marco.incalcaterra@thinksoft.it)
 *
 */

public class Multipliers
{
	public static final int K = 0;
	public static final int M = 1;
	public static final int G = 2;
	public static final int T = 3;
	public static final int P = 4;
	public static final int E = 5;
	public static final int Z = 5;
	public static final int Y = 5;

	static public String getLabel(int type, int mul)
	{
		String labels[][] = {
				{ "", "", "", "", "", "", "", "" }, // No multiplier
				{ "K", "M", "G", "T", "P", "E", "Z", "Y" }, // Decimal multiplier
				{ "Ki", "Mi", "Gi", "Ti", "Pi", "Ei", "Zi", "Yi" } // Binary multiplier
		};
		return labels[type][mul];
	}

	static public double getValue(int type, int mul)
	{
		double values[][] = {
				{ 1, 1, 1, 1, 1, 1, 1, 1 },
				{ 1E3, 1E6, 1E9, 1E12, 1E15, 1E18, 1E21, 1E24 },
				{
						1024.,
						1024. * 1024,
						1024. * 1024 * 1024,
						1024. * 1024 * 1024 * 1024,
						1024. * 1024 * 1024 * 1024 * 1024,
						1024. * 1024 * 1024 * 1024 * 1024 * 1024,
						1024. * 1024 * 1024 * 1024 * 1024 * 1024 * 1024,
						1024. * 1024 * 1024 * 1024 * 1024 * 1024 * 1024 * 1024 }, };
		if (type >= 0 && type < values.length && mul >= 0 && mul < values[type].length)
			return values[type][mul];
		else
			return 1;
	}
}
