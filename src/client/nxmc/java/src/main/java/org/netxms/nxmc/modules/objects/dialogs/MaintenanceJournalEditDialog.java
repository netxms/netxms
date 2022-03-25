package org.netxms.nxmc.modules.objects.dialogs;

import java.util.Date;
import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.window.IShellProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.MaintenanceJournalEntry;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Interface;
import org.netxms.client.objects.Rack;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.modules.objects.widgets.ObjectSelector;
import org.netxms.nxmc.modules.objects.widgets.PatchPanelSelector;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;

/**
 * Dialog to create/edit maintenance journal entry
 */
public class MaintenanceJournalEditDialog extends Dialog
{
   private NXCSession session;
   private String title;
   private String objectName;
   private String entryDesc;

   private LabeledText descriptionText;

   /**
    * Physical link add/edit dialog constructor
    * 
    * @param parentShell
    * @param link link to edit or null if new should be created
    */
   public MaintenanceJournalEditDialog(Shell parentShell, String objectName, String description)
   {
      super(parentShell);

      this.objectName = objectName;
      entryDesc = description != null ? description : "";

      title = entryDesc != "" ? "Edit Maintenance Journal Entry" : "Create Maintenance Journal Entry";
      session = Registry.getSession();
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(title);
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = new Composite(parent, SWT.RESIZE);

      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      GridData gd = new GridData();
      gd.widthHint = 700;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;

      LabeledText objectText = new LabeledText(dialogArea, SWT.NONE);
      objectText.setLabel("Object");
      objectText.setText(objectName);
      objectText.setLayoutData(gd);
      objectText.getTextControl().setEditable(false);

      gd = new GridData();
      gd.heightHint = 400;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;

      descriptionText = new LabeledText(dialogArea, SWT.NONE, SWT.MULTI | SWT.BORDER | SWT.WRAP | SWT.V_SCROLL);
      descriptionText.setLabel("Description");
      descriptionText.setText(entryDesc);
      descriptionText.setLayoutData(gd);

      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      if (entryDesc.equals(descriptionText.getText()))
      {
         super.cancelPressed();
      }
      else
      {
         entryDesc = descriptionText.getText();
         super.okPressed();
      }
   }

   /**
    * Get description
    * 
    * @return description string
    */
   public String getDescription()
   {
      return entryDesc;
   }

}
