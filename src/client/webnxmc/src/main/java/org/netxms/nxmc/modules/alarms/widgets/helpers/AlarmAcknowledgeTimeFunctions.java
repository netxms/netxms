package org.netxms.nxmc.modules.alarms.widgets.helpers;

import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

public class AlarmAcknowledgeTimeFunctions
{
   private static I18n i18n = LocalizationHelper.getI18n(AlarmAcknowledgeTimeFunctions.class);

   public static String[] getValues()
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      int menuSize = settings.getAsInteger("AlarmList.AckMenuSize", 0);//$NON-NLS-1$
      if (menuSize < 1)
         return null;
      String[] result = new String[menuSize];
      for(int i = 0; i < menuSize; i++)
      {
         int time = settings.getAsInteger("AlarmList.AckMenuEntry." + Integer.toString(i), 0); //$NON-NLS-1$
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
      formatNumber(days, i18n.tr("day"), i18n.tr("days"), sb);
      formatNumber(hours, i18n.tr("hour"), i18n.tr("hours"), sb);
      formatNumber(minutes, i18n.tr("minute"), i18n.tr("minutes"), sb);
      return (sb.length() > 0) ? sb.toString() : i18n.tr("0 minutes");
   }
}
