/**
 * 
 */
package org.netxms.ui.eclipse.objecttools.widgets;

import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolsCache;
import org.netxms.ui.eclipse.objecttools.dialogs.ObjectToolSelectionDialog;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

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

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#selectionButtonHandler()
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

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.widgets.AbstractSelector#clearButtonHandler()
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
