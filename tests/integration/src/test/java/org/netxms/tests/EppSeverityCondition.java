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
import java.util.HashMap;
import java.util.Map;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.utilities.TestHelper;
import org.netxms.utilities.TestHelperForEpp;

public class EppSeverityCondition  extends AbstractSessionTest
{
   final String TEMPLATE_NAME = "template name for severity filter";
   final String COMMENT_FOR_SEARCHING_RULE = "comment for testing severity filter";
   final String PS_KEY = "key for testing severity filter";
   final String PS_VALUE = "value for testing severity filter";

   //+-----------------+      +----------------------+
   //|severity of rule |      | severity of condition|
   //|-----------------|      |----------------------|
   //|   NORMAL        |      |       NORMAL         |
   //+-----------------+      +----------------------+
   @Test
   public void testSeverityFilterCondition() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, TEMPLATE_NAME);
      eventTestTemplate.setSeverity(Severity.NORMAL); // Changing the event severity 
      session.modifyEventObject(eventTestTemplate);
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, COMMENT_FOR_SEARCHING_RULE, eventTestTemplate, node);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      
      testRule.setFlags(testRule.SEVERITY_NORMAL);
      session.saveEventProcessingPolicy(policy);
      
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);   
            
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //to run next test
      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      testRule.setFlags(testRule.SEVERITY_ANY);
      session.saveEventProcessingPolicy(policy);
      
      session.closeEventProcessingPolicy();
   }
   
   //+-----------------+      +----------------------+
   //|severity of rule |      | severity of condition|
   //|-----------------|      |----------------------|
   //|   NORMAL        |      |ALL EXCEPT NORMAL     |
   //+-----------------+      +----------------------+
   @Test
   public void testSeverityFilterCondition2() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, TEMPLATE_NAME);
      eventTestTemplate.setSeverity(Severity.NORMAL); // Changing the event severity 
      session.modifyEventObject(eventTestTemplate);
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, COMMENT_FOR_SEARCHING_RULE, eventTestTemplate, node);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      
      testRule.setFlags(testRule.getFlags() & ~testRule.SEVERITY_NORMAL);
      session.saveEventProcessingPolicy(policy);
      
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);   
            
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //to run next test
      testRule.setFlags(testRule.SEVERITY_ANY);
      session.saveEventProcessingPolicy(policy);
      
      session.closeEventProcessingPolicy();
   }
   
   //+-----------------+      +----------------------+
   //|severity of rule |      | severity of condition|
   //|-----------------|      |----------------------|
   //|   CRITICAL      |      |       CRITICAL       |
   //+-----------------+      +----------------------+
   @Test
   public void testSeverityFilterCondition3() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, TEMPLATE_NAME);
      eventTestTemplate.setSeverity(Severity.CRITICAL); // Changing the event severity 
      session.modifyEventObject(eventTestTemplate);
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, COMMENT_FOR_SEARCHING_RULE, eventTestTemplate, node);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      
      testRule.setFlags(testRule.SEVERITY_CRITICAL);
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //to run next test
      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      testRule.setFlags(testRule.SEVERITY_ANY);
      session.saveEventProcessingPolicy(policy);
      
      session.closeEventProcessingPolicy();
   }
   
   //+-----------------+      +----------------------+
   //|severity of rule |      | severity of condition|
   //|-----------------|      |----------------------|
   //|   CRITICAL      |      | ALL EXCEPT CRITICAL  |
   //+-----------------+      +----------------------+
   @Test
   public void testSeverityFilterCondition4() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, TEMPLATE_NAME);
      eventTestTemplate.setSeverity(Severity.CRITICAL); // Changing the event severity 
      session.modifyEventObject(eventTestTemplate);
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, COMMENT_FOR_SEARCHING_RULE, eventTestTemplate, node);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      
      testRule.setFlags(testRule.getFlags() & ~testRule.SEVERITY_CRITICAL);
      
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //to run next test
      testRule.setFlags(testRule.SEVERITY_ANY);
      session.saveEventProcessingPolicy(policy);
      
      session.closeEventProcessingPolicy();
   }
   
   //+-----------------+      +----------------------+
   //|severity of rule |      | severity of condition|
   //|-----------------|      |----------------------|
   //|   WARNING       |      |       WARNING        |
   //+-----------------+      +----------------------+
   @Test
   public void testSeverityFilterCondition5() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, TEMPLATE_NAME);
      eventTestTemplate.setSeverity(Severity.WARNING); // Changing the event severity 
      session.modifyEventObject(eventTestTemplate);
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, COMMENT_FOR_SEARCHING_RULE, eventTestTemplate, node);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      
      testRule.setFlags(testRule.SEVERITY_WARNING);
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //to run next test
      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      testRule.setFlags(testRule.SEVERITY_ANY);
      session.saveEventProcessingPolicy(policy);
      
      session.closeEventProcessingPolicy();
   }
   
   //+-----------------+      +---------------------+
   //|severity of rule |      |severity of condition|
   //|-----------------|      |---------------------|
   //|   WARNING       |      | ALL EXCEPT WARNING  |
   //+-----------------+      +---------------------+
   @Test
   public void testSeverityFilterCondition6() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, TEMPLATE_NAME);
      eventTestTemplate.setSeverity(Severity.WARNING); // Changing the event severity 
      session.modifyEventObject(eventTestTemplate);
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, COMMENT_FOR_SEARCHING_RULE, eventTestTemplate, node);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      
      testRule.setFlags(testRule.getFlags() & ~testRule.SEVERITY_WARNING);
      
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //to run next test
      testRule.setFlags(testRule.SEVERITY_ANY);
      session.saveEventProcessingPolicy(policy);
      
      session.closeEventProcessingPolicy();
   }
   
   //+-----------------+      +----------------------+
   //|severity of rule |      | severity of condition|
   //|-----------------|      |----------------------|
   //|   MINOR         |      |       MINOR          |
   //+-----------------+      +----------------------+
   @Test
   public void testSeverityFilterCondition7() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, TEMPLATE_NAME);
      eventTestTemplate.setSeverity(Severity.MINOR); // Changing the event severity 
      session.modifyEventObject(eventTestTemplate);
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, COMMENT_FOR_SEARCHING_RULE, eventTestTemplate, node);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      
      testRule.setFlags(testRule.SEVERITY_MINOR);
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //to run next test
      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      testRule.setFlags(testRule.SEVERITY_ANY);
      session.saveEventProcessingPolicy(policy);
      
      session.closeEventProcessingPolicy();
   }
   
   //+-----------------+      +---------------------+
   //|severity of rule |      |severity of condition|
   //|-----------------|      |---------------------|
   //|   MINOR         |      | ALL EXCEPT MINOR    |
   //+-----------------+      +---------------------+
   @Test
   public void testSeverityFilterCondition8() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, TEMPLATE_NAME);
      eventTestTemplate.setSeverity(Severity.MINOR); // Changing the event severity 
      session.modifyEventObject(eventTestTemplate);
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, COMMENT_FOR_SEARCHING_RULE, eventTestTemplate, node);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      
      testRule.setFlags(testRule.getFlags() & ~testRule.SEVERITY_MINOR);
      
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //to run next test
      testRule.setFlags(testRule.SEVERITY_ANY);
      session.saveEventProcessingPolicy(policy);
      
      session.closeEventProcessingPolicy();
   }
   
   //+-----------------+      +----------------------+
   //|severity of rule |      | severity of condition|
   //|-----------------|      |----------------------|
   //|   MAJOR         |      |       MAJOR          |
   //+-----------------+      +----------------------+
   @Test
   public void testSeverityFilterCondition9() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, TEMPLATE_NAME);
      eventTestTemplate.setSeverity(Severity.MAJOR); // Changing the event severity 
      session.modifyEventObject(eventTestTemplate);
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, COMMENT_FOR_SEARCHING_RULE, eventTestTemplate, node);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      
      testRule.setFlags(testRule.SEVERITY_MAJOR);
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNotNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //to run next test
      session.deletePersistentStorageValue(PS_KEY);
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      testRule.setFlags(testRule.SEVERITY_ANY);
      session.saveEventProcessingPolicy(policy);
      
      session.closeEventProcessingPolicy();
   }
   
   //+-----------------+      +---------------------+
   //|severity of rule |      |severity of condition|
   //|-----------------|      |---------------------|
   //|   MAJOR         |      | ALL EXCEPT MAJOR    |
   //+-----------------+      +---------------------+
   @Test
   public void testSeverityFilterCondition10() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      AbstractObject node = TestHelper.findManagementServer(session);
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed
      
      EventTemplate eventTestTemplate = TestHelperForEpp.findOrCreateEvent(session, TEMPLATE_NAME);
      eventTestTemplate.setSeverity(Severity.MAJOR); // Changing the event severity 
      session.modifyEventObject(eventTestTemplate);
      
      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, COMMENT_FOR_SEARCHING_RULE, eventTestTemplate, node);
      Map<String, String> rulePsSetList = new HashMap<>();
      rulePsSetList.put(PS_KEY, PS_VALUE);// creating a key-value set to pass into the rule
      testRule.setPStorageSet(rulePsSetList);
      
      testRule.setFlags(testRule.getFlags() & ~testRule.SEVERITY_MAJOR);
      
      session.saveEventProcessingPolicy(policy);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      session.sendEvent(0, TEMPLATE_NAME, node.getObjectId(), new String[] {}, null, null, null);
      
      assertNull(TestHelperForEpp.findPsValueByKey(session, PS_KEY));
      
      //to run next test
      testRule.setFlags(testRule.SEVERITY_ANY);
      session.saveEventProcessingPolicy(policy);
      
      session.closeEventProcessingPolicy();
   }

}
