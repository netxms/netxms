/**
 * 
 */
package org.netxms.ui.eclipse.market.dialogs;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.netxms.client.market.Repository;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;

/**
 * "Add Repository"/"Edit Repository" dialog
 */
public class RepositoryPropertiesDlg extends Dialog
{
   private LabeledText textUrl;
   private LabeledText textToken;
   private LabeledText textDescription;
   private String url = "";
   private String token = "";
   private String description = "";
   boolean isAdd;
   
   /**
    * @param parentShell
    */
   public RepositoryPropertiesDlg(Shell parentShell, Repository repository)
   {
      super(parentShell);
      if (repository != null)
      {
         isAdd = false;
         url = repository.getUrl();
         token = repository.getAuthToken();
         description = repository.getDescription();
      }
      else
      {
         isAdd = true;
      }
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
    */
   @Override
   protected void configureShell(Shell newShell)
   {
      super.configureShell(newShell);
      newShell.setText(isAdd ? "Add Repository" : "Edit Repository");
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createDialogArea(Composite parent)
   {
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      GridLayout layout = new GridLayout();
      layout.marginHeight = WidgetHelper.DIALOG_HEIGHT_MARGIN;
      layout.marginWidth = WidgetHelper.DIALOG_WIDTH_MARGIN;
      layout.verticalSpacing = WidgetHelper.DIALOG_SPACING;
      dialogArea.setLayout(layout);
      
      textUrl = new LabeledText(dialogArea, SWT.NONE);
      textUrl.setLabel("URL");
      textUrl.setText(url);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.widthHint = 400;
      textUrl.setLayoutData(gd);
      
      textToken = new LabeledText(dialogArea, SWT.NONE);
      textToken.setLabel("Authentication token");
      textToken.setText(token);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textToken.setLayoutData(gd);
      
      textDescription = new LabeledText(dialogArea, SWT.NONE);
      textDescription.setLabel("Description");
      textDescription.setText(description);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      textDescription.setLayoutData(gd);
      
      return dialogArea;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.dialogs.Dialog#okPressed()
    */
   @Override
   protected void okPressed()
   {
      url = textUrl.getText().trim();
      token = textToken.getText().trim();
      description = textDescription.getText().trim();
      super.okPressed();
   }

   /**
    * @return the url
    */
   public String getUrl()
   {
      return url;
   }

   /**
    * @return the token
    */
   public String getToken()
   {
      return token;
   }

   /**
    * @return the description
    */
   public String getDescription()
   {
      return description;
   }
}
