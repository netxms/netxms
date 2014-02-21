/**
 * 
 */
package org.netxms.webui.mobile.pages.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Label provider for alarm list
 */
public class AlarmListLabelProvider extends LabelProvider implements ITableLabelProvider
{
   private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex == 0)
         return StatusDisplayInfo.getStatusImage(((Alarm)element).getCurrentSeverity());
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      Alarm alarm = (Alarm)element;
      switch(columnIndex)
      {
         case 0:
            return session.getObjectName(alarm.getSourceObjectId()) + " - " + StatusDisplayInfo.getStatusText(alarm.getCurrentSeverity());
         case 1:
            return alarm.getMessage();
      }
      return null;
   }
}
