/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Raden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.tests;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.net.InetAddress;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.Date;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.apache.commons.io.IOUtils;
import org.junit.jupiter.api.Test;
import org.netxms.base.InetAddressEx;
import org.netxms.base.MacAddress;
import org.netxms.client.AgentPolicy;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.ScheduledTask;
import org.netxms.client.ScriptCompilationResult;
import org.netxms.client.TextOutputListener;
import org.netxms.client.UserAgentNotification;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.WebServiceDefinition;
import org.netxms.client.events.Alarm;
import org.netxms.client.mt.MappingTable;
import org.netxms.client.mt.MappingTableEntry;
import org.netxms.client.objects.AbstractObject;
import org.netxms.utilities.TestHelper;

/**
 * Tests for scripting functions
 */
public class ScriptTest extends AbstractSessionTest implements TextOutputListener
{
   private NXCSession session;
   private Boolean scriptFailed;
   private String currentScriptName = "<unset>";
   
   @Test
   public void testAddressMap() throws Exception
   {
      final NXCSession session = connectAndLogin();

      ScriptCompilationResult r = session.compileScript("a = 1; b = 2; return a / b;", true);
      assertTrue(r.success);
      assertNotNull(r.code);
      assertNull(r.errorMessage);
      
      r = session.compileScript("a = 1; b = 2; return a / b;", false);
      assertTrue(r.success);
      assertNull(r.code);
      assertNull(r.errorMessage);

      r = session.compileScript("a = 1*; b = 2; return a / b;", false);
      assertFalse(r.success);
      assertNull(r.code);
      assertNotNull(r.errorMessage);
      System.out.println("ScriptTest::testAddressMap(): Expected compilation error message: \"" + r.errorMessage + "\"");

      session.disconnect();
   }

   @Test
   private void executeScript(String resourceName, List<String> params) throws Exception
   {
      currentScriptName = resourceName;
      String script = IOUtils.toString(this.getClass().getResourceAsStream(resourceName), "UTF-8");
      ScriptCompilationResult r = session.compileScript(script, true);
      if (r.errorMessage != null)
         System.out.println("Script " + resourceName + " compilation error: \"" + r.errorMessage + "\"");
      assertTrue(r.success);
      assertNotNull(r.code);
      assertNull(r.errorMessage);

      scriptFailed = false;
      session.executeScript(2, script, params, ScriptTest.this);
      assertFalse(scriptFailed);         
   }
   
   @Test
   public void testNXSLObjectFunctions() throws Exception
   {
      session = connectAndLogin();
      session.setCommandTimeout(120000);

      session.syncObjects();
      AbstractObject object = session.findObjectById(2);
      assertNotNull(object);

      NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, "TestNode", 2);
      cd.setCreationFlags(NXCObjectCreationData.CF_CREATE_UNMANAGED);
      cd.setIpAddress(new InetAddressEx(InetAddress.getByName("0.0.0.0"), 0));
      long nodeId = session.createObject(cd);
      assertFalse(nodeId == 0);

