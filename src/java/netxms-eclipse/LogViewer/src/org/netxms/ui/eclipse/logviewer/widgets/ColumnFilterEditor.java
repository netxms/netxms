/**
 * 
 */
package org.netxms.ui.eclipse.logviewer.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.netxms.client.log.ColumnFilter;
import org.netxms.client.log.LogColumn;
import org.netxms.ui.eclipse.logviewer.widgets.helpers.FilterTreeElement;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.widgets.DashboardComposite;

/**
 * Widget for editing filter for single column
 */
public class ColumnFilterEditor extends DashboardComposite
{
	private LogColumn column;
	private FilterTreeElement columnElement;
	
	/**
	 * @param parent
	 * @param style
	 * @param columnElement 
	 * @param column 
	 */
	public ColumnFilterEditor(Composite parent, int style, LogColumn column, FilterTreeElement columnElement, final Runnable deleteHandler)
	{
		super(parent, style);
		
		this.column = column;
		this.columnElement = columnElement;
		
		GridLayout layout = new GridLayout();
		setLayout(layout);
		
		final Composite header = new Composite(this, SWT.NONE);
		layout = new GridLayout();
		layout.marginWidth = 0;
		layout.marginHeight = 0;
		layout.numColumns = 3;
		header.setLayout(layout);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		header.setLayoutData(gd);
		
		final Label title = new Label(header, SWT.NONE);
		title.setText(column.getDescription());
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		title.setLayoutData(gd);
		
		ImageHyperlink link = new ImageHyperlink(header, SWT.NONE);
		link.setImage(SharedIcons.IMG_ADD_OBJECT);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addElement(ColumnFilterEditor.this.columnElement);
			}
		});

		link = new ImageHyperlink(header, SWT.NONE);
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				deleteHandler.run();
			}
		});
	}
	
	/**
	 * Add new filter element
	 */
	private void addElement(FilterTreeElement currentElement)
	{
		boolean isGroup = false;
		if (currentElement.getType() == FilterTreeElement.FILTER)
		{
			if (((ColumnFilter)currentElement.getObject()).getType() == ColumnFilter.SET)
				isGroup = true;
		}
		if (currentElement.hasChilds() && !isGroup)
		{
			addGroup(currentElement);
		}
		else
		{
			addCondition(currentElement);
		}
	}
	
	private void addGroup(FilterTreeElement currentElement)
	{
		
	}
	
	private void addCondition(FilterTreeElement currentElement)
	{
		switch(column.getType())
		{
			case LogColumn.LC_OBJECT_ID:
				break;
			default:
				break;
		}
	}
}
