package org.netxms.ui.eclipse.dashboard.dialogs;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.datacollection.DciSummaryTable;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.widgets.DCISummaryTableColumnSelector;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.widgets.AbstractSelector;

public class DCISummaryTableSortColumnSelectionDialog extends Dialog
{
   String sortingColumn = "";
   DCISummaryTableColumnSelector selector;
   DciSummaryTable sourceSummaryTable;
   Button descSorting;
   boolean isDescSorting;
   int summaryTableId;
   
   public DCISummaryTableSortColumnSelectionDialog(Shell parentShell, String sortingColumn, boolean descSorting, int summaryTableId)
   {
      super(parentShell);
      this.sortingColumn = sortingColumn;
      this.isDescSorting = descSorting;   
      this.summaryTableId = summaryTableId;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Edit sorting column");
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob("Get summary table configuration by id", null, Activator.PLUGIN_ID, null) {
         
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            sourceSummaryTable = session.getDciSummaryTable(summaryTableId);
            runInUIThread(new Runnable() {
               
               @Override
               public void run()
               {
                  refresh();
               }
            });
            
         }
         
         @Override
         protected String getErrorMessage()
         {
            return "Unable to get summary table configuration";
         }
      }.start();
      
      GridLayout layout = new GridLayout(); 
      dialogArea.setLayout(layout);
      
      selector = new DCISummaryTableColumnSelector(dialogArea, SWT.NONE, AbstractSelector.SHOW_CLEAR_BUTTON, sortingColumn, null, sourceSummaryTable);
      selector.setLabel("Filter column name");
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 520;
      selector.setLayoutData(gd);
      
      descSorting = new Button(dialogArea, SWT.CHECK);
      gd = new GridData();
      //gd.horizontalAlignment = SWT.FILL;
      //gd.grabExcessHorizontalSpace = true;
      descSorting.setText("Use descending sorting");
      descSorting.setLayoutData(gd);
      descSorting.setSelection(isDescSorting);
      
      return dialogArea;
   }
   
   private void refresh()
   {
      selector.setSummaryTbale(sourceSummaryTable);
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      // TODO Auto-generated method stub
      sortingColumn = (descSorting.getSelection() ? ">" : "<") + selector.getColumnName();
      super.okPressed();
   }

   /**
    * @return the sortingColumn
    */
   public String getSortingColumn()
   {
      return sortingColumn;
   }
}
