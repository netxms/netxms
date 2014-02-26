/**
 * 
 */
package org.netxms.webui.mobile.pages;

import java.io.IOException;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCException;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;
import org.netxms.webui.mobile.pages.helpers.AlarmListLabelProvider;
import com.eclipsesource.tabris.ui.PageData;

/**
 * Alarm viewer
 */
public class Alarms extends BasePage
{
   private Display display;
   private NXCSession session = (NXCSession)ConsoleSharedData.getSession();
   private NXCListener clientListener = null;
   private Map<Long, Alarm> alarmList = new HashMap<Long, Alarm>();
   private SortableTableViewer viewer;
   
   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.pages.BasePage#createPageContent(org.eclipse.swt.widgets.Composite, com.eclipsesource.tabris.ui.PageData)
    */
   @Override
   public void createPageContent(Composite parent, PageData pageData)
   {
      final String[] names = { "Source", "Message" };
      final int[] widths = { -1, -1 };
      viewer = new SortableTableViewer(parent, names, widths, 0, SWT.DOWN, SWT.FULL_SELECTION);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setLabelProvider(new AlarmListLabelProvider());
      
      // Add client library listener
      clientListener = new NXCListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            switch(n.getCode())
            {
               case NXCNotification.NEW_ALARM:
               case NXCNotification.ALARM_CHANGED:
                  synchronized(alarmList)
                  {
                     alarmList.put(((Alarm)n.getObject()).getId(), (Alarm)n.getObject());
                  }
                  scheduleAlarmViewerUpdate();
                  break;
               case NXCNotification.ALARM_TERMINATED:
               case NXCNotification.ALARM_DELETED:
                  synchronized(alarmList)
                  {
                     alarmList.remove(((Alarm)n.getObject()).getId());
                  }
                  scheduleAlarmViewerUpdate();
                  break;
               default:
                  break;
            }
         }
      };
      session.addListener(clientListener);

      refresh();
   }
   
   /**
    * Refresh view 
    */
   private void refresh()
   {
      try
      {
         final HashMap<Long, Alarm> list = session.getAlarms();
         synchronized(alarmList)
         {
            alarmList.clear();
            alarmList.putAll(list);
            viewer.setInput(alarmList.values().toArray());
         }
      }
      catch(NXCException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      }
      catch(IOException e)
      {
         // TODO Auto-generated catch block
         e.printStackTrace();
      }
   }

   /**
    * Schedule alarm viewer update
    */
   private void scheduleAlarmViewerUpdate()
   {
      display.asyncExec(new Runnable() {
         @Override
         public void run()
         {
            if (!viewer.getControl().isDisposed())
            {
               synchronized(alarmList)
               {
                  viewer.setInput(alarmList.values().toArray());
               }
            }
         }
      });
   }

   /* (non-Javadoc)
    * @see org.netxms.webui.mobile.pages.BasePage#getTitle()
    */
   @Override
   protected String getTitle()
   {
      return "Alarms";
   }
}