      long templateId = 0;
      try
      {
         object = session.findObjectById(3);
         assertNotNull(object);

         cd = new NXCObjectCreationData(AbstractObject.OBJECT_TEMPLATE, "TestTemplate", 3);
         templateId = session.createObject(cd);
         assertFalse(templateId == 0);

         Thread.sleep(1000); // Object update should be received from server

         object = session.findObjectById(nodeId);
         assertNotNull(object);
         object = session.findObjectById(templateId);
         assertNotNull(object);

         session.applyTemplate(templateId, nodeId);

         String interfaceName1 = "lo";
         cd = new NXCObjectCreationData(AbstractObject.OBJECT_INTERFACE, interfaceName1, nodeId);
         cd.setMacAddress(MacAddress.parseMacAddress("00:10:FA:23:11:7A"));
         cd.setIpAddress(new InetAddressEx(InetAddress.getLoopbackAddress(), 0));
         cd.setIfType(24);
         cd.setIfIndex(0);
         session.createObject(cd);

         String interfaceName2 = "eth0";
         cd = new NXCObjectCreationData(AbstractObject.OBJECT_INTERFACE, interfaceName2, nodeId);
         cd.setMacAddress(MacAddress.parseMacAddress("01 02 fa c4 10 dc"));
         cd.setIpAddress(new InetAddressEx(InetAddress.getByName("192.168.10.1"), 24));
         cd.setIfType(6);
         cd.setIfIndex(1);
         session.createObject(cd);

         List<String> params = new ArrayList<String>();
         params.add(Long.toString(nodeId));
         params.add(Long.toString(templateId));
         params.add(interfaceName1);
         params.add(interfaceName2);
         executeScript("/objectFunctions.nxsl", params);
      }
      finally
      {
         if (nodeId != 0)
            session.deleteObject(nodeId);

         if (templateId != 0)
            session.deleteObject(templateId);

         AbstractObject o = session.findObjectByName("Integration Test node");
         if (o != null)
            session.deleteObject(o.getObjectId());

         o = session.findObjectByName("Integration Test Container");
         if (o != null)
            session.deleteObject(o.getObjectId());

         session.disconnect();
      }
   }

   @Test
   public void testNXSLDataCollectionFunctions() throws Exception
   {
      session = connectAndLogin();

      session.syncObjects();
      AbstractObject managementNode = TestHelper.findManagementServer(session);
      assertNotNull(managementNode);    

      String dciName = "Test.DCI" + Long.toString((new Date()).getTime());
      List<String> params = new ArrayList<String>();
      params.add(Long.toString(managementNode.getObjectId()));
      params.add(dciName);
      
      executeScript("/dataCollectionFunctions.nxsl", params);
      
      DataCollectionConfiguration config = session.openDataCollectionConfiguration(managementNode.getObjectId());
      for (DataCollectionObject dc : config.getItems())
      {
         if (dc.getName().equals(dciName))
         {
            config.deleteObject(dc.getId());
         }
      }
      config.close();
            
      session.disconnect();
   }
      
   @Test
   public void testNXSLAgentFunctions() throws Exception
   {
      session = connectAndLogin();      
      AbstractObject managementNode = TestHelper.findManagementServer(session);
      assertNotNull(managementNode);      

      String actionName = TestConstants.ACTION;
      String actionName2 = "testEcho";

      NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_TEMPLATE, "ScriptTestTemplate", 3);
      long templateId = session.createObject(cd);
      assertNotEquals(0, templateId);

      try
      {
         Thread.sleep(1000); // Object update should be received from server

         AbstractObject object = session.findObjectById(templateId);
         assertNotNull(object);

         AgentPolicy policy = new AgentPolicy("TestPolicy", AgentPolicy.AGENT_CONFIG);
         policy.setContent("Action = " + actionName2 + ": echo $1 $2 $3 $4");
         session.savePolicy(templateId, policy, false);

         session.applyTemplate(templateId, managementNode.getObjectId());
         session.executeAction(managementNode.getObjectId(), "Agent.Restart", null);

         Thread.sleep(10000); // Wait for agent restart         
         session.pollObject(managementNode.getObjectId(), ObjectPollType.STATUS, null);

         List<String> params = new ArrayList<String>();
         params.add(Long.toString(managementNode.getObjectId()));
         params.add(actionName);
         params.add(actionName2);
         executeScript("/agentFunctions.nxsl", params);
      }
      finally
      {
         session.deleteObject(templateId);
         session.disconnect();
      }
   }
   
   @Test
   public void testNXSLAlarmFunctions() throws Exception
   {
      session = connectAndLogin();
      
      HashMap<Long, Alarm> alarms = session.getAlarms();
      assertTrue(alarms.size() > 0);
      
      List<String> params = new ArrayList<String>();
      for(Alarm alarm : alarms.values())
      {
         String alarmKey = alarm.getKey();
         if (alarmKey.length() < 2)
            continue;
         params.add(Long.toString(alarm.getId()));
         params.add(alarmKey);
         // Escape special regexp characters
         params.add(".*" + alarmKey.substring(1).replace("(", "\\(").replace(")", "\\)").replace("[", "\\[").replace("]", "\\]"));
         break;
      }
      System.out.println("Check that at least one alarm was found");
      assertTrue(params.size() == 3);
      executeScript("/alarmFunctions.nxsl", params);
   }
   
   @Test
   public void testNXSLEventFunctions() throws Exception
   {
      session = connectAndLogin();
      AbstractObject managementNode = TestHelper.findManagementServer(session);
      assertNotNull(managementNode); 
      
      List<String> params = new ArrayList<String>();
      params.add(Long.toString(managementNode.getObjectId()));
      
      executeScript("/eventFunctions.nxsl", params);
   }
   
   @Test
   public void testNXSLMiscellaneousFunctions() throws Exception
   {
      session = connectAndLogin();
      List<String> params = new ArrayList<String>();

      //Find Object
      session.syncObjects();
      AbstractObject managementNode = TestHelper.findManagementServer(session);
      assertNotNull(managementNode);  
      
      //Create scheduled task
      Set<Long> objectSet = new HashSet<Long>();
      objectSet.add(managementNode.getObjectId());
      String scheduledTaskKey = "TestKey" + Long.toString((new Date()).getTime());
      ScheduledTask task = new ScheduledTask("Execute.Script", "", "testScript",
            "NXSLMiscelanious test comment", new Date(new Date().getTime() + 1200000), 0, managementNode.getObjectId());
      task.setKey(scheduledTaskKey);
      session.addScheduledTask(task);

      //Create user support application notification
      session.createUserAgentNotification("Message", objectSet, new Date(), new Date(), true);      
      List<UserAgentNotification> notificaitons = session.getUserAgentNotifications();
      notificaitons.sort(new Comparator<UserAgentNotification>() {

         @Override
         public int compare(UserAgentNotification o1, UserAgentNotification o2)
         {
            return Long.compare(o2.getId(), o1.getId());
         }
      });
      long nextNotificationId = notificaitons.get(0).getId();
      
      //Create map for tests
      String mappingTableName = "TestMap" + Long.toString((new Date()).getTime());
      int id = session.createMappingTable(mappingTableName, "NXSLMiscelanious test map description", 0);
      final MappingTable mappingTable = session.getMappingTable(id);
      String mappingTableKey1 = "TestMapKey1" + Long.toString((new Date()).getTime());
      String mappingTableValue1 = "TestMapKeyValue1";
      MappingTableEntry e = new MappingTableEntry(mappingTableKey1, mappingTableValue1, "NXSLMiscelanious test"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
      mappingTable.getData().add(e);      
      String mappingTableKey2 = "TestMapKey2" + Long.toString((new Date()).getTime());
      String mappingTableValue2 = "TestMapKeyValue2";
      MappingTableEntry e2 = new MappingTableEntry(mappingTableKey2, mappingTableValue2, "NXSLMiscelanious test"); //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
      mappingTable.getData().add(e2);
      session.updateMappingTable(mappingTable);      

      params.add(Long.toString(managementNode.getObjectId()));
      params.add(scheduledTaskKey);
      params.add(Long.toString(nextNotificationId));
      params.add(mappingTableName);
      params.add(mappingTableKey1);
      params.add(mappingTableValue1);
      params.add(mappingTableKey2);
      params.add(mappingTableValue2);
      
      try
      {
         executeScript("/miscellaneousFunctions.nxsl", params);

         //Check if new notification was created by nxsl
         notificaitons = session.getUserAgentNotifications();
         notificaitons.sort(new Comparator<UserAgentNotification>() {
            @Override
            public int compare(UserAgentNotification o1, UserAgentNotification o2)
            {
               return Long.compare(o2.getId(), o1.getId());
            }
         });

         assertEquals(nextNotificationId + 1, notificaitons.get(0).getId());    
      }
      finally
      {
         session.deleteMappingTable(id);
      }
   }
   
   @Test
   public void testNXSLWebService() throws Exception
   {
      session = connectAndLogin();
      AbstractObject managementNode = TestHelper.findManagementServer(session);
      assertNotNull(managementNode); 
      String webServiceId = "neste";
      String webServiceUrl = "https://www.neste.lv/lv/content/degvielas-cenas";
      
      List<WebServiceDefinition> definition = session.getWebServiceDefinitions();
      boolean found = false;
      for (WebServiceDefinition ws : definition)
      {
         if (ws.getName().equals(webServiceId))
         {
            found = true;
            break;
         }
      }
      if (!found)
      {
         WebServiceDefinition ws = new WebServiceDefinition(webServiceId);
         ws.setUrl(webServiceUrl);
         session.modifyWebServiceDefinition(ws);
      }
      
      List<String> params = new ArrayList<String>();
      params.add(Long.toString(managementNode.getObjectId()));
      params.add(webServiceId);
      
      executeScript("/webService.nxsl", params);
   }

   /**
    * @see org.netxms.client.ActionExecutionListener#messageReceived(java.lang.String)
    */
   @Override
   public void messageReceived(final String text)
   {
      System.out.println("[" + currentScriptName + "] " + text);
   }

   @Override
   public void setStreamId(long streamId)
   {
   }

   @Override
   public void onFailure(Exception exception)
   {
      System.out.println("[" + currentScriptName + "] EXECUTION ERROR");
      scriptFailed = true;
   }

   @Override
   public void onSuccess()
   {
   }
}
