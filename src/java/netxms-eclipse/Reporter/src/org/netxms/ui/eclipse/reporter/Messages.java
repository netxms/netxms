package org.netxms.ui.eclipse.reporter;

import org.eclipse.osgi.util.NLS;

public class Messages extends NLS
{
   private static final String BUNDLE_NAME = "org.netxms.ui.eclipse.reporter.messages"; //$NON-NLS-1$
   public static String AlarmStateFieldEditor_Acknowledged;
   public static String AlarmStateFieldEditor_Outstanding;
   public static String AlarmStateFieldEditor_Resolved;
   public static String AlarmStateFieldEditor_Terminated;
   public static String BooleanFieldEditor_No;
   public static String BooleanFieldEditor_Yes;
   public static String DateFieldEditor_Calendar;
   public static String DateFieldEditor_Day;
   public static String DateFieldEditor_Month;
   public static String DateFieldEditor_Year;
   public static String EventFieldEditor_Any;
   public static String EventFieldEditor_ClearSelection;
   public static String EventFieldEditor_SelectEvent;
   public static String ObjectFieldEditor_Any;
   public static String ObjectFieldEditor_ClearSelection;
   public static String ObjectFieldEditor_SelectObject;
   public static String ObjectListFieldEditor_Add;
   public static String ObjectListFieldEditor_Delete;
   public static String ReportResultLabelProvider_Success;
   public static String ScheduleLabelProvider_Daily;
   public static String ScheduleLabelProvider_Error;
   public static String ScheduleLabelProvider_Monthly;
   public static String ScheduleLabelProvider_Once;
   public static String ScheduleLabelProvider_Weekly;
   public static String UserFieldEditor_None;
   static
   {
      // initialize resource bundle
      NLS.initializeMessages(BUNDLE_NAME, Messages.class);
   }

   private Messages()
	{
 }


	private static Messages instance = new Messages();

	public static Messages get()
	{
		return instance;
	}

}
