/**
 * 
 */
package org.netxms.ui.eclipse.objecttools.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.eclipse.objecttools.Activator;
import org.netxms.ui.eclipse.objecttools.api.ObjectToolsCache;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Object tool selection dialog
 */
public class ObjectToolSelectionDialog extends Dialog
{
   private static final String CONFIG_PREFIX = "ObjectToolSelectionDialog";
   
   private TableViewer viewer;
   private ObjectTool tool;
   
   /**
    * @param parent
    */
   public ObjectToolSelectionDialog(Shell parent)
   {
      super(parent);
      setShellStyle(getShellStyle() | SWT.RESIZE);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Select Tool");
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      try
      {
         newShell.setSize(settings.getInt(CONFIG_PREFIX + ".cx"), settings.getInt(CONFIG_PREFIX + ".cy")); //$NON-NLS-1$ //$NON-NLS-2$
      }
      catch(NumberFormatException e)
      {
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      GridLayout layout = new GridLayout();
      dialogArea.setLayout(layout);
      
      viewer = new TableViewer(dialogArea, SWT.BORDER);
      viewer.setContentProvider(new ArrayContentProvider());
      viewer.setComparator(new ViewerComparator() {
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            return ((ObjectTool)e1).getName().compareToIgnoreCase(((ObjectTool)e2).getName());
         }
      });
      viewer.setLabelProvider(new LabelProvider() {
         @Override
         public String getText(Object element)
         {
            return ((ObjectTool)element).getName();
         }
      });      
      viewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            ObjectToolSelectionDialog.this.okPressed();
         }
      });
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 500;
      gd.widthHint = 350;
      viewer.getTable().setLayoutData(gd);
      
      viewer.setInput(ObjectToolsCache.getInstance().getTools());
      
      return dialogArea;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (viewer.getSelection().isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Please select tool from the list");
         return;
      }
      tool = (ObjectTool)((IStructuredSelection)viewer.getSelection()).getFirstElement();
      super.okPressed();
   }

   /**
    * @return the tool
    */
   public ObjectTool getTool()
   {
      return tool;
   }
}
