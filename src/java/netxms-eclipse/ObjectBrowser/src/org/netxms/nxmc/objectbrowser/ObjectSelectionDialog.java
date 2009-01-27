/**
 * 
 */
package org.netxms.nxmc.objectbrowser;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.window.IShellProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.TabFolder;

/**
 * @author Victor
 * 
 */
public class ObjectSelectionDialog extends Dialog
{
	private ObjectTree objectTree;
	private ObjectList objectList;
	protected CTabFolder tabFolder;

	/**
	 * @param parentShell
	 */
	public ObjectSelectionDialog(Shell parentShell)
	{
		super(parentShell);
	}

	/**
	 * @param parentShell
	 */
	public ObjectSelectionDialog(IShellProvider parentShell)
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
      newShell.setText("Select Object");
      //newShell.setMinimumSize(250, 350);
      //newShell.setSize(400, 350);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
      IDialogSettings settings = Activator.getDefault().getDialogSettings();
      Composite dialogArea = (Composite)super.createDialogArea(parent);

      dialogArea.setLayout(new FormLayout());
      
      tabFolder = new CTabFolder(dialogArea, SWT.BOTTOM | SWT.FLAT | SWT.MULTI);

      // Object tree
      objectTree = new ObjectTree(tabFolder, SWT.NONE);
      CTabItem tabItem = new CTabItem(tabFolder, SWT.NONE);
      tabItem.setText("Tree");
      tabItem.setControl(objectTree);
      String text = settings.get("SelectObject.Filter");
      if (text != null)
      	objectTree.setFilter(text);
      
      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		fd.height = 450;
		tabFolder.setLayoutData(fd);
		
		// Object list
		objectList = new ObjectList(tabFolder, SWT.NONE);
      tabItem = new CTabItem(tabFolder, SWT.NONE);
      tabItem.setText("List");
		tabItem.setControl(objectList);
      if (text != null)
      	objectList.setFilter(text);

/*      FormData fd = new FormData();
      fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		objectList.setLayoutData(fd);*/
      return dialogArea;
	}
}
