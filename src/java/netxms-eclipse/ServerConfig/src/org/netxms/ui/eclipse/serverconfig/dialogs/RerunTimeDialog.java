package org.netxms.ui.eclipse.serverconfig.dialogs;

import java.util.Date;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.DateTimeSelector;

public class RerunTimeDialog extends Dialog
{
   private Date rerunDate;
   private DateTimeSelector dateSelector;

   public RerunTimeDialog(Shell shell, Date currentlySelectedDate)
   {
      super(shell);
      rerunDate = currentlySelectedDate;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      final Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.numColumns = 2;
      dialogArea.setLayout(layout);  

      final WidgetFactory factory = new WidgetFactory() {
         @Override
         public Control createControl(Composite parent, int style)
         {
            return new DateTimeSelector(parent, style);
         }
      };
      
      dateSelector = (DateTimeSelector)WidgetHelper.createLabeledControl(dialogArea, SWT.NONE, factory, "", WidgetHelper.DEFAULT_LAYOUT_DATA);
      dateSelector.setValue(new Date());

      return dialogArea;
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText("Execution date");
   }

   
   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      rerunDate = dateSelector.getValue();
      super.okPressed();
   }

   /**
    * Date selected by user
    * @return rerun date
    */
   public Date getRerunDate()
   {
      return rerunDate;      
   }
}
