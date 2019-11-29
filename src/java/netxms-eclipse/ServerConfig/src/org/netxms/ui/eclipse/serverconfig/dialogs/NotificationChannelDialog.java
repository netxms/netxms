package org.netxms.ui.eclipse.serverconfig.dialogs;

import java.util.Collections;
import java.util.List;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.NXCSession;
import org.netxms.client.NotificationChannel;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.serverconfig.Activator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * Notification channel dialog
 */
public class NotificationChannelDialog extends Dialog
{
   private NotificationChannel nc;
   private LabeledText textName;
   private LabeledText textDescription;
   private LabeledText textConfiguraiton;
   private Combo comboDriverName;
   private boolean isNameChangeds;
   private String newName;

   public NotificationChannelDialog(Shell parentShell, NotificationChannel nc)
   {
      super(parentShell);
      this.nc = nc;
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
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);
      
      textName = new LabeledText(dialogArea, SWT.NONE);
      textName.setLabel("Name");
      textName.getTextControl().setTextLimit(63);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      textName.setLayoutData(gd);
      
      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel("Description");
      textDescription.getTextControl().setTextLimit(255);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textDescription.setLayoutData(gd);

      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      comboDriverName = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, "Driver name", gd);
      
      textConfiguraiton = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER);
      textConfiguraiton.setLabel("Driver Configuration");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.heightHint = 300;
      gd.widthHint = 900;
      textConfiguraiton.setLayoutData(gd);          

      if(nc != null)
      {
         textName.setText(nc.getName());
         textDescription.setText(nc.getDescription());
         textConfiguraiton.setText(nc.getConfiguration());
      }
      
      final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
      new ConsoleJob("Get driver names", null, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
         {
            final List<String> ncList = session.getDriverNames();
            Collections.sort(ncList, String.CASE_INSENSITIVE_ORDER);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  updateUI(ncList);
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot get driver names";
         }
      }.start(); 
      
      return dialogArea;
   }
   
   private void updateUI(List<String> ncList)
   {
      for(int i = 0; i < ncList.size(); i++)
      {
         comboDriverName.add(ncList.get(i));
         if(nc != null && ncList.get(i).equals(nc.getDriverName()))
            comboDriverName.select(i);
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(nc != null ? "Update notification channel" : "Create notification channel");
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if(textName.getText().isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Notification channel name should not be empty");
         return;
      }
      
      if(comboDriverName.getSelectionIndex() == -1)
      {
         MessageDialogHelper.openWarning(getShell(), "Warning", "Notification driver should be selected");
         return;
      }
      
      if(nc == null)
      {
         nc = new NotificationChannel();
         nc.setName(textName.getText());
      }
      else if(!nc.getName().equals(textName.getText()))
      {
         isNameChangeds = true;
         newName = textName.getText();
      }
      nc.setDescription(textDescription.getText());
      nc.setDriverName(comboDriverName.getItem(comboDriverName.getSelectionIndex()));
      nc.setConfiguration(textConfiguraiton.getText());      
      super.okPressed();
   }
   
   /**
    * Return updated notification channel
    */
   public NotificationChannel getNotificaiotnChannel()
   {
      return nc;
   }
   
   /**
    * Returns if name is changed
    */
   public boolean isNameChanged()
   {
      return isNameChangeds;
   }
   
   /**
    * Get new name
    */
   public String getNewName()
   {
      return newName;
   }
}
