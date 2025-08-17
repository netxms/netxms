/**
 * 
 */
package com.netxms.mcp;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.BulkAlarmStateChangeData;

/**
 * Alarm manager
 */
public class AlarmManager implements SessionListener
{
   private static AlarmManager instance;

   /**
    * Get the singleton instance of AlarmManager
    *
    * @return the singleton instance of AlarmManager
    */
   public static AlarmManager getInstance()
   {
      return instance;
   }

   protected static void initialize(NXCSession session) throws Exception
   {
      instance = new AlarmManager();
      instance.alarms = session.getAlarms();
      session.addListener(instance);
   }

   private Map<Long, Alarm> alarms = null;

   /**
    * Private constructor to prevent instantiation
    */
   private AlarmManager()
   {
   }

   /**
    * @see org.netxms.client.SessionListener#notificationHandler(org.netxms.client.SessionNotification)
    */
   @Override
   public void notificationHandler(SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.ALARM_CHANGED:
            Alarm alarm = (Alarm)n.getObject();
            synchronized(alarms)
            {
               alarms.put(alarm.getId(), alarm);
            }
            break;
         case SessionNotification.ALARM_TERMINATED:
         case SessionNotification.ALARM_DELETED:
            synchronized(alarms)
            {
               alarms.remove(((Alarm)n.getObject()).getId());
            }
            break;
         case SessionNotification.MULTIPLE_ALARMS_RESOLVED:
            synchronized(alarms)
            {
               BulkAlarmStateChangeData d = (BulkAlarmStateChangeData)n.getObject();
               for(Long id : d.getAlarms())
               {
                  Alarm a = alarms.get(id);
                  if (a != null)
                  {
                     a.setResolved(d.getUserId(), d.getChangeTime());
                  }
               }
            }
            break;
         case SessionNotification.MULTIPLE_ALARMS_TERMINATED:
            synchronized(alarms)
            {
               for(Long id : ((BulkAlarmStateChangeData)n.getObject()).getAlarms())
               {
                  alarms.remove(id);
               }
            }
            break;
         default:
            break;
      }
   }

   /**
    * Get all alarms.
    *
    * @return all active alarms
    */
   public List<Alarm> getAllAlarms()
   {
      synchronized(alarms)
      {
         return new ArrayList<>(alarms.values());
      }
   }

   /**
    * @param rootObjectId
    * @param minSeverity
    * @param state
    * @param limit
    * @return
    */
   public List<Alarm> getAlarms(long rootObjectId, Severity minSeverity, int state, int limit)
   {
      List<Alarm> result = new ArrayList<>();
      synchronized(alarms)
      {
         for(Alarm alarm : alarms.values())
         {
            if ((rootObjectId == 0 || alarm.getSourceObjectId() == rootObjectId) &&
                (minSeverity == null || alarm.getCurrentSeverity().getValue() >= minSeverity.getValue()) &&
                (state == -1 || alarm.getState() == state))
            {
               result.add(alarm);
               if (limit > 0 && result.size() >= limit)
                  break;
            }
         }
      }
      return result;
   }
}
