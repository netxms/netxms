package org.netxms.nxmc.base.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.StructuredViewer;
import org.eclipse.jface.widgets.WidgetFactory;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.views.AbstractViewerFilter;
import org.netxms.nxmc.base.widgets.FilterText;

public abstract class DialogWithFilter extends Dialog
{
   private FilterText filterText;
   private AbstractViewerFilter filter;
   private StructuredViewer viewer;

   protected DialogWithFilter(Shell parentShell)
   {
      super(parentShell);
   }

   
   
   @Override
   protected Control createContents(Composite parent)
   {
      // create the top level composite for the dialog
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      Composite composite = WidgetFactory.composite(0).layout(layout).layoutData(new GridData(GridData.FILL_BOTH))
            .create(parent);
      applyDialogFont(composite);
      // initialize the dialog units
      initializeDialogUnits(composite);
      // create the dialog area and button bar

      filterText = new FilterText(composite, SWT.NONE, null, false);
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      filterText.setLayoutData(gd);      
      filterText.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            onFilterModify();
         }
      });    
      
      dialogArea = createDialogArea(composite);
      dialogArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      
      buttonBar = createButtonBar(composite);
      filterText.setFocus();

      return composite;
   }
   
   /**
    * Handler for filter modification
    */
   protected void onFilterModify()
   {
      if ((filter != null) && (viewer != null))
         defaultOnFilterModify();
   }

   /**
    * Set viewer and filter that will be managed by default filter string handler
    * 
    * @param viewer viewer to refresh
    * @param filter filter to set filtering text
    */
   protected void setFilterClient(StructuredViewer viewer, AbstractViewerFilter filter)
   {
      this.viewer = viewer;
      this.filter = filter;
   }

   /**
    * Default on modify for view with viewer
    */
   private void defaultOnFilterModify()
   {
      final String text = filterText.getText();
      filter.setFilterString(text);
      viewer.refresh(false);
   }
}
