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
package org.netxms.websvc.handlers;

import java.util.Map;
import org.json.JSONObject;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.businessservices.BusinessServiceCheck;
import org.netxms.client.constants.RCC;
import org.netxms.websvc.json.JsonTools;
import org.netxms.websvc.json.ResponseContainer;

/**
 * Business service check request handler
 */
public class BusinessServiceChecks extends AbstractObjectHandler
{   
   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {      
      NXCSession session = getSession();
      Map<Long, BusinessServiceCheck> checks = session.getBusinessServiceChecks(getObjectId());
      return new ResponseContainer("checks", checks.values());
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#create(org.json.JSONObject)
    */
   @Override
   protected Object create(JSONObject data) throws Exception
   {  
      NXCSession session = getSession();
      BusinessServiceCheck check = JsonTools.createGsonInstance().fromJson(data.toString(), BusinessServiceCheck.class);        
      check.setId(0);
      session.modifyBusinessServiceCheck(getObjectId(), check);     
      return null;     
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#update(java.lang.String, org.json.JSONObject)
    */
   @Override
   protected Object update(String id, JSONObject data) throws Exception
   {
      NXCSession session = getSession();
      long checkId = 0;
      try 
      {
         checkId = Long.parseLong(id);
      }
      catch(NumberFormatException e)
      {
         throw new NXCException(RCC.INVALID_BUSINESS_CHECK_ID);
      }
      BusinessServiceCheck check = JsonTools.createGsonInstance().fromJson(data.toString(), BusinessServiceCheck.class);    
      check.setId(checkId);
      session.modifyBusinessServiceCheck(getObjectId(), check);     
      return null;  
   }

   /**
    * @see org.netxms.websvc.handlers.AbstractHandler#delete(java.lang.String)
    */
   @Override
   protected Object delete(String id) throws Exception
   {  
      NXCSession session = getSession();
      long checkId = 0;
      try 
      {
         checkId = Long.parseLong(id);
      }
      catch(NumberFormatException e)
      {
         throw new NXCException(RCC.INVALID_BUSINESS_CHECK_ID);
      }
      session.deleteBusinessServiceCheck(getObjectId(), checkId);      
      return null;      
   }
}
