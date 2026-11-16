/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
import static org.junit.jupiter.api.Assertions.assertNotNull;
import java.util.ArrayList;
import java.util.Arrays;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.maps.MapType;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapDCIContainer;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.maps.elements.NetworkMapTextBox;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.client.objects.NetworkMap;
import org.netxms.utilities.TestHelper;

/**
 * Tests for network map element layers
 */
public class NetworkMapLayerTest extends AbstractSessionTest
{
   private static final String TEST_MAP_NAME = "LayerTest";

   @Test
   public void testLayerRoundTrip() throws Exception
   {
      NXCSession session = connectAndLogin();
      session.syncObjects();

      AbstractObject oldMap = session.findObjectByName(TEST_MAP_NAME);
      if (oldMap != null)
         session.deleteObject(oldMap.getObjectId());

      AbstractObject managementNode = TestHelper.findManagementServer(session);
      assertNotNull(managementNode);

      NXCObjectCreationData cd = new NXCObjectCreationData(GenericObject.OBJECT_NETWORKMAP, TEST_MAP_NAME, GenericObject.NETWORKMAPROOT);
      cd.setMapType(MapType.CUSTOM);
      NetworkMapPage page = ((NetworkMap)session.createObjectSync(cd)).createMapPage();
      long mapId = page.getMapObjectId();

      NetworkMapElement object = new NetworkMapObject(page.createElementId(), managementNode.getObjectId());
      object.setLayer("Status");
      page.addElement(object);
      NetworkMapElement groupBox = new NetworkMapDecoration(page.createElementId(), NetworkMapDecoration.GROUP_BOX);
      groupBox.setLayer("Rooms");
      page.addElement(groupBox);
      NetworkMapElement container = new NetworkMapDCIContainer(page.createElementId());
      container.setLayer("Temperatures");
      page.addElement(container);
      NetworkMapElement textBox = new NetworkMapTextBox(page.createElementId());
      page.addElement(textBox);

      NXCObjectModificationData md = new NXCObjectModificationData(mapId);
      md.setMapContent(page.getElements(), page.getLinks());
      session.modifyObject(md);
      session.disconnect();

      session = connectAndLogin();
      session.syncObjects();
      page = ((NetworkMap)session.findObjectById(mapId)).createMapPage();
      assertEquals("Status", page.getElement(object.getId(), null).getLayer());
      assertEquals("Rooms", page.getElement(groupBox.getId(), null).getLayer());
      assertEquals("Temperatures", page.getElement(container.getId(), null).getLayer());
      assertEquals("", page.getElement(textBox.getId(), null).getLayer());
      assertEquals(Arrays.asList("Rooms", "Status", "Temperatures"), new ArrayList<String>(page.getLayers()));

      page.getElement(groupBox.getId(), null).setLayer("");
      md = new NXCObjectModificationData(mapId);
      md.setMapContent(page.getElements(), page.getLinks());
      session.modifyObject(md);
      session.disconnect();

      session = connectAndLogin();
      session.syncObjects();
      page = ((NetworkMap)session.findObjectById(mapId)).createMapPage();
      assertEquals("", page.getElement(groupBox.getId(), null).getLayer());
      assertEquals(Arrays.asList("Status", "Temperatures"), new ArrayList<String>(page.getLayers()));

      session.deleteObject(mapId);
      session.disconnect();
   }
}
