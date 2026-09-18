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
import org.junit.jupiter.api.Test;
import org.netxms.client.maps.LinkDataDirection;
import org.netxms.client.maps.configs.LinkConfig;
import org.netxms.client.maps.configs.MapLinkDataSource;
import com.google.gson.Gson;

/**
 * Tests for link data source configuration
 */
public class MapLinkDataSourceTest
{
   private static final String CONFIG = "{\"dciList\":[" +
         "{\"nodeId\":12,\"dciId\":345,\"type\":1,\"location\":\"CENTER\",\"direction\":\"FORWARD\",\"system\":true,\"formatString\":\"%{u,m}s\"}," +
         "{\"nodeId\":12,\"dciId\":346,\"type\":1,\"location\":\"CENTER\",\"direction\":\"REVERSE\",\"system\":true,\"formatString\":\"%{u,m}s\"}," +
         "{\"nodeId\":12,\"dciId\":347,\"type\":1,\"location\":\"OBJECT1\",\"system\":true,\"formatString\":\"RX: %{u,m}s\"}]}";

   @Test
   public void testDirectionParsing()
   {
      MapLinkDataSource[] dciList = new Gson().fromJson(CONFIG, LinkConfig.class).getDciList();
      assertEquals(LinkDataDirection.FORWARD, dciList[0].getDirection());
      assertEquals(LinkDataDirection.REVERSE, dciList[1].getDirection());
      assertEquals(LinkDataDirection.NONE, dciList[2].getDirection());
   }

   @Test
   public void testDirectionRoundTrip()
   {
      Gson gson = new Gson();
      LinkConfig config = gson.fromJson(gson.toJson(gson.fromJson(CONFIG, LinkConfig.class)), LinkConfig.class);
      assertEquals(LinkDataDirection.FORWARD, config.getDciList()[0].getDirection());
      assertEquals(LinkDataDirection.REVERSE, new MapLinkDataSource(config.getDciList()[1]).getDirection());
   }

   @Test
   public void testDirectionOfUserDataSource()
   {
      MapLinkDataSource dataSource = new Gson().fromJson(CONFIG, LinkConfig.class).getDciList()[0];
      dataSource.setSystem(false);
      assertEquals(LinkDataDirection.FORWARD, dataSource.getDirection());
      dataSource.setDirection(LinkDataDirection.NONE);
      assertEquals(LinkDataDirection.NONE, new Gson().fromJson(new Gson().toJson(dataSource), MapLinkDataSource.class).getDirection());
   }
}
