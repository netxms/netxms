/**
 * 
 */
package org.netxms.ui.eclipse.usermanager.dialogs;

import java.util.Iterator;

import org.eclipse.jface.dialogs.Dialog;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerFilter;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Shell;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.client.NXCSession;
import org.netxms.client.NXCUser;
import org.netxms.client.NXCUserDBObject;
import org.netxms.ui.eclipse.shared.IUIConstants;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.SortableTableViewer;
import org.netxms.ui.eclipse.usermanager.UserComparator;

/**
 * @author Victor
 *
 */
public class SelectUserDialog extends Dialog
{
	private TableViewer userList;
	private NXCSession session;
	private boolean showGroups;
	private NXCUserDBObject[] selection;
	
	/**
	 * @param parentShell
	 */
	public SelectUserDialog(Shell parentShell, boolean showGroups)
	{
		super(parentShell);
		this.showGroups = showGroups;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.window.Window#configureShell(org.eclipse.swt.widgets.Shell)
	 */
	@Override
	protected void configureShell(Shell newShell)
	{
		newShell.setText("Select users");
		super.configureShell(newShell);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#createDialogArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createDialogArea(Composite parent)
	{
		session  = NXMCSharedData.getInstance().getSession();
		
		Composite dialogArea = (Composite)super.createDialogArea(parent);
		GridLayout layout = new GridLayout();
      layout.marginWidth = IUIConstants.DIALOG_WIDTH_MARGIN;
      layout.marginHeight = IUIConstants.DIALOG_HEIGHT_MARGIN;
      dialogArea.setLayout(layout);
		
		new Label(dialogArea, SWT.NONE).setText("Available users");
		
      final String[] columnNames = { "Login Name" };
      final int[] columnWidths = { 250 };
      userList = new SortableTableViewer(dialogArea, columnNames, columnWidths, 0, SWT.UP,
                                         SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      userList.setContentProvider(new ArrayContentProvider());
      userList.setLabelProvider(new WorkbenchLabelProvider());
      userList.setComparator(new UserComparator());
      userList.addFilter(new ViewerFilter() {
			@Override
			public boolean select(Viewer viewer, Object parentElement, Object element)
			{
				return showGroups || (element instanceof NXCUser);
			}
      });
      userList.setInput(session.getUserDatabaseObjects());
      
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.heightHint = 300;
      userList.getControl().setLayoutData(gd);
      
      return dialogArea;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.dialogs.Dialog#okPressed()
	 */
	@SuppressWarnings("unchecked")
	@Override
	protected void okPressed()
	{
		IStructuredSelection sel = (IStructuredSelection)userList.getSelection();
		if (sel.size() == 0)
		{
			MessageDialog.openWarning(getShell(), "Warning", "You must select at least one user from list and then press OK.");
			return;
		}
		selection = new NXCUserDBObject[sel.size()];
		Iterator<NXCUserDBObject> it = sel.iterator();
		for(int i = 0; it.hasNext(); i++)
		{
			selection[i] = it.next();
		}
		super.okPressed();
	}

	/**
	 * @return the selection
	 */
	public NXCUserDBObject[] getSelection()
	{
		return selection;
	}
}
