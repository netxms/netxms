/**
 * 
 */
package org.netxms.ui.android.main.views;

import java.text.SimpleDateFormat;
import java.util.Date;
import android.content.Context;
import com.jjoe64.graphview.LineGraphView;

/**
 * Extended line graph view - tailored for displaying NetXMS data
 */
public class ExtendedLineGraphView extends LineGraphView
{
	/**
	 * @param context
	 * @param title
	 */
	public ExtendedLineGraphView(Context context, String title)
	{
		super(context, title);
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
			
			double absValue = Math.abs(value);
			if (absValue <= 0.01)
				return String.format("%.5f", value);
			if (absValue <= 1)
				return String.format("%.3f", value);
			if (absValue <= 10)
				return String.format("%.1f", value);
			if (absValue <= 1E3)
				return String.format("%.0f", value);
			if (absValue <= 1E6)
				return String.format("%.1f K", value/1E3);
			if (absValue < 1E9)
				return String.format("%.1f M", value/1E6);
			if (absValue < 1E12)
				return String.format("%.1f G", value/1E9);
			if (absValue < 1E15)
				return String.format("%.1f T", value/1E12);
			return super.formatLabel(value, isValueX);
		}
	}
}
