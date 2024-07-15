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

import java.util.List;
import org.junit.jupiter.api.Test;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.GraphDefinition;

/**
 * Graph configuration test
 *
 */
public class GraphTest extends AbstractSessionTest
{
   @Test
	public void testGetPredefinedGraphs() throws Exception
	{
		final NXCSession session = connectAndLogin();
		
      List<GraphDefinition> graphs = session.getPredefinedGraphs(false);
      System.out.println(graphs.size() + " predefined graphs retrieved");
      for(GraphDefinition g : graphs)
         System.out.println(">>> " + g.getName());
		
		session.disconnect();
	}
}
