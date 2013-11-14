package org.netxms.ui.eclipse.charts;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
	private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.charts.messages"; //$NON-NLS-1$
	public String ChartColors_10th;
	public String ChartColors_11th;
	public String ChartColors_12th;
	public String ChartColors_13th;
	public String ChartColors_14th;
	public String ChartColors_15th;
	public String ChartColors_16th;
	public String ChartColors_1st;
	public String ChartColors_2nd;
	public String ChartColors_3rd;
	public String ChartColors_4th;
	public String ChartColors_5th;
	public String ChartColors_6th;
	public String ChartColors_7th;
	public String ChartColors_8th;
	public String ChartColors_9th;
	public String ChartColors_Background;
	public String ChartColors_GridX;
	public String ChartColors_GridY;
	public String ChartColors_LineColors;
	public String ChartColors_PlotArea;
	public String ChartColors_TickX;
	public String ChartColors_TickY;
	public String ChartColors_Title;
	public String DialChartWidget_G;
	public String DialChartWidget_K;
	public String DialChartWidget_M;
	public String GeneralChartPrefs_Dash;
	public String GeneralChartPrefs_DashDot;
	public String GeneralChartPrefs_DashDotDot;
	public String GeneralChartPrefs_Dot;
	public String GeneralChartPrefs_None;
	public String GeneralChartPrefs_ShowTitle;
	public String GeneralChartPrefs_ShowTooltips;
	public String GeneralChartPrefs_Solid;
	public String GeneralChartPrefs_XStyle;
	public String GeneralChartPrefs_YStyle;
	public String GenericChart_Title0;
	public String LineChart_LongTimeFormat;
	public String LineChart_Medium2TimeFormat;
	public String LineChart_MediumTimeFormat;
	public String LineChart_ShortTimeFormat;
	static
	{
		// initialize resource bundle
		NLS.initializeMessages(BUNDLE_NAME, Messages.class);
	}

	private Messages()
	{
	}


	/**
	 * Get message class for current locale
	 *
	 * @return
	 */
	public static Messages get()
	{
		return RWT.NLS.getISO8859_1Encoded(BUNDLE_NAME, Messages.class);
	}

	/**
	 * Get message class for current locale
	 *
	 * @return
	 */
	public static Messages get(Display display)
	{
		CallHelper r = new CallHelper();
		display.syncExec(r);
		return r.messages;
	}

	/**
	 * Helper class to call RWT.NLS.getISO8859_1Encoded from non-UI thread
	 */
	private static class CallHelper implements Runnable
	{
		Messages messages;

		@Override
		public void run()
		{
			messages = RWT.NLS.getISO8859_1Encoded(BUNDLE_NAME, Messages.class);
		}
	}
}

