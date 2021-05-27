package org.netxms.nxmc.modules.datacollection.widgets.helpers;

import org.eclipse.jface.viewers.StructuredViewer;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.views.ViewerFilterInternal;

public interface DataCollectionCommon
{
   public void refresh();
   
   public void postContentCreate();
   
   public void layout();
   
   public void dispose();
   
   public StructuredViewer getViewer();

   public ViewerFilterInternal getFilter();
   
   public void setDataCollectionTarget(AbstractObject dcTarget);
}
