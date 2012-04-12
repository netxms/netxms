package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.shared.SharedColors;
import org.netxms.ui.eclipse.tools.WidgetHelper;

public abstract class TableElement extends OverviewPageElement
{
	private static final long serialVersionUID = 1L;

	private Table table;
	private Action actionCopy;
	private Action actionCopyName;
	private Action actionCopyValue;

	/**
	 * @param parent
	 * @param object
	 */
	public TableElement(Composite parent, GenericObject object)
	{
		super(parent, object);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.widgets.DashboardElement#createClientArea(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected Control createClientArea(Composite parent)
	{
		table = new Table(parent, SWT.FULL_SELECTION | SWT.HIDE_SELECTION | SWT.SINGLE | SWT.H_SCROLL);
		setupTable();
		createActions();
		createPopupMenu();
		fillTableInternal();
		return table;
	}

	/**
	 * Setup table
	 */
	private void setupTable()
	{
		TableColumn tc = new TableColumn(table, SWT.LEFT);
		tc.setText("Name");
		tc = new TableColumn(table, SWT.LEFT);
		tc.setText("Value");
		
		table.setHeaderVisible(false);
		table.setLinesVisible(false);
		table.setBackground(SharedColors.WHITE);
	}
	
	/**
	 * Create actions required for popup menu
	 */
	protected void createActions()
	{
		actionCopy = new Action("Copy to clipboard") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				int index = table.getSelectionIndex();
				if (index >= 0)
				{
					final TableItem item = table.getItem(index);
					WidgetHelper.copyToClipboard(item.getText(0) + "=" + item.getText(1));
				}
			}
		};

		actionCopyName = new Action("Copy &name to clipboard") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				int index = table.getSelectionIndex();
				if (index >= 0)
				{
					final TableItem item = table.getItem(index);
					WidgetHelper.copyToClipboard(item.getText(0));
				}
			}
		};

		actionCopyValue = new Action("Copy &value to clipboard") {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				int index = table.getSelectionIndex();
				if (index >= 0)
				{
					final TableItem item = table.getItem(index);
					WidgetHelper.copyToClipboard(item.getText(1));
				}
			}
		};
	}
	
	/**
	 * Create popup menu
	 */
	private void createPopupMenu()
	{
		MenuManager menuMgr = new MenuManager();
		menuMgr.setRemoveAllWhenShown(true);
		menuMgr.addMenuListener(new IMenuListener() {
			private static final long serialVersionUID = 1L;

			public void menuAboutToShow(IMenuManager mgr)
			{
				fillContextMenu(mgr);
			}
		});

		Menu menu = menuMgr.createContextMenu(table);
		table.setMenu(menu);
	}
	
	/**
	 * Fill item's context menu
	 * 
	 * @param manager menu manager
	 */
	protected void fillContextMenu(IMenuManager manager)
	{
		manager.add(actionCopy);
		manager.add(actionCopyName);
		manager.add(actionCopyValue);
	}

	/**
	 * Fill table
	 */
	abstract protected void fillTable();
	
	/**
	 * Fill table - internal wrapper
	 */
	private void fillTableInternal()
	{
		if (getObject() == null)
			return;
		
		fillTable();
		table.getColumn(0).pack();
		table.getColumn(1).pack();
	}
	
	/**
	 * Add attribute/value pair to list
	 * @param attr
	 * @param value
	 * @param allowEmpty if set to false, empty values will not be added
	 */
	protected void addPair(String attr, String value, boolean allowEmpty)
	{
		if (!allowEmpty && ((value == null) || value.isEmpty()))
			return;
		TableItem item = new TableItem(table, SWT.NONE);
		item.setText(0, attr);
		item.setText(1, (value != null) ? value : "");
	}

	/**
	 * Add attribute/value pair to list
	 * @param attr
	 * @param value
	 */
	protected void addPair(String attr, String value)
	{
		addPair(attr, value, true);
	}

	/* (non-Javadoc)
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement#onObjectChange()
	 */
	@Override
	protected void onObjectChange()
	{
		table.removeAll();
		fillTableInternal();
	}
}
