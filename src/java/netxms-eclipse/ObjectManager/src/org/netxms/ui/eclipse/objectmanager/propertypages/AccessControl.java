/**
 * 
 */
package org.netxms.ui.eclipse.objectmanager.propertypages;

import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Group;
import org.eclipse.ui.dialogs.PropertyPage;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.netxms.ui.eclipse.objectmanager.AttrListLabelProvider;
import org.netxms.ui.eclipse.objectmanager.AttrViewerComparator;
import org.netxms.ui.eclipse.tools.SortableTableViewer;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * @author Victor
 *
 */
public class AccessControl extends PropertyPage
{
	private SortableTableViewer userList;
	private Button checkRead;
	private Button checkModify;
	private Button checkCreate;
	private Button checkDelete;
	private Button checkControl;
	private Button checkEvents;
	private Button checkViewAlarms;
	private Button checkAckAlarms;
	private Button checkTermAlarms;
	private Button checkPushData;
	private Button checkAccessControl;
	
	/* (non-Javadoc)
	 * @see org.eclipse.jface.preference.PreferencePage#createContents(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createContents(Composite parent)
	{
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
      
      final String[] columnNames = { "Login Name" };
      final int[] columnWidths = { 250 };
      userList = new SortableTableViewer(users, columnNames, columnWidths, 0, SWT.UP,
                                         SWT.BORDER | SWT.MULTI | SWT.FULL_SELECTION);
      userList.setContentProvider(new ArrayContentProvider());
      userList.setLabelProvider(new WorkbenchLabelProvider());
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
      
      Button addButton = new Button(buttons, SWT.PUSH);
      addButton.setText("Add...");

      Button deleteButton = new Button(buttons, SWT.PUSH);
      deleteButton.setText("Delete");
      
      Group rights = new Group(dialogArea, SWT.NONE);
      rights.setText("Access Rights");
      rights.setLayout(new RowLayout(SWT.VERTICAL));
      gd = new GridData();
      gd.grabExcessVerticalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      rights.setLayoutData(gd);
      
      checkRead = new Button(rights, SWT.CHECK);
      checkRead.setText("&Read");
      
      checkModify = new Button(rights, SWT.CHECK);
		checkModify.setText("&Modify");
      
      checkCreate = new Button(rights, SWT.CHECK);
		checkCreate.setText("&Create child objects");
      
      checkDelete = new Button(rights, SWT.CHECK);
		checkDelete.setText("&Delete");
      
      checkControl = new Button(rights, SWT.CHECK);
		checkControl.setText("C&ontrol");
      
      checkEvents = new Button(rights, SWT.CHECK);
		checkEvents.setText("Send &events");
      
      checkViewAlarms = new Button(rights, SWT.CHECK);
		checkViewAlarms.setText("&View alarms");
      
      checkAckAlarms = new Button(rights, SWT.CHECK);
		checkAckAlarms.setText("Ac&knowledge alarms");
      
      checkTermAlarms = new Button(rights, SWT.CHECK);
		checkTermAlarms.setText("&Terminate alarms");
      
      checkPushData = new Button(rights, SWT.CHECK);
		checkPushData.setText("&Push data");
      
      checkAccessControl = new Button(rights, SWT.CHECK);
      checkAccessControl.setText("&Access control");
      
		return dialogArea;
	}
}
