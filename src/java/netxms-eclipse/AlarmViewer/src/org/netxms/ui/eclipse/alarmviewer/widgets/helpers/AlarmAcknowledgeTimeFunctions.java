package org.netxms.ui.eclipse.alarmviewer.widgets.helpers;

import org.eclipse.jface.dialogs.IDialogSettings;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.Messages;

public class AlarmAcknowledgeTimeFunctions
{

   public static final AlarmAcknowledgeTimeFunctions instance = new AlarmAcknowledgeTimeFunctions();
   
   public String[] getValues()
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
      for (int i = 0; i<menuSize ; i++){
         final int time = settings.getInt("AlarmList.ackMenuEntry"+Integer.toString(i)); //$NON-NLS-1$
         if (time == 0)
         {
            result[i]=Messages.get().AlarmAcknowledgeTimeFunctions_ZiroMinutesEntry;
            continue;
         }
         
         result[i]= parseTimeToString(time);
      }
      return result;
   }
   
   public String parseTimeToString(int time)
   {
      final int days = time / (24 * 60 * 60);
      final int hours = (time - days*(24 * 60 * 60)) / (60 * 60);
      final int minutes = (time - hours*(60 * 60) - days*(24 * 60 * 60)) / 60;
      return (days > 0 ? Integer.toString(days)+Messages.get().AlarmAcknowledgeTimeFunctions_days : "") + (hours > 0 ? Integer.toString(hours)+Messages.get().AlarmAcknowledgeTimeFunctions_hours : "") + (minutes > 0 ? Integer.toString(minutes)+Messages.get().AlarmAcknowledgeTimeFunctions_minutes : "");  //$NON-NLS-1$//$NON-NLS-2$ //$NON-NLS-3$ //$NON-NLS-4$ //$NON-NLS-6$
   }

}
