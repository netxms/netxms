/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Raden Solutions
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
package org.netxms.utilities;

import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.PollState;
import org.netxms.client.ProtocolVersion;
import org.netxms.client.Script;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.client.datacollection.DciSummaryTableColumn;
import org.netxms.client.datacollection.DciSummaryTableDescriptor;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.Node;
import org.netxms.client.objects.configs.CustomAttribute;
import org.netxms.client.users.User;
import org.netxms.tests.TestConstants;

public class TestHelper
{
   /**
    * Create NXCSession instance, connect and login
    * 
    * @param useEncryption
    * @throws Exception
    */
   public static NXCSession connect() throws Exception
   {
      NXCSession session = new NXCSession(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT, true);
      session.setRecvBufferSize(65536, 33554432);
      session.connect(new int[] { ProtocolVersion.INDEX_FULL });
      session.login(TestConstants.SERVER_LOGIN, TestConstants.SERVER_PASSWORD);
      return session;
   }

   /**
    * Find management node
    * 
    * @param session NXCSession object
    * @return management node
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException 
    */
   public static Node findManagementServer(NXCSession session) throws IOException, NXCException, InterruptedException
   {
      session.syncObjects();
      for(AbstractObject object : session.getAllObjects())
      {
         if ((object instanceof Node) && ((Node)object).isManagementServer())
         {
            if (((Node)object).getAgentPort() != TestConstants.AGENT_PORT)
            {
               NXCObjectModificationData data = new NXCObjectModificationData(object.getObjectId());
               data.setAgentPort(TestConstants.AGENT_PORT);
               session.modifyObject(data);

               session.pollObject(object.getObjectId(), ObjectPollType.CONFIGURATION_NORMAL, null);
               Thread.sleep(5000);               
            }
            return (Node)object;
         }
      }
      return null;
   }

   /**
    * Get test node with topology
    * 
    * @param session NXCSession object
    * @return test node which is used for methods: testLinkLayerTopology, testRoutingTable, testSwitchForwardingTable
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static Node getTopologyNode(NXCSession session) throws IOException, NXCException, InterruptedException
   {
      session.syncObjects();
      checkObjectCreated(session);
      return (Node)session.findObject((object) -> (object instanceof Node) && ((Node)object).getPrimaryIP().getHostAddress().equals(TestConstants.TEST_NODE_2));
   }

   /**
    * Checks test node is created, if not create test node
    * 
    * @param session
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static void checkObjectCreated(NXCSession session) throws IOException, NXCException, InterruptedException
   {
      String[] nodes = { TestConstants.TEST_NODE_1, TestConstants.TEST_NODE_2, TestConstants.TEST_NODE_3 };
      // checking if the object is created, and if it's not, create it
      for(String nodeIP : nodes)
      {
         boolean found = false;

         for(AbstractObject object : session.getAllObjects())
         {
            if ((object instanceof Node) && (((Node)object).getPrimaryIP().getHostAddress().equals(nodeIP)))
            {
               found = true;
               break;
            }
         }

         if (!found)
         {
            NXCObjectCreationData cd = new NXCObjectCreationData(GenericObject.OBJECT_NODE, nodeIP, GenericObject.SERVICEROOT);
            cd.setPrimaryName(nodeIP);
            session.createObjectSync(cd);

         }

      }
      // checking that the topology poll passed for all nodes
      for(String nodeIP : nodes)
      {
         boolean pending = true;
         for(AbstractObject object : session.getAllObjects())
         {
            if ((object instanceof Node) && (((Node)object).getPrimaryIP().getHostAddress().equals(nodeIP)))
            {
               do
               {
                  Thread.sleep(1000);
                  Node findedNode = (Node)session.findObjectById(object.getObjectId());
                  for(PollState status : findedNode.getPollStates())
                  {
                     if ("topology".equals(status.getPollType()))
                     {
                        Date lastCompleted = status.getLastCompleted();
                        pending = (lastCompleted.getTime() == 0) || status.isPending();
                     }

                  }
               } while(pending);
               break;
            }
         }
      }
   }

   /**
    * Checks if a container with the given name exists; if it doesn't exist, creates one.
    * 
    * @param session
    * @param containerName
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static void createContainer(NXCSession session, String containerName) throws IOException, NXCException, InterruptedException
   {
      boolean containerFound = false;

      for(AbstractObject object : session.getAllObjects())
      {
         if (object instanceof Container && ((Container)object).getObjectName().equals(containerName))
         {
            containerFound = true;
            break;
         }
      }
      if (!containerFound)
      {
         NXCObjectCreationData cd = new NXCObjectCreationData(GenericObject.OBJECT_CONTAINER, containerName, GenericObject.SERVICEROOT);
         session.createObjectSync(cd);
      }
   }

   /**
    * Checks if any of the test containers exist, and if so, deletes them. This method is necessary to run at the beginning of each
    * test so that if one test fails, the others can still run.
    * 
    * @param session
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static void findAndDeleteContainer(NXCSession session, String[] containers) throws IOException, NXCException, InterruptedException
   {
      session.syncObjects();
      for(String container : containers)
      {
         for(AbstractObject object : session.getAllObjects())
         {
            if (object instanceof Container && ((Container)object).getObjectName().equals(container))
            {
               System.out.println(container);
               AbstractObject foundContainer = session.findObjectByName(container);
               session.deleteObject(foundContainer.getObjectId());
            }
         }
      }
   }

   /**
    * Checks if a node with the given name exists; if it doesn't exist, creates one.
    * 
    * @param session
    * @param objectName
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static void checkNodeCreatedOrCreate(NXCSession session, String objectName) throws IOException, NXCException, InterruptedException
   {
      boolean objectFound = false;
      for(AbstractObject object : session.getAllObjects())
      {
         if (object instanceof Node && (object.getObjectName().equals(objectName)))
         {
            objectFound = true;
            break;
         }
      }
      if (!objectFound)
      {
         NXCObjectCreationData cd = new NXCObjectCreationData(GenericObject.OBJECT_NODE, objectName, GenericObject.SERVICEROOT);
         cd.setPrimaryName(objectName);
         session.createObjectSync(cd);
      }
   }

   /**
    * Creates or modifies custom attributes for the object.
    * 
    * @param session
    * @param caName
    * @param caValue
    * @param flag
    * @param objectId
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static void changeCustomAttributes(NXCSession session, String caName, String caValue, int flag, long objectId) throws IOException, NXCException, InterruptedException
   {

      Map<String, CustomAttribute> customAttributes = new HashMap<>();
      if (caName != null && caValue != null)
      {
         CustomAttribute customAttribute = new CustomAttribute(caValue, flag, 0);
         customAttributes.put(caName, customAttribute);
      }

      NXCObjectModificationData data = new NXCObjectModificationData(objectId);
      data.setCustomAttributes(customAttributes);
      session.modifyObject(data);
   }

   /**
    * Find the name of the script in the list or created based on the specified parameters.
    * 
    * @param session
    * @param scriptName
    * @param scriptSource
    * @return scriptName
    * @throws Exception
    */
   public static String findOrCreateScript(NXCSession session, String scriptName, String scriptSource) throws Exception
   {
      List<Script> scriptsList = session.getScriptLibrary();
      for(Script script : scriptsList)
      {
         if (script.getName().equals(scriptName))
         {
            return scriptName;
         }
      }
      session.modifyScript(0, scriptName, scriptSource);
      return scriptName;
   }
   
