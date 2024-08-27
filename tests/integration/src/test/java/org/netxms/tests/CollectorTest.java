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
import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.io.IOException;
import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCSession;
import org.netxms.client.PollState;
import org.netxms.client.Table;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.PerfTabDci;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.events.Alarm;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.GenericObject;
import org.netxms.utilities.TestHelper;

public class CollectorTest extends AbstractSessionTest
{
   final static String COLLECTOR_NAME = "Test-collector";
   final String testCollectorScript = "return %(1,2,3);";
   final String scriptNameForSearching = "InstanceScript";
   final String DUMMY = "Dummy";
   final String DCI_DESCRIPTION = "test DCI for Collector";
   final String TRANSORMATION_SCRIPT = "return 6;";

   /**
    * Checks if a collector with specified name exists. If it doesn't exist, creates a new one.
    * 
    * @param session
    * @param collectorName
    * @throws IOException
    * @throws NXCException
    * @throws InterruptedException
    */
   public static Collector findOrCreateCollector(NXCSession session, String collectorName) throws IOException, NXCException, InterruptedException
   {
      Collector collector = null;

      for(AbstractObject object : session.getAllObjects())
      {
         if (object instanceof Collector && ((Collector)object).getObjectName().equals(collectorName))
         {
            collector = (Collector)object;
            break;
         }
      }
      if (collector == null)
      {
         NXCObjectCreationData testCollector = new NXCObjectCreationData(GenericObject.OBJECT_COLLECTOR, collectorName, GenericObject.SERVICEROOT);
         session.createObjectSync(testCollector);
         collector = (Collector)session.findObjectByName(collectorName);
      }
      return collector;
   }

   @Test
   public void testGetLastValues() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      Collector testCollector = findOrCreateCollector(session, COLLECTOR_NAME);
      assertNotNull(testCollector);

      TestHelper.findAndDeleteDci(session, testCollector, DCI_DESCRIPTION);

      DataCollectionConfiguration dcc = session.openDataCollectionConfiguration(testCollector.getObjectId());

      final DataCollectionItem dci = new DataCollectionItem(dcc, 0);

      dci.setOrigin(DataOrigin.INTERNAL);
      dci.setDescription(DCI_DESCRIPTION);
      dci.setName(DUMMY);
      dci.setPollingScheduleType(DataCollectionObject.POLLING_SCHEDULE_CUSTOM);
      dci.setPollingInterval(Integer.toString(1));
      dci.setTransformationScript(TRANSORMATION_SCRIPT);

      dcc.modifyObject(dci);
      Thread.sleep(1000);

      DciValue[] list = session.getLastValues(testCollector.getObjectId());

      assertTrue(list.length > 0); // Checking that DCI has been created

