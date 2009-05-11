/**
 * 
 */
package org.netxms.ui.eclipse.objectbrowser;

import java.util.ArrayList;
import java.util.List;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.IDialogSettings;
import org.eclipse.jface.window.IShellProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.swt.widgets.Tree;
import org.eclipse.swt.widgets.TreeItem;

/**
 * @author Victor
 * 
 */
public class ObjectSelectionDialog extends Dialog
{
	private ObjectTree objectTree;
	private ObjectList objectList;
	protected CTabFolder tabFolder;

	private boolean treeActive = true;

	/**
	 * @param parentShell
	 */
	public ObjectSelectionDialog(Shell parentShell)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
	}

	/**
	 * @param parentShell
	 */
	public ObjectSelectionDialog(IShellProvider parentShell)
	{
		super(parentShell);
		setShellStyle(getShellStyle() | SWT.RESIZE);
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets
	 * .Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		super.configureShell(newShell);
		newShell.setText("Select Object");
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		try
		{
			newShell.setSize(settings.getInt("SelectObject.cx"), settings.getInt("SelectObject.cy"));
		}
		catch(NumberFormatException e)
		{
			newShell.setSize(400, 350);
		}
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see
	 * org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets
	 * .Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		IDialogSettings settings = Activator.getDefault().getDialogSettings();
		Composite dialogArea = (Composite) super.createDialogArea(parent);

		dialogArea.setLayout(new FormLayout());

		tabFolder = new CTabFolder(dialogArea, SWT.BOTTOM | SWT.FLAT | SWT.MULTI);

		// Object tree
		objectTree = new ObjectTree(tabFolder, SWT.NONE, ObjectTree.CHECKBOXES);
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
		tabFolder.setLayoutData(fd);

		// Object list
		objectList = new ObjectList(tabFolder, SWT.NONE, ObjectList.CHECKBOXES);
		tabItem = new CTabItem(tabFolder, SWT.NONE);
		tabItem.setText("List");
		tabItem.setControl(objectList);
		if (text != null)
			objectList.setFilter(text);

		return dialogArea;
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#cancelPressed()
	 */
	@Override
	protected void cancelPressed()
	{
		saveSettings();
		super.cancelPressed();
	}

	/*
	 * (non-Javadoc)
	 * 
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@Override
	protected void okPressed()
	{
		final Control control = tabFolder.getSelection().getControl();

		if (control == objectTree)
		{
			treeActive = true;
			final Tree tree = objectTree.getTreeControl();
		}
		else if (control == objectList)
		{
			treeActive = false;
		}

		saveSettings();
		super.okPressed();
	}

	/**
	 * Save dialog settings
	 */
	private void saveSettings()
	{
		Point size = getShell().getSize();
		IDialogSettings settings = Activator.getDefault().getDialogSettings();

		settings.put("SelectObject.cx", size.x);
		settings.put("SelectObject.cy", size.y);
		settings.put("SelectObject.Filter", objectList.getFilter());
	}

	/**
	 * TODO: REWRITE FROM SCRATCH
	 * 
	 * @return
	 */
	public List<Long> getSelectedObjects()
	{
		List<Long> ret = new ArrayList<Long>(0);

		if (treeActive)
		{
			final Tree tree = objectTree.getTreeControl();
			final TreeItem[] selection = tree.getSelection();
			for(int i = 0; i < selection.length; i++)
			{
				TreeItem item = selection[i];
				ret.add(item.getText);
			}
		}
		else
		{
			ret.add(2L);
		}

		return ret;
	}
}
