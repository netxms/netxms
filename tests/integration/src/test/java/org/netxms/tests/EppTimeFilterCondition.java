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

import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import java.util.ArrayList;
import java.util.Calendar;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.events.TimeFrame;
import org.netxms.client.objects.AbstractObject;
import org.netxms.utilities.TestHelper;
import org.netxms.utilities.TestHelperForEpp;

public class EppTimeFilterCondition extends AbstractSessionTest
{
   final String TEMPLATE_NAME = "template name for time filter";
   final String COMMENT_FOR_SEARCHING_RULE = "comment for testing time filter";
   final String PS_KEY = "key for testing time filter";
   final String PS_VALUE = "value for testing time filter";
   
   int startHour = 0;
   int startMinute = 0;
   int endHour = 23;
   int endMinute = 59;
   boolean[] daysOfWeek = { true, true, true, true, true, true, true };
   String daysOfMonth = "1-31";
   boolean[] month = { true, true, true, true, true, true, true, true, true, true, true, true }; 

   /**
    * Сreates a new EPP rule with specific parameters
    * 
    * @param session
    * @param node
    * @param policy
    * @param templateName
    * @param commentForSearch
    * @return EPP rule for test
    * @throws Exception
    */
   public EventProcessingPolicyRule createTestRule(NXCSession session, AbstractObject node, EventProcessingPolicy policy, String templateName, String commentForSearch) throws Exception
   {
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, templateName);
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, commentForSearch, eventTestTemplate, node);
      return testRule;
   }

   /**
    * Sets current day of the week
    * 
    * @return day of the week
    * @throws Exception
    */
   
   public boolean[] setCurrentDayOfTheWeek(Calendar calendar) throws Exception
   {
      int currentDayOfWeek = calendar.get(Calendar.DAY_OF_WEEK);

      boolean[] daysOfWeek = new boolean[7];

      for(int i = 0; i < daysOfWeek.length; i++)
      {
         daysOfWeek[i] = (i+2 == currentDayOfWeek);
      }
      return daysOfWeek;
      
   }
   
   /**
    * Sets current month
    * 
    * @return current month
    * @throws Exception
    */
   public boolean[] setCurrentMonth (Calendar calendar) throws Exception
   {
      int currentMonth = calendar.get(Calendar.MONTH);

      boolean[] month = new boolean[12];

      for(int i = 0; i < month.length; i++)
      {
         month[i] = (i == currentMonth);
      }
      return month;
      
   }

   //+---------------------------+
   //| Start time     | 00:00    |
   //|----------------|----------|
   //| End time       | 23:59    |
   //|----------------|----------|
   //|Day of the week | all      |     + INVERSE RULE
   //|----------------|----------|
   //|Day of the month| all      |
   //|----------------|----------|
   //|Month           | all      |
   //+---------------------------+
      
   @Test
   public void testTimeFilterCondition() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      EventProcessingPolicyRule testRule = createTestRule(session, node, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);

      TimeFrame timeFrame = new TimeFrame();
      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, month);

      List<TimeFrame> timeFrameList = new ArrayList<>();
      timeFrameList.add(timeFrame);

      testRule.setTimeFrames(timeFrameList);
      session.saveEventProcessingPolicy(policy);

      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));// checking that PS does not contain a value for the given key
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);

      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));

      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      testRule.setFlags(testRule.getFlags() | testRule.NEGATED_TIME_FRAMES); // making inverse rule
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //to run next test
      testRule.setFlags(testRule.getFlags() & ~testRule.NEGATED_TIME_FRAMES);
      session.saveEventProcessingPolicy(policy);
      

      session.closeEventProcessingPolicy();
   }
   //+---------------------------+
   //| Start time     | current  |    + INVERSE RULE
   //|----------------|----------|
   //| End time       | 23:59    |
   //|----------------|----------|
   //|Day of the week | all      |  
   //|----------------|----------|
   //|Day of the month| all      |
   //|----------------|----------|
   //|Month           | all      |
   //+---------------------------+
   @Test
   public void testTimeFilterCondition2() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      Calendar calendar = Calendar.getInstance();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      int startHour = calendar.get(Calendar.HOUR_OF_DAY);
      int startMinute = calendar.get(Calendar.MINUTE);

      EventProcessingPolicyRule testRule = createTestRule(session, node, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);

      TimeFrame timeFrame = new TimeFrame();
      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, month);

      List<TimeFrame> timeFrameList = new ArrayList<>();
      timeFrameList.add(timeFrame);

      testRule.setTimeFrames(timeFrameList);
      session.saveEventProcessingPolicy(policy);

      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));// checking that PS does not contain a value for the given key
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);

      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));

      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //making inverse rule

      testRule.setFlags(testRule.getFlags() | testRule.NEGATED_TIME_FRAMES); 
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //Setting the time that will be outside the bounds of the time rule
      calendar = Calendar.getInstance(); 

      calendar.add(Calendar.MINUTE, 2);
      startHour = calendar.get(Calendar.HOUR_OF_DAY);
      startMinute = calendar.get(Calendar.MINUTE);
      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, month);

      testRule.setTimeFrames(timeFrameList);
      testRule.setFlags(testRule.getFlags() & ~testRule.NEGATED_TIME_FRAMES);

      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.closeEventProcessingPolicy();
   }
   
   //+------------------------------+
   //| Start time     | current     |    + INVERSE RULE
   //|----------------|-------------|
   //| End time       |current +2min|
   //|----------------|-------------|
   //|Day of the week | all         |  
   //|----------------|-------------|
   //|Day of the month| all         |
   //|----------------|-------------|
   //|Month           | all         |
   //+------------------------------+
   @Test
   public void testTimeFilterCondition3() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      Calendar calendar = Calendar.getInstance();

      int startHour = calendar.get(Calendar.HOUR_OF_DAY);
      int startMinute = calendar.get(Calendar.MINUTE);
           
      int endMinute = (startMinute + 2) % 60;
      int endHour = startHour + (startMinute + 2) / 60;

      EventProcessingPolicyRule testRule = createTestRule(session, node, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);

      TimeFrame timeFrame = new TimeFrame();
      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, month);

      List<TimeFrame> timeFrameList = new ArrayList<>();
      timeFrameList.add(timeFrame);

      testRule.setTimeFrames(timeFrameList); 
      session.saveEventProcessingPolicy(policy);

      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));// checking that PS does not contain a value for the given key
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);

      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));

      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      testRule.setFlags(testRule.getFlags() | testRule.NEGATED_TIME_FRAMES); //making inverse rule
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //Setting the time that will be outside the bounds of the time rule
      calendar = Calendar.getInstance(); 

      calendar.add(Calendar.MINUTE, 2);
      startHour = calendar.get(Calendar.HOUR_OF_DAY);
      startMinute = calendar.get(Calendar.MINUTE);
      endMinute = (startMinute + 2) % 60;
      endHour = startHour + (startMinute + 2) / 60;
      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, month);

      testRule.setTimeFrames(timeFrameList);
      testRule.setFlags(testRule.getFlags() & ~testRule.NEGATED_TIME_FRAMES);

      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));

      session.closeEventProcessingPolicy();
   }
   
   //+---------------------------+
   //| Start time     | 00:00    |
   //|----------------|----------|
   //| End time       | current  |   + INVERSE RULE
   //|----------------|----------|
   //|Day of the week | all      |      
   //|----------------|----------|
   //|Day of the month| all      |
   //|----------------|----------|
   //|Month           | all      |
   //+---------------------------+
   @Test
   public void testTimeFilterCondition4() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      Calendar calendar = Calendar.getInstance();

      int endHour = calendar.get(Calendar.HOUR_OF_DAY);
      int endMinute = calendar.get(Calendar.MINUTE);

      EventProcessingPolicyRule testRule = createTestRule(session, node, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      
      TimeFrame timeFrame = new TimeFrame();
      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, month);

      List<TimeFrame> timeFrameList = new ArrayList<>();
      timeFrameList.add(timeFrame);

      testRule.setTimeFrames(timeFrameList);
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));// checking that PS does not contain a value for the given key
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);

      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      testRule.setFlags(testRule.getFlags() | testRule.NEGATED_TIME_FRAMES); //making inverse rule
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //Setting the time that will be outside the bounds of the time rule
      calendar = Calendar.getInstance(); 

      calendar.add(Calendar.MINUTE, -2);
      endHour = calendar.get(Calendar.HOUR);
      endMinute = calendar.get(Calendar.MINUTE);
      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, month);

      testRule.setTimeFrames(timeFrameList);
      testRule.setFlags(testRule.getFlags() & ~testRule.NEGATED_TIME_FRAMES);

      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.closeEventProcessingPolicy();
   }
   
   //+---------------------------+
   //| Start time     | 00:00    |
   //|----------------|----------|
   //| End time       | 23:59    |
   //|----------------|----------|
   //|Day of the week | current  |    + INVERSE RULE
   //|----------------|----------|
   //|Day of the month| all      |
   //|----------------|----------|
   //|Month           | all      |
   //+---------------------------+
   @Test
   public void testTimeFilterCondition5() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      Calendar calendar = Calendar.getInstance();

      boolean[] daysOfWeek = setCurrentDayOfTheWeek(calendar);

      EventProcessingPolicyRule testRule = createTestRule(session, node, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);

      TimeFrame timeFrame = new TimeFrame();
      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, month);

      List<TimeFrame> timeFrameList = new ArrayList<>();
      timeFrameList.add(timeFrame);

      testRule.setTimeFrames(timeFrameList);
      session.saveEventProcessingPolicy(policy);

      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));// checking that PS does not contain a value for the given key
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);

      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));

      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
       
      testRule.setFlags(testRule.getFlags() | testRule.NEGATED_TIME_FRAMES); //making inverse rule
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //Setting the day that will be outside the bounds of the time rule
      calendar = Calendar.getInstance(); 
      int currentDayOfWeek = calendar.get(Calendar.DAY_OF_WEEK);
      int nextDayOfWeek = (currentDayOfWeek + 1) % 7;

      boolean[] nextDayOfWeekArray = new boolean[7];

      for (int i = 0; i < 7; i++) {
         if (i == nextDayOfWeek) {
            nextDayOfWeekArray[i] = true;
        } else {
            nextDayOfWeekArray[i] = false;
        }         }

      timeFrame.update(startHour, startMinute, endHour, endMinute, nextDayOfWeekArray, daysOfMonth, month);

      testRule.setTimeFrames(timeFrameList);
      testRule.setFlags(testRule.getFlags() & ~testRule.NEGATED_TIME_FRAMES);

      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));

      session.closeEventProcessingPolicy();
   }

   //+---------------------------+
   //| Start time     | 00:00    |
   //|----------------|----------|
   //| End time       | 23:59    |
   //|----------------|----------|
   //|Day of the week | all      |
   //|----------------|----------|
   //|Day of the month| current  |    + INVERSE RULE
   //|----------------|----------|
   //|Month           | all      |
   //+---------------------------+
   @Test
   public void testTimeFilterCondition6() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      Calendar calendar = Calendar.getInstance();
      
      String daysOfMonth = Integer.toString(calendar.get(calendar.DAY_OF_MONTH)); 

      EventProcessingPolicyRule testRule = createTestRule(session, node, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);

      TimeFrame timeFrame = new TimeFrame();
      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, month);

      List<TimeFrame> timeFrameList = new ArrayList<>();
      timeFrameList.add(timeFrame);

      testRule.setTimeFrames(timeFrameList);
      session.saveEventProcessingPolicy(policy);

      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));// checking that PS does not contain a value for the given key
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);

      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));

      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      testRule.setFlags(testRule.getFlags() | testRule.NEGATED_TIME_FRAMES); //making inverse rule
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //Setting the day that will be outside the bounds of the time rule
      calendar = Calendar.getInstance(); 

      calendar.add(Calendar.DAY_OF_MONTH, 1);
      daysOfMonth = Integer.toString(calendar.get(calendar.DAY_OF_MONTH)); 
      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, month);

      testRule.setTimeFrames(timeFrameList);
      testRule.setFlags(testRule.getFlags() & ~testRule.NEGATED_TIME_FRAMES);

      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));

      session.closeEventProcessingPolicy();
   }
   
   //+---------------------------+
   //| Start time     | 00:00    |
   //|----------------|----------|
   //| End time       | 23:59    |
   //|----------------|----------|
   //|Day of the week | all      |
   //|----------------|----------|
   //|Day of the month| all      |
   //|----------------|----------|
   //|Month           | current  |    + INVERSE RULE
   //+---------------------------+
   @Test
   public void testTimeFilterCondition7() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      Calendar calendar = Calendar.getInstance();

      boolean[] CurrentMonthArray = setCurrentMonth(calendar);

      EventProcessingPolicyRule testRule = createTestRule(session, node, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);

      TimeFrame timeFrame = new TimeFrame();
      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, CurrentMonthArray);

      List<TimeFrame> timeFrameList = new ArrayList<>();
      timeFrameList.add(timeFrame);

      testRule.setTimeFrames(timeFrameList);
      session.saveEventProcessingPolicy(policy);

      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));// checking that PS does not contain a value for the given key
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);

      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));

      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      testRule.setFlags(testRule.getFlags() | testRule.NEGATED_TIME_FRAMES); //making inverse rule
      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //Setting the month that will be outside the bounds of the time rule
      calendar = Calendar.getInstance(); 
      int currentMonth = calendar.get(Calendar.DAY_OF_MONTH);
      int nextMonth = (currentMonth + 1) % 12;

      boolean[] nextMonthArray = new boolean[12];
      for (int i = 0; i < 12; i++) {
         if (i == nextMonth) {
            nextMonthArray[i] = true;
        } else {
           nextMonthArray[i] = false;
        }         }

      timeFrame.update(startHour, startMinute, endHour, endMinute, daysOfWeek, daysOfMonth, nextMonthArray);

      testRule.setTimeFrames(timeFrameList);
      testRule.setFlags(testRule.getFlags() & ~testRule.NEGATED_TIME_FRAMES);

      session.saveEventProcessingPolicy(policy);
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));

      session.closeEventProcessingPolicy();
   }
}
