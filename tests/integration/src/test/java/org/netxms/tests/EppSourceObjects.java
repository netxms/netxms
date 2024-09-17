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
import static org.junit.jupiter.api.Assertions.assertNull;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.utilities.TestHelperForEpp;

public class EppSourceObjects extends AbstractSessionTest
{
   final static String TEMPLATE_NAME = "template name for source object";
   final static String COMMENT_FOR_SEARCHING_RULE = "comment for testing source object";
   final static String CA_VALUE = "CA value";

   public static String TEST_NODE_A = "node-a";
   public static String TEST_NODE_AB = "node-ab";
   public static String TEST_NODE_B1 = "node-b1";
   public static String TEST_NODE_B = "node-b";

   public static String TEST_CONTAINER_A = "container-a";
   public static String TEST_CONTAINER_AB = "container-ab";
   public static String TEST_CONTAINER_B = "container-b";
   public static String TEST_CONTAINER_B1 = "container-b1";

   
   
   
   //                  INFRASTRUCTURE 
   //                  SERVICES
   //               /               \
   //           +------+             +------+
   //           |  A   |             |  B   |
   //           +------+             +------+
   //        /           \         /      |      \
   //   +-------+         +-------+   +------+   +--------+
   //   |NODE A |         |  AB   |   |  B1  |   | NODE B |
   //   +-------+         +-------+   +------+   +--------+
   //                        |             |   
   //                     +-------+     +------+
   //                     |NODE AB|     |NODE B|
   //                     +-------+     +------+
   
   
   /**
    * Checks if a container with specified parameters exists. 
    * If it doesn't exist, creates a new A container with its sub-containers and nodes within them.
    * 
    * @param session
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static void checkContainerACreated(NXCSession session) throws IOException, NXCException, InterruptedException
   {
      boolean containerFound = false;

      for(AbstractObject object : session.getAllObjects())
      {
         if (object instanceof Container && ((Container)object).getObjectName().equals(TEST_CONTAINER_A))
         {
            containerFound = true;
            break;
         }
      }

      if (!containerFound)
      {
         NXCObjectCreationData containerACd = new NXCObjectCreationData(GenericObject.OBJECT_CONTAINER, TEST_CONTAINER_A, GenericObject.SERVICEROOT);
         Container containerA = (Container)session.createObjectSync(containerACd);

         NXCObjectCreationData nodeACd = new NXCObjectCreationData(GenericObject.OBJECT_NODE, TEST_NODE_A, containerA.getObjectId());
         session.createObjectSync(nodeACd);

          NXCObjectCreationData containerAbCd = new NXCObjectCreationData(GenericObject.OBJECT_CONTAINER, TEST_CONTAINER_AB,
          containerA.getObjectId());
          Container containerAB = (Container)session.createObjectSync(containerAbCd);

          NXCObjectCreationData nodeAbCd = new NXCObjectCreationData(GenericObject.OBJECT_NODE, TEST_NODE_AB,
          containerAB.getObjectId());
          session.createObjectSync(nodeAbCd);
      }
   }

   /**
    * Checks if a container with specified parameters exists. 
    * If it doesn't exist, creates a new B container with its sub-containers and nodes within them.
    * 
    * @param session
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static void checkContainerBCreated(NXCSession session) throws IOException, NXCException, InterruptedException
   {
      boolean containerFound = false;

      for(AbstractObject object : session.getAllObjects())
      {
         if (object instanceof Container && ((Container)object).getObjectName().equals(TEST_CONTAINER_B))
         {
            containerFound = true;
            break;
         }
      }

      if (!containerFound)
      {
         NXCObjectCreationData containerBCd = new NXCObjectCreationData(GenericObject.OBJECT_CONTAINER, TEST_CONTAINER_B, GenericObject.SERVICEROOT);
         Container containerB = (Container)session.createObjectSync(containerBCd);

         NXCObjectCreationData nodeBCd = new NXCObjectCreationData(GenericObject.OBJECT_NODE, TEST_NODE_B, containerB.getObjectId());
         session.createObjectSync(nodeBCd);

         NXCObjectCreationData containerB1Cd = new NXCObjectCreationData(GenericObject.OBJECT_CONTAINER, TEST_CONTAINER_B1, containerB.getObjectId());
         Container containerB1 = (Container)session.createObjectSync(containerB1Cd);

         NXCObjectCreationData nodeB1Cd = new NXCObjectCreationData(GenericObject.OBJECT_NODE, TEST_NODE_B1, containerB1.getObjectId());
         session.createObjectSync(nodeB1Cd);

         session.bindObject(containerB.getObjectId(), session.findObjectByName(TEST_CONTAINER_AB).getObjectId());
      }
   }

   /**
    * Removes a custom attribute list from a specified node. 
    * 
    * @param session
    * @param nodeName
    * @throws Exception
    */
   public static void deleteCaListFromNode(NXCSession session, String nodeName) throws Exception
   {
      AbstractObject node = session.findObjectByName(nodeName);
      Map<String, CustomAttribute> attrList = new HashMap <>();
      session.setObjectCustomAttributes(node.getObjectId(), attrList);
   }
   
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
   public static EventProcessingPolicyRule createTestRule (NXCSession session, AbstractObject node, EventProcessingPolicy policy, String templateName, String commentForSearch) throws Exception
   {
      EventTemplate eventTemplate = TestHelperForEpp.findOrCreateEvent(session, templateName);

      EventProcessingPolicyRule testRule = TestHelperForEpp.findOrCreateRule(session, policy, commentForSearch, eventTemplate, node);
      return testRule;
   }
   
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |     B     |            |
   //        +-----------+------------+
   @Test
   public void testSourceObjects() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);
      
      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
            
      List<Long> listForInclusion = new ArrayList<Long>();
      listForInclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() & ~ EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A)); 
      
      assertEquals(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB), CA_VALUE);

      //To run next test
      deleteCaListFromNode(session, TEST_NODE_B);
      deleteCaListFromNode(session, TEST_NODE_B1);
      deleteCaListFromNode(session, TEST_NODE_AB);

      Thread.sleep(500);
      
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.closeEventProcessingPolicy();
   }
   
   //        INVERSE RULE
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |     B     |            |
   //        +-----------+------------+
   @Test
   public void testSourceObjects2() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
            
      List<Long> listForInclusion = new ArrayList<Long>();
      listForInclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSources(listForInclusion);
      
      testRule.setFlags(testRule.getFlags() | EventProcessingPolicyRule.NEGATED_SOURCE);
      
      List<Long> listForExclusion = new ArrayList<Long>();
      testRule.setSourceExclusions(listForExclusion);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);
      
      assertEquals(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A), CA_VALUE); 
      
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      //To run next test
      deleteCaListFromNode(session, TEST_NODE_A);
      
      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));

      session.closeEventProcessingPolicy();
   }

   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |           |     B      |
   //        +-----------+------------+
   @Test
   public void testSourceObjects3() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      
      List<Long> listForInclusion = new ArrayList<Long>();
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      listForExclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() & ~ EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);

      assertEquals(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A), CA_VALUE); 
      
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      deleteCaListFromNode(session, TEST_NODE_A);
      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));

      session.closeEventProcessingPolicy();
   }
   
   //        INVERSE RULE
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |           |     B      |
   //        +-----------+------------+
   @Test
   public void testSourceObjects4() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);

      List<Long> listForInclusion = new ArrayList<Long>();
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      listForExclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() | EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);
      
      Thread.sleep(500);

      
      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      
      assertEquals(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB), CA_VALUE);

      deleteCaListFromNode(session, TEST_NODE_B);
      deleteCaListFromNode(session, TEST_NODE_B1);
      deleteCaListFromNode(session, TEST_NODE_AB);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.closeEventProcessingPolicy();
   }
   
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |     B1    |            |
   //        +-----------+------------+
   @Test
   public void testSourceObjects5() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      
      List<Long> listForInclusion = new ArrayList<Long>();
      listForInclusion.add(session.findObjectByName(TEST_CONTAINER_B1).getObjectId());
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() & ~ EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);
      
      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);

      assertEquals(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1), CA_VALUE);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));
      
      //to run next test
      deleteCaListFromNode(session, TEST_NODE_B1);
      
      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_B1));

      session.closeEventProcessingPolicy();
   }
   
   //        INVERSE RULE
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |     B1    |            |
   //        +-----------+------------+
   @Test
   public void testSourceObjects6() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      
      List<Long> listForInclusion = new ArrayList<Long>();
      listForInclusion.add(session.findObjectByName(TEST_CONTAINER_B1).getObjectId());
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() | EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));

      assertEquals(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB), CA_VALUE);

      //to run next test
      deleteCaListFromNode(session, TEST_NODE_A);
      deleteCaListFromNode(session, TEST_NODE_B);
      deleteCaListFromNode(session, TEST_NODE_AB);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.closeEventProcessingPolicy();
   }
   
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |           |     B1     |
   //        +-----------+------------+
   @Test
   public void testSourceObjects7() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      
      List<Long> listForInclusion = new ArrayList<Long>();
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      listForExclusion.add(session.findObjectByName(TEST_CONTAINER_B1).getObjectId());
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() & ~ EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));

      assertEquals(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB), CA_VALUE);

      deleteCaListFromNode(session, TEST_NODE_A);
      deleteCaListFromNode(session, TEST_NODE_B);
      deleteCaListFromNode(session, TEST_NODE_AB);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));


      session.closeEventProcessingPolicy();
   }
   //        INVERSE RULE
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |           |     B1     |
   //        +-----------+------------+
   @Test
   public void testSourceObjects8() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      
      List<Long> listForInclusion = new ArrayList<Long>();
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      listForExclusion.add(session.findObjectByName(TEST_CONTAINER_B1).getObjectId());
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() | EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);

      assertEquals(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1), CA_VALUE);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));
      
      deleteCaListFromNode(session, TEST_NODE_B1);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));

      session.closeEventProcessingPolicy();
   }
   
   
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |     B     |     B      |
   //        +-----------+------------+
   @Test
   public void testSourceObjects9() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      
      List<Long> listForInclusion = new ArrayList<Long>();
      listForInclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      listForExclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() & ~ EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.closeEventProcessingPolicy();
   }
   
      
   //        INVERSE RULE
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |     B     |     B      |
   //        +-----------+------------+
   @Test
   public void testSourceObjects10() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      
      List<Long> listForInclusion = new ArrayList<Long>();
      listForInclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      listForExclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() | EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);
      
      Thread.sleep(500);
      
      assertEquals(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB), CA_VALUE);

      deleteCaListFromNode(session, TEST_NODE_A);
      deleteCaListFromNode(session, TEST_NODE_B);
      deleteCaListFromNode(session, TEST_NODE_B1);
      deleteCaListFromNode(session, TEST_NODE_AB);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));
      
      session.closeEventProcessingPolicy();
   }

   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |    IS     |     B      |
   //        +-----------+------------+
   @Test
   public void testSourceObjects11() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      
      List<Long> listForInclusion = new ArrayList<Long>();
      listForInclusion.add(session.findObjectByName("Infrastructure Services").getObjectId());
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      listForExclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() & ~ EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);

      assertEquals(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A), CA_VALUE);
      
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));
      
      deleteCaListFromNode(session, TEST_NODE_A);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));

      session.closeEventProcessingPolicy();
   }
   
   //        INVERSE RULE
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |    IS     |     B      |
   //        +-----------+------------+
   @Test
   public void testSourceObjects12() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      
      List<Long> listForInclusion = new ArrayList<Long>();
      listForInclusion.add(session.findObjectByName("Infrastructure Services").getObjectId());
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      listForExclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() | EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      
      assertEquals(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB), CA_VALUE);
      
      deleteCaListFromNode(session, TEST_NODE_B);
      deleteCaListFromNode(session, TEST_NODE_B1);
      deleteCaListFromNode(session, TEST_NODE_AB);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.closeEventProcessingPolicy();
   }
   
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |    B      |     IS     |
   //        +-----------+------------+
   @Test
   public void testSourceObjects13() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      
      List<Long> listForInclusion = new ArrayList<Long>();
      listForInclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      listForExclusion.add(session.findObjectByName("Infrastructure Services").getObjectId());
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() & ~ EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.closeEventProcessingPolicy();
   }
      
   //        INVERSE RULE
   //        +-----------+------------+
   //        | INCLUSION | EXCLUSION  |
   //        |    B      |     IS     |
   //        +-----------+------------+
   @Test
   public void testSourceObjects14() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();
      EventProcessingPolicy policy = session.openEventProcessingPolicy();// To make this work, EPP rules must be closed

      checkContainerACreated(session);
      checkContainerBCreated(session);

      EventProcessingPolicyRule testRule = createTestRule(session, null, policy, TEMPLATE_NAME, COMMENT_FOR_SEARCHING_RULE);
      
      List<Long> listForInclusion = new ArrayList<Long>();
      listForInclusion.add(session.findObjectByName(TEST_CONTAINER_B).getObjectId());
      testRule.setSources(listForInclusion);
      List<Long> listForExclusion = new ArrayList<Long>();
      listForExclusion.add(session.findObjectByName("Infrastructure Services").getObjectId());
      testRule.setSourceExclusions(listForExclusion);
      
      testRule.setFlags(testRule.getFlags() | EventProcessingPolicyRule.NEGATED_SOURCE);
      
      Map<String, String> ruleCaSetList = new HashMap<>();
      ruleCaSetList.put("%n", "CA value");// creating a key-value set to pass into the rule
      testRule.setCustomAttributeStorageSet(ruleCaSetList);
      session.saveEventProcessingPolicy(policy);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));

      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_A).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_B1).getObjectId(), new String[] {}, null, null, null);
      session.sendEvent(0, TEMPLATE_NAME, session.findObjectByName(TEST_NODE_AB).getObjectId(), new String[] {}, null, null, null);

      Thread.sleep(500);

      assertEquals(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1), CA_VALUE);
      assertEquals(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB), CA_VALUE);

      deleteCaListFromNode(session, TEST_NODE_A);
      deleteCaListFromNode(session, TEST_NODE_B);
      deleteCaListFromNode(session, TEST_NODE_B1);
      deleteCaListFromNode(session, TEST_NODE_AB);
      
      Thread.sleep(500);

      assertNull(session.findObjectByName(TEST_NODE_A).getCustomAttributeValue(TEST_NODE_A));
      assertNull(session.findObjectByName(TEST_NODE_B).getCustomAttributeValue(TEST_NODE_B));
      assertNull(session.findObjectByName(TEST_NODE_B1).getCustomAttributeValue(TEST_NODE_B1));
      assertNull(session.findObjectByName(TEST_NODE_AB).getCustomAttributeValue(TEST_NODE_AB));
      
      session.closeEventProcessingPolicy();
   }

}