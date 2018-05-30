/**
 * 
 */
package org.netxms.ui.eclipse.dashboard.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.dashboard.widgets.internal.ObjectToolsConfig.Tool;
import org.netxms.ui.eclipse.objectbrowser.widgets.ObjectSelector;
import org.netxms.ui.eclipse.objecttools.widgets.ObjectToolSelector;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Edit tool configuration dialog
 */
public class EditToolDialog extends Dialog
{
   private LabeledText name;
   private ObjectSelector objectSelector;
   private ObjectToolSelector toolSelector;
   private Tool tool;

   /**
    * @param parentShell
    */
   public EditToolDialog(Shell parentShell, Tool tool)
   {
      super(parentShell);
      this.tool = tool;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Edit Tool");
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);
      
      name = new LabeledText(dialogArea, SWT.NONE);
      name.setLabel("Display name");
      name.getTextControl().setTextLimit(63);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 350;
      name.setLayoutData(gd);

      objectSelector = new ObjectSelector(dialogArea, SWT.NONE, false);
      objectSelector.setLabel("Object");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 350;
      objectSelector.setLayoutData(gd);
      
      toolSelector = new ObjectToolSelector(dialogArea, SWT.NONE, 0);
      toolSelector.setLabel("Tool");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 350;
      toolSelector.setLayoutData(gd);
      
      return dialogArea;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      tool.name = name.getText().trim();
      tool.objectId = objectSelector.getObjectId();
      tool.toolId = toolSelector.getToolId();
      super.okPressed();
   }

   /**
    * @return the tool
    */
   public Tool getTool()
   {
      return tool;
   }
}
