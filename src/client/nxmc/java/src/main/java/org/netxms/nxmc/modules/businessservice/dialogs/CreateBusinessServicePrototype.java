package org.netxms.nxmc.modules.businessservice.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

public class CreateBusinessServicePrototype extends Dialog
{
   private I18n i18n = LocalizationHelper.getI18n(CreateBusinessServicePrototype.class);
   
   private LabeledText nameField;
   private Combo instanceDiscoveyMethodCombo;
   
   private String name;
   private int instanceDiscoveyMethod;

   public CreateBusinessServicePrototype(Shell parentShell)
   {
      super(parentShell);
   }

   /**
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(i18n.tr("Create Business Service Prototype"));
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      GridLayout layout = new GridLayout();
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      dialogArea.setLayout(layout);

      nameField = new LabeledText(dialogArea, SWT.NONE);
      nameField.setLabel(i18n.tr("Name"));
      nameField.getTextControl().setTextLimit(255);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 300;
      nameField.setLayoutData(gd);

      instanceDiscoveyMethodCombo = WidgetHelper.createLabeledCombo(dialogArea, SWT.READ_ONLY, i18n.tr("Instance discovery type"), new GridData(SWT.FILL, SWT.CENTER, true, false));
      instanceDiscoveyMethodCombo.add(i18n.tr("Script"));
      instanceDiscoveyMethodCombo.add(i18n.tr("Agent List"));
      instanceDiscoveyMethodCombo.add(i18n.tr("Agent Table"));
      instanceDiscoveyMethodCombo.select(0);
      
      return dialogArea;
   }

   /**
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      instanceDiscoveyMethod = instanceDiscoveyMethodCombo.getSelectionIndex();
      name = nameField.getText().trim();
      if (name.isEmpty())
      {
         MessageDialogHelper.openWarning(getShell(), i18n.tr("Warning"), i18n.tr("Object name cannot be empty"));
         return;
      }
      super.okPressed();
   }

   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the instanceDiscoveyMethod
    */
   public int getInstanceDiscoveyMethod()
   {
      return instanceDiscoveyMethod;
   }
}