   /**
    * Checks if any test DCI exist, and if so, deletes them. This method is necessary to run at the beginning of each
    * test so that if one test fails, the others can still run.
    * @param session
    * @param collector
    * @param description
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static void findAndDeleteDci(NXCSession session, AbstractObject collector, String description) throws IOException, NXCException, InterruptedException
   {
      DataCollectionConfiguration dcc = session.openDataCollectionConfiguration(collector.getObjectId());
      for(DataCollectionObject value : dcc.getItems())
      {
         if (value.getDescription().equals(description) || value.getDescription().equals(description + "{instance}"))
         {
            dcc.deleteObject(value.getId());
         }
      }
   }
   
   /**
    * Checks if a summary table with the given title exists; if it doesn't exist, creates one.
    * @param session
    * @return summary table id
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static int findOrCreateSumTable(NXCSession session) throws IOException, NXCException, InterruptedException
   {
      DciSummaryTableDescriptor desc = null;
      for(DciSummaryTableDescriptor item : session.listDciSummaryTables())
      {
         if (item.getTitle().equals("summary name for test"))
         {
            desc = item;
            return desc.getId();
         }
      }

      DciSummaryTable table = new DciSummaryTable("summary name for test", "");
      DciSummaryTableColumn column = new DciSummaryTableColumn("Dummy", "Dummy", 0, ";");
      List<DciSummaryTableColumn> columns = new ArrayList<DciSummaryTableColumn>();
      columns.add(column);
      table.getColumns().addAll(columns);
      return session.modifyDciSummaryTable(table);
   }

   /**
    * Find or create user
    *
    * @param session
    * @param name
    * @param password
    * @return
    * @throws Exception
    */
   public static User findOrCreateUser(NXCSession session, String name, String password) throws Exception
   {
      session.syncUserDatabase();
      User user = (User)session.findUserDBObjectByName(name, User.class);
      if (user == null)
      {
         int uid = session.createUser(name);
         session.setUserPassword(uid, password, null);
         session.syncMissingUsers(Arrays.asList(new Integer[] { uid }));
         user = (User)session.findUserDBObjectById(uid, null);
      }
      return user;
   }

   public static long findOrCreateDCI(NXCSession session, AbstractObject node, String description) throws IOException, NXCException
   {
      DciValue[] list = session.getLastValues(node.getObjectId());

      for(int i = 0; i < list.length; i++)
      {
         if (list[i].getDescription().equalsIgnoreCase(description))
            return list[i].getId();
      }
      
      DataCollectionConfiguration dcc;
      dcc = session.openDataCollectionConfiguration(node.getObjectId());      

      final DataCollectionItem dci = new DataCollectionItem(dcc, 0);

      dci.setOrigin(DataOrigin.INTERNAL);
      dci.setDescription(description);
      dci.setName("Status");

      long id = dcc.modifyObject(dci);
      dcc.close();
      return id;
   }
}
