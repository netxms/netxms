package org.netxms.nxmc.modules.objects.views;

import org.eclipse.jface.resource.ImageDescriptor;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.modules.objects.ObjectContext;

/**
 * Base Object tool result view class
 */
public abstract class ObjectToolResults extends ObjectView
{
   protected NXCSession session;
   protected ObjectTool tool;
   protected ObjectContext object;

   /**
    * Get View name
    * 
    * @param object object
    * @param tool object tool 
    * @return view name
    */
   protected static String getViewName(AbstractObject object, ObjectTool tool)
   {
      return object.getObjectName() + " - " + tool.getDisplayName();
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      TableToolResults view = (TableToolResults)super.cloneView();
      view.tool = tool;
      view.object = object;
      return view;
   } 
   
   /**
    * Get View id
    * 
    * @param object object
    * @param tool object tool 
    * @return view id
    */
   protected static String getViewId(AbstractObject object, ObjectTool tool)
   {
      return object.getObjectId() + " - " + tool.getId();
   }
   
   public ObjectToolResults(ObjectContext object, ObjectTool tool, ImageDescriptor image, boolean hasFilter)
   {
      super(getViewName(object.object, tool), image, getViewId(object.object, tool), hasFilter);
      this.tool = tool;
      this.object = object;
      session = (NXCSession)Registry.getSession();
   }

   @Override
   public boolean isCloseable()
   {
      return true;
   }  

   @Override
   public boolean isValidForContext(Object context)
   {
      if (context != null && context instanceof AbstractObject)
      {
         if (((AbstractObject)context).getObjectId() == object.object.getObjectId())
            return true;
      }
      return false;
   } 
}
