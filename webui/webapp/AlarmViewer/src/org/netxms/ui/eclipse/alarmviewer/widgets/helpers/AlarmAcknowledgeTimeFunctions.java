package org.netxms.ui.eclipse.alarmviewer.widgets.helpers;

import org.eclipse.jface.dialogs.IDialogSettings;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.Messages;

public class AlarmAcknowledgeTimeFunctions
{
   public static String[] getValues()
   {
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      int menuSize;
      String[] result;
      try
      {
         menuSize = settings.getInt("AlarmList.ackMenuSize");//$NON-NLS-1$
      }
      catch(NumberFormatException e)
      {
         return null;
      }
      result = new String[menuSize];
      for(int i = 0; i < menuSize; i++)
      {
         final int time = settings.getInt("AlarmList.ackMenuEntry" + Integer.toString(i)); //$NON-NLS-1$
         if (time == 0)
         {
            result[i] = Messages.get().AlarmAcknowledgeTimeFunctions_ZeroMinutesEntry;
            continue;
         }

         result[i] = timeToString(time);
      }
      return result;
   }
   
   /**
    * @param n
    * @param singular
    * @param plural
    * @return
    */
   private static void formatNumber(int n, String singular, String plural, StringBuilder sb)
   {
      if (n == 0)
         return;
      
      if (sb.length() > 0)
         sb.append(' ');
      sb.append(n);
      sb.append(' ');
      if (n == 1)
         sb.append(singular);
      else
         sb.append(plural);
   }

   /**
    * @param time
    * @return
    */
   public static String timeToString(int time)
   {
      final int days = time / (24 * 60 * 60);
      final int hours = (time - days * (24 * 60 * 60)) / (60 * 60);
      final int minutes = (time - hours * (60 * 60) - days * (24 * 60 * 60)) / 60;
      StringBuilder sb = new StringBuilder();
      formatNumber(days, Messages.get().AlarmAcknowledgeTimeFunctions_day, Messages.get().AlarmAcknowledgeTimeFunctions_days, sb);
      formatNumber(hours, Messages.get().AlarmAcknowledgeTimeFunctions_hour, Messages.get().AlarmAcknowledgeTimeFunctions_hours, sb);
      formatNumber(minutes, Messages.get().AlarmAcknowledgeTimeFunctions_minute, Messages.get().AlarmAcknowledgeTimeFunctions_minutes, sb);
      return sb.toString();
   }
}
