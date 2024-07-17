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

import static org.junit.jupiter.api.Assertions.assertNotEquals;
import static org.junit.jupiter.api.Assertions.assertNotNull;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.Date;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.Table;
import org.netxms.client.constants.DataOrigin;
import org.netxms.client.datacollection.DataCollectionConfiguration;
import org.netxms.client.datacollection.DataCollectionItem;
import org.netxms.client.datacollection.DataCollectionObject;
import org.netxms.client.datacollection.DataCollectionTable;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Node;
import org.netxms.utilities.TestHelper;
/**
 * Test for creating data collection.
 */
public class AgentDataCollectionTest extends AbstractSessionTest
{
   @Test
   public void testGetLastValues() throws Exception
   {
      final NXCSession session = connectAndLogin();
      Node testNode = TestHelper.findManagementServer(session);
      
      assertNotNull(testNode);
      assertTrue(testNode.hasAgent());

      DataCollectionConfiguration dcc = session.openDataCollectionConfiguration(testNode.getObjectId());
      final DataCollectionItem dci = new DataCollectionItem(dcc, 0);
      dci.setOrigin(DataOrigin.AGENT);
      dci.setDescription("Test agent id");
      dci.setName("agent.ID");
      dci.setPollingScheduleType(DataCollectionObject.POLLING_SCHEDULE_CUSTOM);
      dci.setPollingInterval(Integer.toString(1));
      long id = dcc.modifyObject(dci);
      Thread.sleep(1000);

      DciValue testDCiValue = null;  
      DciValue[]list = session.getLastValues(testNode.getObjectId());
      for(DciValue value : list)
      {
         if (value.getId() == id)
         {
            testDCiValue = value;
         }
      }      
      Date currentDate = new Date();
      long oneMinuteAgoMillis = currentDate.getTime() - (1 * 60 * 1000);
      Date oneMinuteAgoDate = new Date(oneMinuteAgoMillis);
      assertTrue(testDCiValue.getTimestamp().after(oneMinuteAgoDate)); 
      assertNotEquals(testDCiValue.getValue(), ""); 
      
      final DataCollectionTable dct = new DataCollectionTable(dcc, 0);
      dct.setOrigin(DataOrigin.AGENT);
      dct.setDescription("Test agent table");
      dct.setName("Net.Interfaces");
      dct.setPollingScheduleType(DataCollectionObject.POLLING_SCHEDULE_CUSTOM);
      dct.setPollingInterval(Integer.toString(1));
      long dctId = dcc.modifyObject(dct);
      Thread.sleep(2000);

      Table testDct = session.getTableLastValues(testNode.getObjectId(), dctId);

      AbstractObject[] childrens = testNode.getChildrenAsArray();
      int rowCount = testDct.getRowCount();
      Boolean[] foundItems = new Boolean[rowCount];
      for(AbstractObject child : childrens)
      {
         String childName = child.getObjectName();
         for(int i = 0; i < rowCount; i++)
         {
            if (testDct.getCellValue(i, 1).equals(childName))
            {
               foundItems[i] = true;
               break;
            }
         }
      }
      
      for (Boolean f : foundItems)
         assertTrue(f);
      
      dcc.deleteObject(id);
      dcc.deleteObject(dctId);
      dcc.close();
      session.disconnect();
   }
}
