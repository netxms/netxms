/**
 * 
 */
package org.netxms.nxmc.base.preferencepages;

import org.eclipse.jface.preference.PreferencePage;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.nxmc.PreferenceStore;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * "Appearance" preference page
 */
public class Appearance extends PreferencePage
{
   private static final I18n i18n = LocalizationHelper.getI18n(Appearance.class);

   private Button checkVerticalLayout;

   public Appearance()
   {
      super(i18n.tr("Appearance"));
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createContents(Composite parent)
   {
      PreferenceStore settings = PreferenceStore.getInstance();

      Composite dialogArea = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      dialogArea.setLayout(layout);

      checkVerticalLayout = new Button(dialogArea, SWT.CHECK);
      checkVerticalLayout.setText("Vertical layout of perspective switcher");
      checkVerticalLayout.setSelection(settings.getAsBoolean("Appearance.VerticalLayout", true));

      return dialogArea;
   }

   /**
    * Apply changes
    */
   private void doApply()
   {
      PreferenceStore settings = PreferenceStore.getInstance();
      settings.set("Appearance.VerticalLayout", checkVerticalLayout.getSelection());
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performApply()
    */
   @Override
   protected void performApply()
   {
      doApply();
   }

   /**
    * @see org.eclipse.jface.preference.PreferencePage#performOk()
    */
   @Override
   public boolean performOk()
   {
      doApply();
      return true;
   }
}
