package org.netxms.ui.eclipse.objectview.objecttabs.elements;

import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Event;
import org.eclipse.swt.widgets.Listener;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.TableItem;
import org.netxms.client.objects.GenericObject;

public abstract class TableElement extends OverviewPageElement
{
	private static final Color BACKGROUND_COLOR = new Color(Display.getDefault(), 255, 255, 255);
	
	private Table table;

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
		table = new Table(parent, SWT.FULL_SELECTION | SWT.HIDE_SELECTION | SWT.SINGLE);
		setupTable();
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
		table.setBackground(BACKGROUND_COLOR);

		// On Windows Vista and Windows 7, Table widget draws
		// vertical lines even when setLinesVisible set to false.
		// To avoid it, we use custom drawn background. As side effect,
		// selection highlight also removed.
		table.addListener(SWT.EraseItem, new Listener() {
			@Override
			public void handleEvent(Event event)
			{
				GC gc = event.gc;
				gc.setBackground(BACKGROUND_COLOR);
				gc.fillRectangle(event.x, event.y, event.width, event.height);
			}
		});
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
		if (!allowEmpty && value.isEmpty())
			return;
		TableItem item = new TableItem(table, SWT.NONE);
		item.setText(0, attr);
		item.setText(1, value);
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
