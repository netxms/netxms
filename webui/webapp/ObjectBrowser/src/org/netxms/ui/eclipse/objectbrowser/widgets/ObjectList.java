/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.ui.eclipse.objectbrowser.widgets;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.core.runtime.IStatus;
import org.eclipse.core.runtime.Status;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FormAttachment;
import org.eclipse.swt.layout.FormData;
import org.eclipse.swt.layout.FormLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.progress.UIJob;
import org.netxms.api.client.SessionNotification;
import org.netxms.client.NXCListener;
import org.netxms.client.NXCNotification;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectListFilter;
import org.netxms.ui.eclipse.objectbrowser.widgets.internal.ObjectTreeComparator;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Object list widget
 *
 */
public class ObjectList extends Composite
{
	private static final long serialVersionUID = 1L;

	// Options
	public static final int NONE = 0;
	public static final int CHECKBOXES = 0x01;

	private ObjectListFilter filter;
	private boolean filterEnabled = true;
	private TableViewer objectList;
	private Composite filterArea;
	private Label filterLabel;
	private Text filterText;
	private NXCListener sessionListener = null;
	private NXCSession session = null;
	private int changeCount = 0;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ObjectList(Composite parent, int style, int options)
	{
		super(parent, style);

		session = (NXCSession)ConsoleSharedData.getSession();

		FormLayout formLayout = new FormLayout();
		setLayout(formLayout);
		
		objectList = new TableViewer(this, SWT.SINGLE | SWT.FULL_SELECTION | (((options & CHECKBOXES) == CHECKBOXES) ? SWT.CHECK : 0));
		objectList.setContentProvider(new ArrayContentProvider());
		objectList.setLabelProvider(new WorkbenchLabelProvider());
		objectList.setComparator(new ObjectTreeComparator());
		filter = new ObjectListFilter();
		objectList.addFilter(filter);
		objectList.setInput(session.getAllObjects());
		
		filterArea = new Composite(this, SWT.NONE);
		
		filterLabel = new Label(filterArea, SWT.NONE);
		filterLabel.setText("Filter: ");
		
		filterText = new Text(filterArea, SWT.BORDER);
		filterText.setLayoutData(new GridData(GridData.FILL_HORIZONTAL));
		filterText.addModifyListener(new ModifyListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void modifyText(ModifyEvent e)
			{
				filter.setFilterString(filterText.getText());
				objectList.refresh(false);
			}
		});
		
		filterArea.setLayout(new GridLayout(2, false));
		
		FormData fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(filterArea);
		fd.right = new FormAttachment(100, 0);
		fd.bottom = new FormAttachment(100, 0);
		objectList.getControl().setLayoutData(fd);
		
		fd = new FormData();
		fd.left = new FormAttachment(0, 0);
		fd.top = new FormAttachment(0, 0);
		fd.right = new FormAttachment(100, 0);
		filterArea.setLayoutData(fd);
		
		// Add client library listener
		sessionListener = new NXCListener() {
			@Override
			public void notificationHandler(SessionNotification n)
			{
				if ((n.getCode() == NXCNotification.OBJECT_CHANGED) || (n.getCode() == NXCNotification.OBJECT_DELETED))
				{
					changeCount++;
					new UIJob(getDisplay(), "Update object list") {
						@Override
						public IStatus runInUIThread(IProgressMonitor monitor)
						{
							if (ObjectList.this.isDisposed() || objectList.getControl().isDisposed())
								return Status.OK_STATUS;
							
							changeCount--;
							if (changeCount <= 0)
							{
								objectList.setInput(session.getAllObjects());
							}
							return Status.OK_STATUS;
						}
					}.schedule(500);
				}
			}
		};
		session.addListener(sessionListener);

		// Set dispose listener
		addDisposeListener(new DisposeListener() {
			private static final long serialVersionUID = 1L;

			@Override
			public void widgetDisposed(DisposeEvent e)
			{
				if ((session != null) && (sessionListener != null))
					session.removeListener(sessionListener);
			}
		});
		
		// Set initial focus to filter input line
		filterText.setFocus();
	}
	
	/**
	 * Enable or disable filter
	 * 
	 * @param enable New filter state
	 */
	public void enableFilter(boolean enable)
	{
		filterEnabled = enable;
		filterArea.setVisible(filterEnabled);
		FormData fd = (FormData)objectList.getControl().getLayoutData();
		fd.top = enable ? new FormAttachment(filterArea) : new FormAttachment(0, 0);
		layout();
	}

	/**
	 * @return the filterEnabled
	 */
	public boolean isFilterEnabled()
	{
		return filterEnabled;
	}
	
	/**
	 * Set filter text
	 * 
	 * @param text New filter text
	 */
	public void setFilter(final String text)
	{
		filterText.setText(text);
	}
	
	/**
	 * Get filter text
	 * 
	 * @return Current filter text
	 */
	public String getFilter()
	{
		return filterText.getText();
	}
	
	/**
	 * Get underlying table control
	 * 
	 * @return Underlying table control
	 */
	public Control getTableControl()
	{
		return objectList.getControl();
	}
	
	/**
	 * Get underlying table viewer
	 *
	 * @return Underlying table viewer
	 */
	public TableViewer getTableViewer()
	{
		return objectList;
	}
}
