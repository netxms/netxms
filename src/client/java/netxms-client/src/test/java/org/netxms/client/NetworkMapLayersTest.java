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
package org.netxms.client;

import static org.junit.jupiter.api.Assertions.assertEquals;
import static org.junit.jupiter.api.Assertions.assertTrue;
import java.util.ArrayList;
import java.util.Arrays;
import org.junit.jupiter.api.Test;
import org.netxms.base.NXCPMessage;
import org.netxms.client.maps.NetworkMapPage;
import org.netxms.client.maps.elements.NetworkMapDecoration;
import org.netxms.client.maps.elements.NetworkMapElement;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.maps.elements.NetworkMapTextBox;

/**
 * Tests for network map element layers
 */
public class NetworkMapLayersTest
{
   @Test
   public void testEmptyPage()
   {
      assertTrue(new NetworkMapPage("test").getLayers().isEmpty());
   }

   @Test
   public void testElementsWithoutLayer()
   {
      NetworkMapPage page = new NetworkMapPage("test");
      page.addElement(new NetworkMapObject(1, 100));
      page.addElement(new NetworkMapDecoration(2, NetworkMapDecoration.GROUP_BOX));
      assertTrue(page.getLayers().isEmpty());
   }

   @Test
   public void testDistinctSortedLayers()
   {
      NetworkMapPage page = new NetworkMapPage("test");
      NetworkMapElement e = new NetworkMapObject(1, 100);
      e.setLayer("Temperatures");
      page.addElement(e);
      e = new NetworkMapObject(2, 101);
      e.setLayer("Power");
      page.addElement(e);
      e = new NetworkMapTextBox(3);
      e.setLayer("Temperatures");
      page.addElement(e);
      page.addElement(new NetworkMapObject(4, 102));
      assertEquals(new ArrayList<String>(page.getLayers()), Arrays.asList("Power", "Temperatures"));
   }

   @Test
   public void testSetLayerNormalization()
   {
      NetworkMapElement e = new NetworkMapObject(1, 100);
      e.setLayer("  Power ");
      assertEquals("Power", e.getLayer());
      e.setLayer(null);
      assertEquals("", e.getLayer());
   }

   @Test
   public void testMessageRoundTrip()
   {
      NetworkMapElement e = new NetworkMapObject(1, 100);
      e.setLayer("Power");
      NXCPMessage msg = new NXCPMessage(0);
      e.fillMessage(msg, 1000);
      assertEquals("Power", NetworkMapElement.createMapElement(msg, 1000).getLayer());

      msg = new NXCPMessage(0);
      new NetworkMapObject(1, 100).fillMessage(msg, 1000);
      assertEquals("", NetworkMapElement.createMapElement(msg, 1000).getLayer());
   }
}
