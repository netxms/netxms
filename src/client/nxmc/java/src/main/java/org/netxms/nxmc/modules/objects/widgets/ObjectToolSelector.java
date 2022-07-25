/**
 * 
 */
package org.netxms.nxmc.modules.objects.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.base.widgets.AbstractSelector;
import org.netxms.nxmc.modules.objects.ObjectToolsCache;
import org.netxms.nxmc.modules.objects.dialogs.ObjectToolSelectionDialog;

/**
 * Object tool selector
 */
public class ObjectToolSelector extends AbstractSelector
{
   private long toolId;
   
   /**
    * @param parent
    * @param style
    * @param options
    */
   public ObjectToolSelector(Composite parent, int style, int options)
   {
      super(parent, style, options);
      toolId = 0;
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#selectionButtonHandler()
    */
   @Override
   protected void selectionButtonHandler()
   {
      ObjectToolSelectionDialog dlg = new ObjectToolSelectionDialog(getShell());
      if (dlg.open() == Window.OK)
      {
         toolId = dlg.getTool().getId();
         updateToolName();
      }
   }

   /**
    * @see org.netxms.nxmc.base.widgets.AbstractSelector#clearButtonHandler()
    */
   @Override
   protected void clearButtonHandler()
   {
      toolId = 0;
      updateToolName();
   }

   /**
    * @return the toolId
    */
   public long getToolId()
   {
      return toolId;
   }

   /**
    * @param toolId the toolId to set
    */
   public void setToolId(long toolId)
   {
      this.toolId = toolId;
      updateToolName();
   }
   
   /**
    * Update tool name in selector
    */
   private void updateToolName()
   {
      if (toolId == 0)
      {
         setText("");
         return;
      }

      ObjectTool t = ObjectToolsCache.getInstance().findTool(toolId);
      if (t != null)
         setText(t.getName());
      else
         setText("[" + toolId + "]");
   }
}
