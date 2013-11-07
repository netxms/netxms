/**
 * 
 */
package org.netxms.ui.android.main.views;

import java.text.SimpleDateFormat;
import java.util.Date;

import org.netxms.ui.android.helpers.Multipliers;

import android.content.Context;
import android.content.SharedPreferences;
import android.preference.PreferenceManager;

import com.jjoe64.graphview.LineGraphView;

/**
 * Extended line graph view - tailored for displaying NetXMS data
 */
public class ExtendedLineGraphView extends LineGraphView
{
	private final SharedPreferences sp;

	/**
	 * @param context
	 * @param title
	 */
	public ExtendedLineGraphView(Context context, String title)
	{
		super(context, title);
		sp = PreferenceManager.getDefaultSharedPreferences(context);
	}

	/* (non-Javadoc)
	 * @see com.jjoe64.graphview.GraphView#formatLabel(double, boolean)
	 */
	@Override
	protected String formatLabel(double value, boolean isValueX)
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

			int m = Integer.parseInt(sp.getString("global.multipliers", "1"));
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
			return super.formatLabel(value, isValueX);
		}
	}
}
