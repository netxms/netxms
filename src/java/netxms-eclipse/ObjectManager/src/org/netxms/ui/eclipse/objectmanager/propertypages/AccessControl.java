/**
 * 
 */
package org.netxms.ui.eclipse.objectmanager.propertypages;

import java.util.HashMap;
import java.util.Iterator;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Platform;
import org.eclipse.core.runtime.Status;
import org.eclipse.core.runtime.jobs.Job;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.ISelectionChangedListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.SelectionChangedEvent;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.progress.UIJob;
import org.netxms.client.AccessListElement;
import org.netxms.client.NXCException;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCUserDBObject;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.objectmanager.AccessListComparator;
import org.netxms.ui.eclipse.objectmanager.AccessListLabelProvider;
import org.netxms.ui.eclipse.objectmanager.Activator;
import org.netxms.ui.eclipse.shared.NXMCSharedData;
import org.netxms.ui.eclipse.tools.SortableTableViewer;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.dialogs.SelectUserDialog;

/**
 * @author Victor
 *
 */
public class AccessControl extends PropertyPage
{
	private GenericObject object;
	private SortableTableViewer userList;
	private HashMap<Integer, Button> accessChecks = new HashMap<Integer, Button>(11);
	private HashMap<Long, AccessListElement> acl;
	private Button checkInherit;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
		object = (GenericObject)getElement().getAdapter(GenericObject.class);
		
		AccessListElement[] origAcl = object.getAccessList();
		acl = new HashMap<Long, AccessListElement>(origAcl.length);
		for(int i = 0; i < origAcl.length; i++)
			acl.put(origAcl[i].getUserId(), new AccessListElement(origAcl[i]));
		
		// Initiate loading of user manager plugin if it was not loaded before
		Platform.getAdapterManager().loadAdapter(new AccessListElement(0, 0), "org.eclipse.ui.model.IWorkbenchAdapter");
		
		Composite dialogArea = new Composite(parent, SWT.NONE);
		
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.numColumns = 2;
		dialogArea.setLayout(layout);
		
		Group users = new Group(dialogArea, SWT.NONE);
		users.setText("Users and Groups");
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      users.setLayoutData(gd);

		layout = new GridLayout();
		users.setLayout(layout);
      
