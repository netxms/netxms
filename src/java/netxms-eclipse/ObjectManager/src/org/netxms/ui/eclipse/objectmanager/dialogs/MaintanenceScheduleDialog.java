package org.netxms.ui.eclipse.objectmanager.dialogs;

import java.util.Date;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.DateTimeSelector;

public class MaintanenceScheduleDialog extends Dialog
{
   private Date startDate;
   private Date endDate;
   private DateTimeSelector startDateSelector;
   private DateTimeSelector endDateSelector;
   private Label labelStartDate;
   private Label labelEndDate;
   
   

   /**
    * @param parentShell
    */
   public MaintanenceScheduleDialog(Shell parentShell)
   {
      super(parentShell);
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Schedule Maintenance");
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);
      
      labelStartDate = new Label(dialogArea, SWT.NONE);
      labelStartDate.setText("Start date");
            
      startDateSelector = new DateTimeSelector(dialogArea, SWT.NONE);
      startDateSelector.setValue(new Date());
      startDateSelector.setToolTipText("Start date");
      
      labelEndDate = new Label(dialogArea, SWT.NONE);
      labelEndDate.setText("End date");
      
      endDateSelector = new DateTimeSelector(dialogArea, SWT.NONE);
      endDateSelector.setValue(new Date());
      startDateSelector.setToolTipText("End date");
      
      return dialogArea;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      startDate = startDateSelector.getValue();
      endDate = endDateSelector.getValue();
      if (startDate.after(endDate))
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Start time must be gather end time.");
         return;
      }
      
      super.okPressed();
   }

   public Date getStartDate()
   {
      return startDate;
   }

   public Date getEndDate()
   {
      return endDate;
   }
   
}
