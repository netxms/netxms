/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
 * <p/>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p/>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p/>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import java.util.ArrayList;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;
import org.netxms.utilities.TestHelperForEpp;

/**
 * Testing alarm functionality in EPPR action. test generates an alarm, changes few times state and severity, generate timeout alarm
 * terminates the alarm and checks that these functions are working correctly
 * 
 * @throws Exception
 */
public class EppAlarmTest extends AbstractSessionTest
{
   /**
    * Find alarm 
    * 
    * @param session
    * @param key
    * @return alarm based on the event rule key that is expected to generate the alarm
    * @throws Exception
    */
   private Alarm findAlarmByKey(final NXCSession session, String key) throws Exception
   {
      HashMap<Long, Alarm> allAlarms = session.getAlarms();
      for(Alarm a : allAlarms.values())
      {
         if (a.getKey().equals(key))
            return a;
      }
      return null;
   }

   /**
    * Testing alarm creating functionality
    * 
    * @throws Exception
    */
   @Test
   public void testCreateAlarm() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      session.syncEventTemplates();

      final String templateNameEventDown = "TestEventDown";
      final String templateNameEventUp = "TestEventUp";
      final String ruleEventDownComment = "test comment for TestEventDown event";
      final String ruleEventUpComment = "test comment for TestEventUp event";
      final String alarmKey = "Test Key for TestEventDown event" + new Date().getTime();
      final String alarmMessage = "ALARM MESSAGE for event down";

      Node node = TestHelper.findManagementServer(session);

      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, templateNameEventDown);
      EventTemplate eventTestTemplate2 = TestHelperForEpp.findOrCreateEvent(session, templateNameEventUp);

      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      // Searching for the alarm generation test rule based on the specified comment; if not found, creates a new one.
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, ruleEventDownComment, eventTestTemplate, node);
      testRule.setAlarmKey(alarmKey);
      testRule.setFlags(testRule.getFlags() | testRule.GENERATE_ALARM);
      testRule.setAlarmMessage(alarmMessage + "%N"); // %N to understand which event alarm is generated
      testRule.setAlarmSeverity(Severity.UNKNOWN);
      testRule.setAlarmTimeout(4);
      List<Integer> events = new ArrayList<>();
      events.add(eventTestTemplate.getCode());
      events.add(testRule.getAlarmTimeoutEvent());
      testRule.setEvents(events);
      eventTestTemplate.setSeverity(Severity.NORMAL); // Changing it back so that the test can be run multiple times in a row
                                                      // without deleting EPPR
      session.modifyEventObject(eventTestTemplate);
      session.saveEventProcessingPolicy(policy);

      // Searches for the alarm termination test rule based on the specified comment; if not found, creates a new one.
      EventProcessingPolicyRule testRule2 = TestHelperForEpp.findOrCreateRule(session, policy, ruleEventUpComment, eventTestTemplate2, node);
      testRule2.setAlarmKey(alarmKey);
      testRule2.setAlarmSeverity(Severity.TERMINATE);
      testRule2.setFlags(testRule2.getFlags() | EventProcessingPolicyRule.GENERATE_ALARM | EventProcessingPolicyRule.TERMINATE_BY_REGEXP);
      session.saveEventProcessingPolicy(policy);

      session.sendEvent(0, templateNameEventDown, node.getObjectId(), new String[] {}, null, null, null); // sending event which generated
                                                                                                    // alarm to the server
      Thread.sleep(1000);
      Alarm alarm = findAlarmByKey(session, alarmKey); // founding alarm

      assertNotNull(alarm);
      Thread.sleep(500);

      // Check that the alarm was generated based on the TestEventDown
      assertEquals(alarmMessage + eventTestTemplate.getName(), alarm.getMessage());
      assertEquals(alarm.getCurrentSeverity(), eventTestTemplate.getSeverity());
      assertEquals(Alarm.STATE_OUTSTANDING, alarm.getState());

      Thread.sleep(6000);
      alarm = findAlarmByKey(session, alarmKey);

      // Check that the alarm was generated based on the TimeOutEvent
      assertEquals(alarmMessage + session.getEventName(testRule.getAlarmTimeoutEvent()), alarm.getMessage());
      assertEquals(alarm.getMessage(), alarmMessage + session.getEventName(testRule.getAlarmTimeoutEvent()));

      eventTestTemplate.setSeverity(Severity.CRITICAL); // Changing the alarm severity through the event test template
      session.modifyEventObject(eventTestTemplate);
      session.sendEvent(0, templateNameEventDown, node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(1000);
      alarm = findAlarmByKey(session, alarmKey);
      assertEquals(eventTestTemplate.getSeverity(), alarm.getCurrentSeverity());// checking that alarm severity is Critical

      testRule.setAlarmSeverity(Severity.RESOLVE); // changing alarm STATE
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateNameEventDown, node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(1000);
      alarm = findAlarmByKey(session, alarmKey);

      assertEquals(Alarm.STATE_RESOLVED, alarm.getState());// checking that alarm is resolved

      testRule.setAlarmSeverity(Severity.MAJOR); // Changing the severity of testRule to MAJOR
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, templateNameEventDown, node.getObjectId(), new String[] {}, null, null, null);
      Thread.sleep(1000);
      alarm = findAlarmByKey(session, alarmKey);

      assertEquals(testRule.getAlarmSeverity(), alarm.getCurrentSeverity());// checking that alarm takes severity from rule

      session.sendEvent(0, templateNameEventUp, node.getObjectId(), new String[] {}, null, null, null); // sending rule which terminated
                                                                                                  // alarm
      Thread.sleep(1000);
      alarm = findAlarmByKey(session, alarmKey);
      assertNull(alarm); // checking that cannot find the alarm in the list of alarms, indicating that it is terminated

      session.closeEventProcessingPolicy();
      session.disconnect();
   }
}
