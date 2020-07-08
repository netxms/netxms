/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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

import java.net.InetAddress;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;
import org.apache.commons.io.IOUtils;
import org.netxms.base.InetAddressEx;
import org.netxms.base.MacAddress;
import org.netxms.client.AgentPolicy;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.ScriptCompilationResult;
import org.netxms.client.TextOutputListener;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;

/**
 * Tests for scripting functions
 */
public class ScriptTest extends AbstractSessionTest implements TextOutputListener
{
   private NXCSession session;
   private Boolean scriptFailed;
   
   public void testAddressMap() throws Exception
   {
      final NXCSession session = connect();

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
      System.out.println("Compilation error message: \"" + r.errorMessage + "\"");
      
      session.disconnect();
   }
   
   private void executeScript(String script, List <String> params) throws Exception
   {
      ScriptCompilationResult r = session.compileScript(script, true);
      if (r.errorMessage != null)
         System.out.println("Compilation error message: \"" + r.errorMessage + "\"");
      assertTrue(r.success);
      assertNotNull(r.code);
      assertNull(r.errorMessage);
      
      scriptFailed = false;
      session.executeScript(2, script, params, ScriptTest.this);
      assertFalse(scriptFailed);         
   }
   
   public void testNXSLObjectFunctions() throws Exception
   {
      session = connect();
      String script = IOUtils.toString(this.getClass().getResourceAsStream("/objectFunctions.nxsl"), "UTF-8");      

      session.syncObjects();
      AbstractObject object = session.findObjectById(2);
      assertNotNull(object);
      
      NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_NODE, "TestNode", 2);
      cd.setCreationFlags(NXCObjectCreationData.CF_CREATE_UNMANAGED);
      cd.setIpAddress(new InetAddressEx(InetAddress.getByName("192.168.10.1"), 0));
      long nodeId = session.createObject(cd);
      assertFalse(nodeId == 0);
      
      object = session.findObjectById(3);
      assertNotNull(object);      

      cd = new NXCObjectCreationData(AbstractObject.OBJECT_TEMPLATE, "TestTemplate", 3);
      long templateId = session.createObject(cd);
      assertFalse(templateId == 0);

      Thread.sleep(1000);  // Object update should be received from server

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
      executeScript(script, params);
      
      session.deleteObject(nodeId);
      session.deleteObject(templateId);      
      session.disconnect();
   }
   
   public void testNXSLDataCollectionFunctions() throws Exception
   {
      session = connect();
      String script = IOUtils.toString(this.getClass().getResourceAsStream("/dataCollectionFunctions.nxsl"), "UTF-8");      

      session.syncObjects();
      List<AbstractObject> objects = session.getAllObjects(); //Find managment node
      AbstractObject managementNode = null;
      for (AbstractObject obj : objects)
      {
         if (obj instanceof Node && ((Node)obj).isManagementServer())
         {
            managementNode = obj;
            break;            
         }
      }
      assertNotNull(managementNode);      

      String dciName = "Test.DCI";
      List<String> params = new ArrayList<String>();
      params.add(Long.toString(managementNode.getObjectId()));
      params.add(dciName);
      executeScript(script, params);
      
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
      
   public void testNXSLAgentFunctions() throws Exception
   {
      session = connect();
      String script = IOUtils.toString(this.getClass().getResourceAsStream("/agentFunctions.nxsl"), "UTF-8");      

      session.syncObjects();
      List<AbstractObject> objects = session.getAllObjects(); //Find managment node
      AbstractObject managementNode = null;
      for (AbstractObject obj : objects)
      {
         if (obj instanceof Node && ((Node)obj).isManagementServer())
         {
            managementNode = obj;
            break;            
         }
      }
      assertNotNull(managementNode);      

      String actionName = TestConstants.ACTION;
      String actionName2 = "testEcho";

      NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_TEMPLATE, "TestTemplate", 3);
      long templateId = session.createObject(cd);
      assertFalse(templateId == 0);

      Thread.sleep(1000);  // Object update should be received from server

      AbstractObject object = session.findObjectById(templateId);
      assertNotNull(object); 

      AgentPolicy policy = new AgentPolicy("TestPolicy", AgentPolicy.AGENT_CONFIG);
      policy.setContent("Action = " + actionName2 + ": echo $1 $2 $3 $4");
      session.savePolicy(templateId, policy, false);

      session.applyTemplate(templateId, managementNode.getObjectId());    
      session.executeActionWithExpansion(managementNode.getObjectId(), 0, "Agent.Restart", false, null, null, null);

      Thread.sleep(10000);  // Wait for agent restart
      
      List<String> params = new ArrayList<String>();
      params.add(Long.toString(managementNode.getObjectId()));
      params.add(actionName);
      params.add(actionName2);
      executeScript(script, params);
      
      session.deleteObject(templateId);
      
      session.disconnect();
   }

   /* (non-Javadoc)
    * @see org.netxms.client.ActionExecutionListener#messageReceived(java.lang.String)
    */
   @Override
   public void messageReceived(final String text)
   {
      System.out.println(text);
   }

   @Override
   public void setStreamId(long streamId)
   {
   }

   @Override
   public void onError()
   {
      System.out.println("Script error occured");
      scriptFailed = true;
   }
}
