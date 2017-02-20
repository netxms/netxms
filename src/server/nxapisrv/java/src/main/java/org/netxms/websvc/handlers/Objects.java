/**
 * 
 */
package org.netxms.websvc.handlers;

import java.util.Iterator;
import java.util.List;
import java.util.Map;
import org.netxms.base.Glob;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.objects.AbstractObject;
import org.netxms.websvc.json.ResponseContainer;

/**
 * Objects request handler
 */
public class Objects extends AbstractHandler
{
   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(org.json.JSONObject)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      if (!session.isObjectsSynchronized())
         session.syncObjects();
      
      List<AbstractObject> objects = session.getAllObjects();
      
      String nameFilter = query.get("name");
      String classFilter = query.get("class");
      if ((nameFilter != null) || (classFilter != null))
      {
         Iterator<AbstractObject> it = objects.iterator();
         while(it.hasNext())
         {
            AbstractObject o = it.next();
            
            if ((nameFilter != null) && !nameFilter.isEmpty() && !Glob.matchIgnoreCase(nameFilter, o.getObjectName()))
            {
               it.remove();
               continue;
            }

            if ((classFilter != null) && !classFilter.isEmpty())
            {
               String[] classes = classFilter.split(",");
               boolean match = false;
               for(String c : classes)
               {
                  if (o.getObjectClassName().equalsIgnoreCase(c))
                  {
                     match = true;
                     break;
                  }
               }
               if (!match)
               {
                  it.remove();
                  continue;
               }
            }
         }
      }

      return new ResponseContainer("objects", objects);
   }

   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#get(java.lang.String)
    */
   @Override
   protected Object get(String id) throws Exception
   {
      Long objectId = Long.parseLong(id);
      AbstractObject object = getSession().findObjectById(objectId);
      return (object != null) ? object : createErrorResponse(RCC.INVALID_OBJECT_ID);
   }
}
