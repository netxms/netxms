package org.netxms.ui.eclipse.reporter;

import org.eclipse.osgi.util.NLS;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.swt.widgets.Display;


public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.reporter.messages"; //$NON-NLS-1$
   public String AlarmStateFieldEditor_Acknowledged;
   public String AlarmStateFieldEditor_Outstanding;
   public String AlarmStateFieldEditor_Resolved;
   public String AlarmStateFieldEditor_Terminated;
   public String BooleanFieldEditor_No;
   public String BooleanFieldEditor_Yes;
   public String DateFieldEditor_Calendar;
   public String DateFieldEditor_Day;
   public String DateFieldEditor_Month;
   public String DateFieldEditor_Year;
   public String EventFieldEditor_Any;
   public String EventFieldEditor_ClearSelection;
   public String EventFieldEditor_SelectEvent;
   public String ObjectFieldEditor_Any;
   public String ObjectFieldEditor_ClearSelection;
   public String ObjectFieldEditor_SelectObject;
   public String ObjectListFieldEditor_Add;
   public String ObjectListFieldEditor_Delete;
   public String ReportResultLabelProvider_Success;
   public String ScheduleLabelProvider_Daily;
   public String ScheduleLabelProvider_Error;
   public String ScheduleLabelProvider_Monthly;
   public String ScheduleLabelProvider_Once;
   public String ScheduleLabelProvider_Weekly;
   public String UserFieldEditor_None;
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
