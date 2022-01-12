/**
 * 
 */
package org.netxms.ui.eclipse.usermanager.dialogs.helpers;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * @author victor
 *
 */
public class CustomMethodBindingConfigurator extends AbstractMethodBindingConfigurator
{
   private LabeledText configuration;

   /**
    * @param parent
    */
   public CustomMethodBindingConfigurator(Composite parent)
   {
      super(parent);
      
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      setLayout(layout);

      configuration = new LabeledText(this, SWT.NONE, SWT.BORDER | SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      configuration.setLabel("Configuration");
      GridData gd = new GridData();
      gd.heightHint = 400;
      gd.widthHint = 600;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      configuration.setLayoutData(gd);
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#setConfiguration(java.lang.String)
    */
   @Override
   public void setConfiguration(String configuration)
   {
      this.configuration.setText(configuration);
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#getConfiguration()
    */
   @Override
   public String getConfiguration()
   {
      return configuration.getText().trim();
   }
}
