/**
 * 
 */
package org.netxms.ui.eclipse.console;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.resource.ImageDescriptor;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Monitor;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Text;
import org.netxms.ui.eclipse.shared.IUIConstants;

/**
 * @author Victor
 *
 */
public class LoginDialog extends Dialog
{
	private ImageDescriptor loginImage;
	private Button checkBoxEncrypt;
	private Button checkBoxDontCache;
	private Button checkBoxClearCache;
	private Button checkBoxMatchVersion;
	private Combo comboServer;
	private Text textLogin;
	private Text textPassword;
	private String password;
	private boolean isOk = false;
	
	protected LoginDialog(Shell parentShell)
	{
		super(parentShell);
		loginImage = Activator.getImageDescriptor(Messages.getString("LoginDialog.image_name")); //$NON-NLS-1$
	}

	
	@Override
	protected void configureShell(Shell newShell)
	{
      super.configureShell(newShell);
      newShell.setText(Messages.getString("LoginDialog.title")); //$NON-NLS-1$
      
      // Center dialog on screen
      // We don't have main window at this moment, so use
      // monitor data to determine right position
      Monitor [] ma = newShell.getDisplay().getMonitors();
      if (ma != null)
      	newShell.setLocation((ma[0].getClientArea().width - newShell.getSize().x) / 2, (ma[0].getClientArea().height - newShell.getSize().y) / 2);   
	}

	
	@Override
	protected Control createDialogArea(Composite parent) 
	{
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      Composite dialogArea = (Composite)super.createDialogArea(parent);
      
      RowLayout rowLayout = new RowLayout();
      rowLayout.type = SWT.VERTICAL;
      rowLayout.marginWidth = IUIConstants.DIALOG_WIDTH_MARGIN;
      rowLayout.marginHeight = IUIConstants.DIALOG_HEIGHT_MARGIN;
      rowLayout.fill = true;
      dialogArea.setLayout(rowLayout);
      
      // Header image
      Label label = new Label(dialogArea, 0);
      label.setImage(loginImage.createImage());
      label.addDisposeListener(
      		new DisposeListener()
      		{
      			public void widgetDisposed(DisposeEvent event)
      			{
      				((Label)event.widget).getImage().dispose();
      			}
      		}
      		);
      
      Composite fields = new Composite(dialogArea, SWT.NONE);
      FormLayout formLayout = new FormLayout();
      formLayout.spacing = IUIConstants.DIALOG_SPACING;
      fields.setLayout(formLayout);
      
      // Connection
      Group groupConn = new Group(fields, SWT.SHADOW_ETCHED_IN);
      groupConn.setText(Messages.getString("LoginDialog.connection")); //$NON-NLS-1$
      GridLayout gridLayout = new GridLayout(2, false);
      gridLayout.marginWidth = IUIConstants.DIALOG_WIDTH_MARGIN;
      gridLayout.marginHeight = IUIConstants.DIALOG_HEIGHT_MARGIN;
      groupConn.setLayout(gridLayout);

      label = new Label(groupConn, SWT.NONE);
      label.setText(Messages.getString("LoginDialog.server")); //$NON-NLS-1$
      comboServer = new Combo(groupConn, SWT.DROP_DOWN);
      String[] items = settings.getArray("Connect.ServerHistory"); //$NON-NLS-1$
      if (items != null)
      	comboServer.setItems(items);
      String text = settings.get("Connect.Server"); //$NON-NLS-1$
      if (text != null)
      	comboServer.setText(text);
      GridData gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      comboServer.setLayoutData(gridData);
      
      label = new Label(groupConn, SWT.NONE);
      label.setText(Messages.getString("LoginDialog.login")); //$NON-NLS-1$
      textLogin = new Text(groupConn, SWT.SINGLE | SWT.BORDER);
      text = settings.get("Connect.Login"); //$NON-NLS-1$
      if (text != null)
      	textLogin.setText(text);
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      textLogin.setLayoutData(gridData);
      
      label = new Label(groupConn, SWT.NONE);
      label.setText(Messages.getString("LoginDialog.password")); //$NON-NLS-1$
      textPassword = new Text(groupConn, SWT.SINGLE | SWT.PASSWORD | SWT.BORDER);
      gridData = new GridData();
      gridData.horizontalAlignment = GridData.FILL;
      gridData.grabExcessHorizontalSpace = true;
      textPassword.setLayoutData(gridData);
      
      // Options
      Group groupOpts = new Group(fields, SWT.SHADOW_ETCHED_IN);
      groupOpts.setText(Messages.getString("LoginDialog.options")); //$NON-NLS-1$
      rowLayout = new RowLayout();
      rowLayout.type = SWT.VERTICAL;
      //rowLayout.justify = true;
      rowLayout.spacing = 4;
      groupOpts.setLayout(rowLayout);

      checkBoxEncrypt = new Button(groupOpts, SWT.CHECK);
      checkBoxEncrypt.setText(Messages.getString("LoginDialog.opt_encrypt")); //$NON-NLS-1$
      checkBoxEncrypt.setSelection(settings.getBoolean("Connect.Encrypt")); //$NON-NLS-1$
      
      checkBoxClearCache = new Button(groupOpts, SWT.CHECK);
      checkBoxClearCache.setText(Messages.getString("LoginDialog.opt_clear_cache")); //$NON-NLS-1$
      checkBoxClearCache.setSelection(settings.getBoolean("Connect.ClearCache")); //$NON-NLS-1$
      
      checkBoxDontCache = new Button(groupOpts, SWT.CHECK);
      checkBoxDontCache.setText(Messages.getString("LoginDialog.opt_dont_cache")); //$NON-NLS-1$
      checkBoxDontCache.setSelection(settings.getBoolean("Connect.DontCache")); //$NON-NLS-1$
      
      checkBoxMatchVersion = new Button(groupOpts, SWT.CHECK);
      checkBoxMatchVersion.setText(Messages.getString("LoginDialog.opt_version_match")); //$NON-NLS-1$
      checkBoxMatchVersion.setSelection(settings.getBoolean("Connect.MatchVersion")); //$NON-NLS-1$

      FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
      fd.right = new FormAttachment(groupOpts, 0, SWT.LEFT);
      fd.bottom = new FormAttachment(100, 0);
		groupConn.setLayoutData(fd);   

      fd = new FormData();
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
      fd.bottom = new FormAttachment(100, 0);
		groupOpts.setLayoutData(fd);   
		
      return dialogArea;
   }
	
	@Override
	protected void okPressed() 
	{
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
   
      settings.put("Connect.Server", comboServer.getText()); //$NON-NLS-1$
      settings.put("Connect.ServerHistory", comboServer.getItems()); //$NON-NLS-1$
      settings.put("Connect.Login", textLogin.getText()); //$NON-NLS-1$
      settings.put("Connect.Encrypt", checkBoxEncrypt.getSelection()); //$NON-NLS-1$
      settings.put("Connect.DontCache", checkBoxDontCache.getSelection()); //$NON-NLS-1$
      settings.put("Connect.ClearCache", checkBoxClearCache.getSelection()); //$NON-NLS-1$
      settings.put("Connect.MatchVersion", checkBoxMatchVersion.getSelection()); //$NON-NLS-1$
      
      password = textPassword.getText();
  
      isOk = true;
      
      super.okPressed();
   }


	/**
	 * @return the password
	 */
	public String getPassword()
	{
		return password;
	}

	
	/**
	 * @return true if dialog was closed with OK button, false otherwise
	 */
	public boolean isOk()
	{
		return isOk;
	}
}
