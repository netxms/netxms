package org.netxms.nxmc.modules.actions.views.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.ServerActionType;
import org.netxms.nxmc.resources.ResourceManager;

public class BaseActionLabelProvider extends LabelProvider
{
   private static Map<ServerActionType, Image> images;

   static
   {
      images = new HashMap<ServerActionType, Image>();
      images.put(ServerActionType.LOCAL_COMMAND, ResourceManager.getImageDescriptor("icons/actions/exec_local.png").createImage());
      images.put(ServerActionType.AGENT_COMMAND, ResourceManager.getImageDescriptor("icons/actions/exec_remote.png").createImage());
      images.put(ServerActionType.SSH_COMMAND, ResourceManager.getImageDescriptor("icons/actions/exec_remote.png").createImage());
      images.put(ServerActionType.NOTIFICATION, ResourceManager.getImageDescriptor("icons/actions/email.png").createImage());
      images.put(ServerActionType.FORWARD_EVENT, ResourceManager.getImageDescriptor("icons/actions/fwd_event.png").createImage());
      images.put(ServerActionType.NXSL_SCRIPT, ResourceManager.getImageDescriptor("icons/actions/exec_script.gif").createImage());
      images.put(ServerActionType.XMPP_MESSAGE, ResourceManager.getImageDescriptor("icons/actions/xmpp.gif").createImage());
   }

   public Image getColumnImage(Object element, int columnIndex)
   {
      return images.get(((ServerAction)element).getType());
   }

   public String getColumnText(Object element, int columnIndex)
   {
      return ((ServerAction)element).getName();
   }

   /**
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      super.dispose();
   }

}
