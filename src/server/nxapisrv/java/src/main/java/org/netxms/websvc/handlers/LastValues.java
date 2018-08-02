package org.netxms.websvc.handlers;

import java.util.Map;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.RCC;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.websvc.json.ResponseContainer;

public class LastValues extends AbstractObjectHandler
{

   /* (non-Javadoc)
    * @see org.netxms.websvc.handlers.AbstractHandler#getCollection(java.util.Map)
    */
   @Override
   protected Object getCollection(Map<String, String> query) throws Exception
   {
      NXCSession session = getSession();
      AbstractObject obj = getObject();
      
      if (!(obj instanceof DataCollectionTarget))
         throw new NXCException(RCC.INVALID_OBJECT_ID);
      
      DciValue[] values = session.getLastValues(obj.getObjectId());
      
      return new ResponseContainer("lastValues", values);
   }
}
