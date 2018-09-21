/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Raden Solutions
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

package org.netxms.websvc.handlers;

import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.websvc.json.ResponseContainer;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

public class ObjectTools extends AbstractObjectHandler
{
   @Override protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      AbstractObject object = getObject();
      if (object.getObjectClass() != AbstractObject.OBJECT_NODE)
         throw new NXCException(RCC.INVALID_OBJECT_ID);

      List<ObjectTool> objectTools = session.getObjectTools();
      List<ObjectTool> result = new ArrayList<ObjectTool>();
      for(ObjectTool t : objectTools)
         if (t.isApplicableForNode((AbstractNode)object))
            result.add(t);

      return new ResponseContainer("objectTools", objectTools);
   }
}