      final String[] columnNames = { "Login Name", "Rights" };
      final int[] columnWidths = { 150, 100 };
      userList = new SortableTableViewer(users, columnNames, columnWidths, 0, SWT.UP,
                                         SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      userList.setContentProvider(new ArrayContentProvider());
      userList.setLabelProvider(new AccessListLabelProvider());
      userList.setComparator(new AccessListComparator());
      userList.setInput(acl.values().toArray());
      gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      userList.getControl().setLayoutData(gd);
      
      Composite buttons = new Composite(users, SWT.NONE);
      FillLayout buttonsLayout = new FillLayout();
      buttonsLayout.spacing = WidgetHelper.INNER_SPACING;
      buttons.setLayout(buttonsLayout);
      gd = new GridData();
      gd.horizontalAlignment = SWT.RIGHT;
      gd.widthHint = 184;
      buttons.setLayoutData(gd);
      
      final Button addButton = new Button(buttons, SWT.PUSH);
      addButton.setText("Add...");
      addButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				SelectUserDialog dlg = new SelectUserDialog(AccessControl.this.getShell(), true);
				if (dlg.open() == Window.OK)
				{
					NXCUserDBObject[] selection = dlg.getSelection();
					for(NXCUserDBObject user : selection)
						acl.put(user.getId(), new AccessListElement(user.getId(), 0));
					userList.setInput(acl.values().toArray());
				}
			}
      });

      final Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText("Delete");
      deleteButton.setEnabled(false);
      deleteButton.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@SuppressWarnings("unchecked")
			@Override
			public void widgetSelected(SelectionEvent e)
			{
				IStructuredSelection sel = (IStructuredSelection)userList.getSelection();
				Iterator<AccessListElement> it = sel.iterator();
				while(it.hasNext())
				{
					AccessListElement element = it.next();
					acl.remove(element.getUserId());
				}
				userList.setInput(acl.values().toArray());
			}
      });
      
      Group rights = new Group(dialogArea, SWT.NONE);
      rights.setText("Access Rights");
      rights.setLayout(new RowLayout(SWT.VERTICAL));
      gd = new GridData();
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      rights.setLayoutData(gd);
      
      createAccessCheck(rights, "&Read", NXCUserDBObject.OBJECT_ACCESS_READ);
      createAccessCheck(rights, "&Modify", NXCUserDBObject.OBJECT_ACCESS_MODIFY);
      createAccessCheck(rights, "&Create child objects", NXCUserDBObject.OBJECT_ACCESS_CREATE);
      createAccessCheck(rights, "&Delete", NXCUserDBObject.OBJECT_ACCESS_DELETE);
      createAccessCheck(rights, "C&ontrol", NXCUserDBObject.OBJECT_ACCESS_CONTROL);
      createAccessCheck(rights, "Send &events", NXCUserDBObject.OBJECT_ACCESS_SEND_EVENTS);
      createAccessCheck(rights, "&View alarms", NXCUserDBObject.OBJECT_ACCESS_READ_ALARMS);
      createAccessCheck(rights, "Ac&knowledge alarms", NXCUserDBObject.OBJECT_ACCESS_ACK_ALARMS);
      createAccessCheck(rights, "&Terminate alarms", NXCUserDBObject.OBJECT_ACCESS_TERM_ALARMS);
      createAccessCheck(rights, "&Push data", NXCUserDBObject.OBJECT_ACCESS_PUSH_DATA);
      createAccessCheck(rights, "&Access control", NXCUserDBObject.OBJECT_ACCESS_ACL);
      
      userList.addSelectionChangedListener(new ISelectionChangedListener() {
			@Override
			public void selectionChanged(SelectionChangedEvent event)
			{
				IStructuredSelection sel = (IStructuredSelection)event.getSelection();
				if (sel.size() == 1)
				{
					enableAllChecks(true);
					AccessListElement element = (AccessListElement)sel.getFirstElement();
					int rights = element.getAccessRights();
					for(int i = 0, mask = 1; i < 16; i++, mask <<= 1)
					{
						Button check = accessChecks.get(mask);
						if (check != null)
						{
							check.setSelection((rights & mask) == mask);
						}
					}
				}
				else
				{
					enableAllChecks(false);
				}
				deleteButton.setEnabled(sel.size() > 0);
			}
      });
      
      checkInherit = new Button(dialogArea, SWT.CHECK);
      checkInherit.setText("&Inherit access rights from parent object(s)");
      checkInherit.setSelection(object.isInheritAccessRights());
      
		return dialogArea;
	}
	
	/**
	 * Create access control check box.
	 * 
	 * @param parent Parent composite
	 * @param name Name of the access right
	 * @param bitMask Bit mask for access right
	 */
	private void createAccessCheck(final Composite parent, final String name, final Integer bitMask)
	{
      final Button check = new Button(parent, SWT.CHECK);
      check.setText(name);
      check.setEnabled(false);
      check.addSelectionListener(new SelectionListener() {
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}

			@Override
			public void widgetSelected(SelectionEvent e)
			{
				IStructuredSelection sel = (IStructuredSelection)userList.getSelection();
				AccessListElement element = (AccessListElement)sel.getFirstElement();
				int rights = element.getAccessRights();
				if (check.getSelection())
					rights |= bitMask;
				else
					rights &= ~bitMask;
				element.setAccessRights(rights);
				userList.update(element, null);
			}
      });
      accessChecks.put(bitMask, check);
	}

	/**
	 * Enables all access check boxes if the argument is true, and disables them otherwise.
	 * 
	 * @param enabled the new enabled state
	 */
	private void enableAllChecks(boolean enabled)
	{
		for(final Button b : accessChecks.values())
		{
			b.setEnabled(enabled);
		}
	}
	
	/**
	 * Apply changes
	 * 
	 * @param isApply true if update operation caused by "Apply" button
	 */
	protected void applyChanges(final boolean isApply)
	{
		if (isApply)
			setValid(false);
		
		final boolean inheritAccessRights = checkInherit.getSelection();
		
		new Job("Update access control list for object " + object.getObjectName()) {
			@Override
			protected IStatus run(IProgressMonitor monitor)
			{
				IStatus status;
				
				try
				{
					NXCObjectModificationData md = new NXCObjectModificationData(object.getObjectId());
					md.setACL(acl.values().toArray(new AccessListElement[acl.size()]));
					md.setInheritAccessRights(inheritAccessRights);
					NXMCSharedData.getInstance().getSession().modifyObject(md);
					status = Status.OK_STATUS;
				}
				catch(Exception e)
				{
					status = new Status(Status.ERROR, Activator.PLUGIN_ID, 
					                    (e instanceof NXCException) ? ((NXCException)e).getErrorCode() : 0,
					                    "Cannot change access control list: " + e.getMessage(), null);
				}

				if (isApply)
				{
					new UIJob("Update \"Access Control\" property page") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							AccessControl.this.setValid(true);
							return Status.OK_STATUS;
						}
					}.schedule();
				}

				return status;
			}
		}.schedule();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performOk()
	 */
	@Override
	public boolean performOk()
	{
		applyChanges(false);
		return true;
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performApply()
	 */
	@Override
	protected void performApply()
	{
		applyChanges(true);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#performDefaults()
	 */
	@Override
	protected void performDefaults()
	{
		super.performDefaults();
		checkInherit.setSelection(true);
		acl.clear();
		userList.setInput(acl.values().toArray());
	}
}
