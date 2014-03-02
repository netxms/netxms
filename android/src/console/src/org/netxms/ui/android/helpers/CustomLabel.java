package org.netxms.ui.android.helpers;

import java.text.SimpleDateFormat;
import java.util.Date;

import com.jjoe64.graphview.CustomLabelFormatter;

public class CustomLabel implements CustomLabelFormatter
{
	private int m = 1;

	public CustomLabel(int m)
	{
		this.m = m;
	}

	@Override
	public String formatLabel(double value, boolean isValueX)
	{
		if (isValueX)
		{
			SimpleDateFormat s = new SimpleDateFormat("HH:mm:ss");
			return s.format(new Date((long)value));
		}
		else
		{
			if (value == 0)
				return "0";

			double absValue = Math.abs(value);
			if (absValue <= 0.01)
				return String.format("%.5f", value);
			if (absValue <= 0.1)
				return String.format("%.4f", value);
			if (absValue <= 1)
				return String.format("%.3f", value);
			if (absValue <= 10)
				return String.format("%.2f", value);
			if (absValue <= 100)
				return String.format("%.1f", value);
			if (absValue <= Multipliers.getValue(m, Multipliers.K))
				return String.format("%.0f", value);
			if (absValue <= Multipliers.getValue(m, Multipliers.M))
				return String.format("%.1f %s", value / Multipliers.getValue(m, Multipliers.K), Multipliers.getLabel(m, Multipliers.K));
			if (absValue < Multipliers.getValue(m, Multipliers.G))
				return String.format("%.1f %s", value / Multipliers.getValue(m, Multipliers.M), Multipliers.getLabel(m, Multipliers.M));
			if (absValue < Multipliers.getValue(m, Multipliers.T))
				return String.format("%.1f %s", value / Multipliers.getValue(m, Multipliers.G), Multipliers.getLabel(m, Multipliers.G));
			if (absValue < Multipliers.getValue(m, Multipliers.P))
				return String.format("%.1f %s", value / Multipliers.getValue(m, Multipliers.T), Multipliers.getLabel(m, Multipliers.T));
			return String.format("%.1f %s", value / Multipliers.getValue(m, Multipliers.P), Multipliers.getLabel(m, Multipliers.P));
		}
	}
}
