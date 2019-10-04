package org.netxms.ui.eclipse.objectview.widgets.helpers;

import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.PhysicalLink;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.ui.eclipse.objectview.widgets.PhysicalLinkWidget;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Physical link label provider
 */
public class PhysicalLinkLabelProvider extends LabelProvider implements ITableLabelProvider
{  
   private NXCSession session;
   private WorkbenchLabelProvider wbLabelProvider;

   /**
    * Physical link label provider constructor
    */
   public PhysicalLinkLabelProvider()
   {
      session = ConsoleSharedData.getSession();
      wbLabelProvider = new WorkbenchLabelProvider();
   }

   /**
    * Get text for object
    * 
    * @param link reference to link
    * @param isLeft if left or right link object should be used
    * @return text to display about object
    */
   public String getObjectText(PhysicalLink link, boolean isLeft)
   {
      AbstractObject obj = null;
      if (isLeft)
         obj = session.findObjectById(link.getLeftObjectId());
      else
         obj = session.findObjectById(link.getRightObjectId());
      String name = "Unknown";
      if(obj instanceof Interface)
      {
         name = ((Interface)obj).getParentNode().getObjectName();
      }
      else if (obj instanceof Rack)
      {
         name = obj.getObjectName();
      }
      return name;
   }

   /**
    * Get port text - text with interface or with patch panel information
    * 
    * @param link link object
    * @param isLeft if left or right link object should be used
    * @return text with port description
    */
   public String getPortText(PhysicalLink link, boolean isLeft)
   {      
      if(isLeft)
         return getPortTextInternal(link.getLeftObjectId(), link.getLeftPatchPanelId(), link.getLeftPortNumber(), link.getLeftFront());
      else
         return getPortTextInternal(link.getRightObjectId(), link.getRightPatchPanelId(), link.getRightPortNumber(), link.getRightFront());         
   }

   /**
    * Internal function that generates port text - text with interface or with patch panel information
    * 
    * @param objectId link object id
    * @param patchPanelId link patch panel id
    * @param portNumber link port number
    * @param side link side
    * @return text with port description
    */
   private String getPortTextInternal(long objectId, long patchPanelId, int portNumber, int side)
   {
      StringBuilder sb = new StringBuilder();
      AbstractObject obj = session.findObjectById(objectId);
      if(obj instanceof Interface)
      {
         sb.append(obj.getObjectName());
      }
      else if (obj instanceof Rack)
      {
         PassiveRackElement el = ((Rack)obj).getPassiveElement(patchPanelId);
         sb.append(el.toString());
         sb.append("/");
         sb.append(portNumber);
         sb.append("/");
         sb.append(side == 0 ? "Front" : "Back");
      }
      return sb.toString();
   }

   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      AbstractObject object = null;
      switch(columnIndex)
      {
         case PhysicalLinkWidget.LEFT_PORT:
            object = session.findObjectById(((PhysicalLink)element).getLeftObjectId());
            return (object != null && object instanceof Interface) ? wbLabelProvider.getImage(object) : null;
         case PhysicalLinkWidget.RIGHT_PORT:
            object = session.findObjectById(((PhysicalLink)element).getRightObjectId());
            return (object != null && object instanceof Interface) ? wbLabelProvider.getImage(object) : null;
         case PhysicalLinkWidget.LEFT_OBJECT:
            object = session.findObjectById(((PhysicalLink)element).getLeftObjectId());
            break;
         case PhysicalLinkWidget.RIGHT_OBJECT:
            object = session.findObjectById(((PhysicalLink)element).getRightObjectId());
            break;
         
      }
      if(object != null)
      {
         if(object instanceof Interface)
         {
            object = ((Interface)object).getParentNode();
         }
         return (object != null) ? wbLabelProvider.getImage(object) : null;
      }
      return null;
   }

   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      PhysicalLink link = (PhysicalLink)element;
      switch(columnIndex)
      {
         case PhysicalLinkWidget.PHYSICAL_LINK_ID:
            return Long.toString(link.getId());
         case PhysicalLinkWidget.DESCRIPTOIN:
            return link.getDescription();
         case PhysicalLinkWidget.LEFT_OBJECT:
         case PhysicalLinkWidget.RIGHT_OBJECT:
            return getObjectText(link, columnIndex == PhysicalLinkWidget.LEFT_OBJECT);
         case PhysicalLinkWidget.LEFT_PORT:
         case PhysicalLinkWidget.RIGHT_PORT:
            return getPortText(link, columnIndex == PhysicalLinkWidget.LEFT_PORT);
      }
      return null;
   }

}
