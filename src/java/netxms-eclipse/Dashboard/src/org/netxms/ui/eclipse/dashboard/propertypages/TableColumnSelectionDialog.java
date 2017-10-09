package org.netxms.ui.eclipse.dashboard.propertypages;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;

public class TableColumnSelectionDialog  extends Dialog
{

   private TableViewer columnNames;
   private String valuesArray[];
   private String selectedName;

   public TableColumnSelectionDialog(Shell parentShell, String valuesArray[])
   {
      super(parentShell);
      this.valuesArray = valuesArray;
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
      
      columnNames  = new TableViewer(dialogArea, SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 2;
      gd.heightHint = 150;
      columnNames.getTable().setLayoutData(gd);
      columnNames.getTable().setSortDirection(SWT.UP);
      columnNames.setContentProvider(new ArrayContentProvider());
      columnNames.setInput(valuesArray);
      columnNames.addSelectionChangedListener(new ISelectionChangedListener() {
         
         @Override
         public void selectionChanged(SelectionChangedEvent event)
         {
            IStructuredSelection selection = (IStructuredSelection)event.getSelection();
            selectedName = (String)selection.getFirstElement();
         }
      });
      
      return dialogArea;
   }
   


   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if(selectedName == null)
         return;
      super.okPressed();
   }   

   /**
    * @param selectedName the selectedName to set
    */
   public String getSelectedName()
   {
      return selectedName;
   }
}