      dcc.close();
      session.disconnect();
   }

   @Test
   public void testThreshold() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      Collector testCollector = findOrCreateCollector(session, COLLECTOR_NAME);
      assertNotNull(testCollector);

      TestHelper.findAndDeleteDci(session, testCollector, DCI_DESCRIPTION);

      DataCollectionConfiguration dcc = session.openDataCollectionConfiguration(testCollector.getObjectId());

      final DataCollectionItem dci = new DataCollectionItem(dcc, 0);

      dci.setOrigin(DataOrigin.INTERNAL);
      dci.setDescription(DCI_DESCRIPTION);
      dci.setName(DUMMY);
      dci.setTransformationScript(TRANSORMATION_SCRIPT);

      Threshold threshold = new Threshold();
      threshold.setValue("10");
      threshold.setOperation(Threshold.OP_LE);
      threshold.setFunction(Threshold.F_LAST);
      dci.getThresholds().add(threshold);
      dcc.modifyObject(dci);

      Thread.sleep(2000);

      DciValue[] list = session.getLastValues(testCollector.getObjectId());
      testCollector = findOrCreateCollector(session, COLLECTOR_NAME);

      boolean findThreshold = false;
      for(DciValue dciValue : list)
      {
         if (dciValue.getActiveThreshold() != null && testCollector.getStatus() != (ObjectStatus.NORMAL))
            findThreshold = true;
      }
      assertTrue(findThreshold);

      Alarm testAlarm = null;
      HashMap<Long, Alarm> alarms = session.getAlarms();
      for(Map.Entry<Long, Alarm> entry : alarms.entrySet())
      {
         Alarm alarm = entry.getValue();
         if (alarm.getSourceObjectId() == testCollector.getObjectId() && alarm.getCurrentSeverity() != Severity.NORMAL)
         {
            testAlarm = alarm;
            break;
         }
      }
      assertNotNull(testAlarm);

      dcc.close();
      session.disconnect();
   }

   @Test
   public void testGetPerfTabItems() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      Collector testCollector = findOrCreateCollector(session, COLLECTOR_NAME);
      assertNotNull(testCollector);

      TestHelper.findAndDeleteDci(session, testCollector, DCI_DESCRIPTION);

      DataCollectionConfiguration dcc = session.openDataCollectionConfiguration(testCollector.getObjectId());

      final DataCollectionItem dci = new DataCollectionItem(dcc, 0);

      dci.setOrigin(DataOrigin.INTERNAL);
      dci.setDescription(DCI_DESCRIPTION);
      dci.setName(DUMMY);
      dci.setTransformationScript(TRANSORMATION_SCRIPT);
      dci.setPerfTabSettings("<config><enabled>true</enabled></config>");

      dcc.modifyObject(dci);
      Thread.sleep(1000);

      List<PerfTabDci> perfTabList = session.getPerfTabItems(testCollector.getObjectId());
      assertNotNull(perfTabList);
      assertFalse(perfTabList.isEmpty());

      dcc.close();
      session.disconnect();
   }

   @Test
   public void testInstanceDiscovery() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      Collector testCollector = findOrCreateCollector(session, COLLECTOR_NAME);
      assertNotNull(testCollector);

      TestHelper.findAndDeleteDci(session, testCollector, DCI_DESCRIPTION);

      DataCollectionConfiguration dcc = session.openDataCollectionConfiguration(testCollector.getObjectId());

      final DataCollectionItem dci = new DataCollectionItem(dcc, 0);

      dci.setOrigin(DataOrigin.INTERNAL);
      dci.setDescription(DCI_DESCRIPTION + "{instance}");
      dci.setName(DUMMY);
      dci.setTransformationScript(TRANSORMATION_SCRIPT);

      String scriptName = TestHelper.findOrCreateScript(session, scriptNameForSearching, testCollectorScript);
      dci.setInstanceDiscoveryMethod(DataCollectionObject.IDM_SCRIPT);
      dci.setInstanceDiscoveryData(scriptName);

      long id = dcc.modifyObject(dci);

      session.pollObject(testCollector.getObjectId(), ObjectPollType.INSTANCE_DISCOVERY, null);
      Thread.sleep(1000);

      DciValue[] list = session.getLastValues(testCollector.getObjectId());
      testCollector = findOrCreateCollector(session, COLLECTOR_NAME);

      int count = 0;
      for(DciValue value : list)
      {
         if (value.getDescription().equals(DCI_DESCRIPTION + "1") || value.getDescription().equals(DCI_DESCRIPTION + "2") || value.getDescription().equals(DCI_DESCRIPTION + "3"))
         {
            count++;
         }
      }
      assertTrue(count == 3); // because script makes 3 instances

      for(DciValue value : list)
      {
         assertEquals(id, value.getTemplateDciId()); // checking that instances are created from the test DCI
      }

      for(PollState status : testCollector.getPollStates())
      {
         if ("instance".equals(status.getPollType()))
         {
            Date lastCompleted = status.getLastCompleted();
            Date currentDate = new Date();
            long fiveMinutesAgoMillis = currentDate.getTime() - (5 * 60 * 1000);
            Date fiveMinutesAgoDate = new Date(fiveMinutesAgoMillis);
            assertTrue(lastCompleted.after(fiveMinutesAgoDate)); // сhecking that the DCI instance discovery status has changed
         }
      }
      dcc.close();
      session.disconnect();
   }

   @Test
   public void testGetSumTab() throws Exception
   {
      final NXCSession session = connectAndLogin();
      session.syncObjects();

      Collector testCollector = findOrCreateCollector(session, COLLECTOR_NAME);
      assertNotNull(testCollector);

      TestHelper.findAndDeleteDci(session, testCollector, DCI_DESCRIPTION);

      DataCollectionConfiguration dcc = session.openDataCollectionConfiguration(testCollector.getObjectId());

      final DataCollectionItem dci = new DataCollectionItem(dcc, 0);

      dci.setOrigin(DataOrigin.INTERNAL);
      dci.setDescription(DCI_DESCRIPTION);
      dci.setName(DUMMY);
      dci.setPollingScheduleType(DataCollectionObject.POLLING_SCHEDULE_CUSTOM);
      dci.setPollingInterval(Integer.toString(1));
      dci.setTransformationScript(TRANSORMATION_SCRIPT);
      dcc.modifyObject(dci);
      
      Thread.sleep(1000);

      DciValue[] list = session.getLastValues(testCollector.getObjectId());
      assertTrue(list.length > 0);

      long parentId = 0;
      long[] parentIdList = testCollector.getParentIdList();
      for(long idP : parentIdList)
      {
         if (parentIdList.length > 0)
         {
            parentId = idP;
         }
      }

      int sumTableId = TestHelper.findOrCreateSumTable(session);
      Table queryDciSumTable = session.queryDciSummaryTable(sumTableId, parentId);
      boolean found = false;
      for(int i = 0; i < queryDciSumTable.getRowCount(); i++)
      {
         if (queryDciSumTable.getCellValue(i, 0).equals(testCollector.getObjectName()))
         {
            assertEquals("6", queryDciSumTable.getCellValue(i, 1));
            found = true;
            break;
         }
      }
      assertTrue(found);

      dcc.close();
      session.disconnect();
   }
}
