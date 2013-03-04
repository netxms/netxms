/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.widgets;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import org.eclipse.core.internal.runtime.AdapterManager;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.ui.IViewPart;
import org.eclipse.ui.internal.dialogs.PropertyDialog;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.ui.eclipse.console.resources.SharedColors;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.Messages;
import org.netxms.ui.eclipse.dashboard.dialogs.EditElementXmlDlg;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementConfig;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardElementLayout;
import org.netxms.ui.eclipse.dashboard.widgets.internal.DashboardModifyListener;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Dashboard rendering control
 */
@SuppressWarnings("restriction")
public class DashboardControl extends Composite
{
	public static final String DEFAULT_CHART_CONFIG = "<element>\n\t<showIn3D>true</showIn3D>\n\t<dciList length=\"0\">\n\t</dciList>\n</element>";  //$NON-NLS-1$
	public static final String DEFAULT_LINE_CHART_CONFIG = "<element>\n\t<dciList length=\"0\">\n\t</dciList>\n</element>"; //$NON-NLS-1$
	public static final String DEFAULT_DIAL_CHART_CONFIG = "<element>\n\t<maxValue>100</maxValue>\n\t<yellowZone>70</yellowZone>\n\t<redZone>90</redZone>\n\t<dciList length=\"0\">\n\t</dciList>\n</element>";  //$NON-NLS-1$
	public static final String DEFAULT_AVAILABILITY_CHART_CONFIG = "<element>\n\t<objectId>9</objectId>\n\t<showIn3D>true</showIn3D>\n</element>"; //$NON-NLS-1$
	public static final String DEFAULT_TABLE_CHART_CONFIG = "<element>\n\t<showIn3D>true</showIn3D>\n\t<nodeId>0</nodeId>\n\t<dciId>0</dciId>\n\t<dataColumn>DATA</dataColumn>\n</element>";  //$NON-NLS-1$
	public static final String DEFAULT_OBJECT_REFERENCE_CONFIG = "<element>\n\t<objectId>0</objectId>\n</element>"; //$NON-NLS-1$
	public static final String DEFAULT_LABEL_CONFIG = "<element>\n\t<title>Label</title>\n</element>";  //$NON-NLS-1$
	public static final String DEFAULT_NETWORK_MAP_CONFIG = "<element>\n\t<objectId>0</objectId>\n\t<title></title>\n</element>";  //$NON-NLS-1$
	public static final String DEFAULT_GEO_MAP_CONFIG = "<element>\n\t<latitude>0</latitude>\n\t<longitude>0</longitude>\n\t<zoom>8</zoom>\t<title></title>\n</element>";  //$NON-NLS-1$
	public static final String DEFAULT_WEB_PAGE_CONFIG = "<element>\n\t<url>http://</url>\n\t<title></title>\n</element>";  //$NON-NLS-1$
			
	private static final long serialVersionUID = 1L;

	private Dashboard dashboard;
	private List<DashboardElement> elements;
	private Map<DashboardElement, ElementWidget> elementWidgets = new HashMap<DashboardElement, ElementWidget>();
	private boolean embedded = false;
	private boolean editMode = false;
	private boolean modified = false;
	private DashboardModifyListener modifyListener = null;
	private IViewPart viewPart;
	
	/**
	 * @param parent
	 * @param style
	 */
	public DashboardControl(Composite parent, int style, Dashboard dashboard, IViewPart viewPart, boolean embedded)
	{
		super(parent, style);
		this.dashboard = dashboard;
		this.embedded = embedded;
		this.elements = new ArrayList<DashboardElement>(dashboard.getElements());
		this.viewPart = viewPart;
		createContent();
	}

	/**
	 * @param parent
	 * @param style
	 */
	public DashboardControl(Composite parent, int style, Dashboard dashboard, List<DashboardElement> elements, IViewPart viewPart, boolean modified)
	{
		super(parent, style);
		this.dashboard = dashboard;
		this.embedded = false;
		this.modified = modified;
		this.elements = new ArrayList<DashboardElement>(elements);
		this.viewPart = viewPart;
		createContent();
	}

	/**
	 * Create dashboard's content
	 */
	private void createContent()
	{
		setBackground(SharedColors.getColor(SharedColors.DASHBOARD_BACKGROUND, getDisplay()));
		
		GridLayout layout = new GridLayout();
		layout.numColumns = dashboard.getNumColumns();
		layout.marginWidth = embedded ? 0 : 15;
		layout.marginHeight = embedded ? 0 : 15;
		layout.horizontalSpacing = 10;
		layout.verticalSpacing = 10;
		layout.makeColumnsEqualWidth = (dashboard.getOptions() & Dashboard.EQUAL_WIDTH_COLUMNS) != 0;
		setLayout(layout);
		
		for(final DashboardElement e : elements)
		{
			createElementWidget(e);
		}
	}
	
	/**
	 * Map dashboard element alignment info to SWT
	 * 
	 * @param a
	 * @return
	 */
	static int mapHorizontalAlignment(int a)
	{
		switch(a)
		{
			case DashboardElement.FILL:
				return SWT.FILL;
			case DashboardElement.LEFT:
				return SWT.LEFT;
			case DashboardElement.RIGHT:
				return SWT.RIGHT;
			case DashboardElement.CENTER:
				return SWT.CENTER;
		}
		return 0;
	}

	/**
	 * Map dashboard element alignment info to SWT
	 * 
	 * @param a
	 * @return
	 */
	static int mapVerticalAlignment(int a)
	{
		switch(a)
		{
			case DashboardElement.FILL:
				return SWT.FILL;
			case DashboardElement.TOP:
				return SWT.TOP;
			case DashboardElement.BOTTOM:
				return SWT.BOTTOM;
			case DashboardElement.CENTER:
				return SWT.CENTER;
		}
		return 0;
	}

	/**
	 * Factory method for creating dashboard elements
	 * 
	 * @param e
	 */
	private ElementWidget createElementWidget(DashboardElement e)
	{
		ElementWidget w;
		switch(e.getType())
		{
			case DashboardElement.LINE_CHART:
				w = new LineChartElement(this, e, viewPart);
				break;
			case DashboardElement.BAR_CHART:
				w = new BarChartElement(this, e, viewPart);
				break;
			case DashboardElement.TUBE_CHART:
				w = new TubeChartElement(this, e, viewPart);
				break;
			case DashboardElement.PIE_CHART:
				w = new PieChartElement(this, e, viewPart);
				break;
			case DashboardElement.TABLE_BAR_CHART:
				w = new TableBarChartElement(this, e, viewPart);
				break;
			case DashboardElement.TABLE_PIE_CHART:
				w = new TablePieChartElement(this, e, viewPart);
				break;
			case DashboardElement.TABLE_TUBE_CHART:
				w = new TableTubeChartElement(this, e, viewPart);
				break;
			case DashboardElement.DIAL_CHART:
				w = new DialChartElement(this, e, viewPart);
				break;
			case DashboardElement.STATUS_CHART:
				w = new ObjectStatusChartElement(this, e, viewPart);
				break;
			case DashboardElement.AVAILABLITY_CHART:
				w = new AvailabilityChartElement(this, e, viewPart);
				break;
			case DashboardElement.LABEL:
				w = new LabelElement(this, e, viewPart);
				break;
			case DashboardElement.DASHBOARD:
				w = new EmbeddedDashboardElement(this, e, viewPart);
				break;
			case DashboardElement.NETWORK_MAP:
				w = new NetworkMapElement(this, e, viewPart);
				break;
			case DashboardElement.GEO_MAP:
				w = new GeoMapElement(this, e, viewPart);
				break;
			case DashboardElement.STATUS_INDICATOR:
				w = new StatusIndicatorElement(this, e, viewPart);
				break;
			case DashboardElement.CUSTOM:
				w = new CustomWidgetElement(this, e, viewPart);
				break;
			case DashboardElement.WEB_PAGE:
				w = new WebPageElement(this, e, viewPart);
				break;
			case DashboardElement.ALARM_VIEWER:
				w = new AlarmViewerElement(this, e, viewPart);
				break;
			case DashboardElement.SEPARATOR:
				w = new SeparatorElement(this, e, viewPart);
				break;
			default:
				w = new ElementWidget(this, e, viewPart);
				break;
		}

		final DashboardElementLayout el = w.getElementLayout();
		final GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = (el.horizontalAlignment == DashboardElement.FILL);
		gd.horizontalAlignment = mapHorizontalAlignment(el.horizontalAlignment);
		gd.grabExcessVerticalSpace = (el.vertcalAlignment == DashboardElement.FILL);
		gd.verticalAlignment = mapVerticalAlignment(el.vertcalAlignment);
		gd.horizontalSpan = el.horizontalSpan;
		gd.verticalSpan = el.verticalSpan;
		gd.widthHint = el.widthHint;
		gd.heightHint = el.heightHint;
		w.setLayoutData(gd);
		
		elementWidgets.put(e, w);
		
		return w;
	}
	
	/**
	 * Redo element layout
	 */
	private void redoLayout()
	{
		if (editMode)
		{
			setEditMode(false);
			layout(true, true);
			setEditMode(true);
		}
		else
		{
			layout(true, true);
		}
	}

	/**
	 * @return the editMode
	 */
	public boolean isEditMode()
	{
		return editMode;
	}

	/**
	 * @param editMode the editMode to set
	 */
	public void setEditMode(boolean editMode)
	{
		this.editMode = editMode;
		for(Control c : getChildren())
		{
			if (c instanceof ElementWidget)
			{
				((ElementWidget)c).setEditMode(editMode);
			}
		}
	}
	
	/**
	 * Delete element
	 * 
	 * @param element
	 */
	void deleteElement(DashboardElement element)
	{
		elements.remove(element);
		ElementWidget w = elementWidgets.get(element);
		if (w != null)
		{
			elementWidgets.remove(element);
			w.dispose();
			redoLayout();
		}
		setModified();
	}
	
	/**
	 * Get next widget in order for given widget
	 * 
	 * @param curr
	 * @return
	 */
	private Control getNextWidget(Control curr)
	{
		Control[] children = getChildren();
		for(int i = 0; i < children.length - 1; i++)
			if (children[i] == curr)
				return children[i + 1];
		return null;
	}
	
	/**
	 * Edit element
	 * 
	 * @param element
	 */
	void editElement(DashboardElement element)
	{
		DashboardElementConfig config = (DashboardElementConfig)AdapterManager.getDefault().getAdapter(element, DashboardElementConfig.class);
		if (config != null)
		{
			try
			{
				config.setLayout(DashboardElementLayout.createFromXml(element.getLayout()));
				
				PropertyDialog dlg = PropertyDialog.createDialogOn(getShell(), null, config);
				if (dlg.open() == Window.CANCEL)
					return;	// element creation cancelled
				
				element.setData(config.createXml());
				element.setLayout(config.getLayout().createXml());
				recreateElement(element);
				redoLayout();
				setModified();
			}
			catch(Exception e)
			{
				MessageDialog.openError(getShell(), Messages.DashboardControl_InternalError, Messages.DashboardControl_InternalErrorPrefix + e.getMessage());
			}
		}
		else
		{
			MessageDialog.openError(getShell(), Messages.DashboardControl_InternalError, Messages.DashboardControl_InternalErrorText1);
		}
	}

	/**
	 * Edit element XML
	 * 
	 * @param element
	 */
	void editElementXml(DashboardElement element)
	{
		EditElementXmlDlg dlg = new EditElementXmlDlg(getShell(), element.getData());
		if (dlg.open() == Window.OK)
		{
			element.setData(dlg.getValue());
			recreateElement(element);
			redoLayout();
			setModified();
		}
	}
	
	/**
	 * @param element
	 */
	private void recreateElement(DashboardElement element)
	{
		ElementWidget w = elementWidgets.get(element);
		Control next = getNextWidget(w);
		w.dispose();
		w = createElementWidget(element);
		if (next != null)
			w.moveAbove(next);
		elementWidgets.put(element, w);
	}
	
	/**
	 * @param element
	 */
	private void addElement(DashboardElement element)
	{
		DashboardElementConfig config = (DashboardElementConfig)AdapterManager.getDefault().getAdapter(element, DashboardElementConfig.class);
		if (config != null)
		{
			try
			{
				config.setLayout(DashboardElementLayout.createFromXml(element.getLayout()));
				
				PropertyDialog dlg = PropertyDialog.createDialogOn(getShell(), null, config);
				if (dlg.open() == Window.CANCEL)
					return;	// element creation cancelled
				
				element.setData(config.createXml());
				element.setLayout(config.getLayout().createXml());
				elements.add(element);
				createElementWidget(element);
				redoLayout();
				setModified();
			}
			catch(Exception e)
			{
				MessageDialog.openError(getShell(), Messages.DashboardControl_InternalError, Messages.DashboardControl_InternalErrorPrefix + e.getMessage());
			}
		}
		else
		{
			MessageDialog.openError(getShell(), Messages.DashboardControl_InternalError, Messages.DashboardControl_InternalErrorText2);
		}
	}
	
	/**
	 * Add alarm browser widget to dashboard
	 */
	public void addAlarmBrowser()
	{
		DashboardElement e = new DashboardElement(DashboardElement.ALARM_VIEWER, DEFAULT_OBJECT_REFERENCE_CONFIG);
		addElement(e);
	}
	
	/**
	 * Add label widget to dashboard
	 */
	public void addLabel()
	{
		DashboardElement e = new DashboardElement(DashboardElement.LABEL, DEFAULT_LABEL_CONFIG);
		addElement(e);
	}
	
	/**
	 * Add pie chart widget to dashboard
	 */
	public void addPieChart()
	{
		DashboardElement e = new DashboardElement(DashboardElement.PIE_CHART, DEFAULT_CHART_CONFIG);
		addElement(e);
	}

	/**
	 * Add bar chart widget to dashboard
	 */
	public void addBarChart()
	{
		DashboardElement e = new DashboardElement(DashboardElement.BAR_CHART, DEFAULT_CHART_CONFIG);
		addElement(e);
	}

	/**
	 * Add tube chart widget to dashboard
	 */
	public void addTubeChart()
	{
		DashboardElement e = new DashboardElement(DashboardElement.TUBE_CHART, DEFAULT_CHART_CONFIG);
		addElement(e);
	}

	/**
	 * Add availability chart widget to dashboard
	 */
	public void addAvailabilityChart()
	{
		DashboardElement e = new DashboardElement(DashboardElement.AVAILABLITY_CHART, DEFAULT_AVAILABILITY_CHART_CONFIG);
		addElement(e);
	}

	/**
	 * Add tube chart widget to dashboard
	 */
	public void addLineChart()
	{
		DashboardElement e = new DashboardElement(DashboardElement.LINE_CHART, DEFAULT_LINE_CHART_CONFIG);
		addElement(e);
	}

	/**
	 * Add embedded dashboard widget to dashboard
	 */
	public void addEmbeddedDashboard()
	{
		DashboardElement e = new DashboardElement(DashboardElement.DASHBOARD, DEFAULT_OBJECT_REFERENCE_CONFIG);
		addElement(e);
	}

	/**
	 * Add status indicator widget to dashboard
	 */
	public void addStatusIndicator()
	{
		DashboardElement e = new DashboardElement(DashboardElement.STATUS_INDICATOR, DEFAULT_OBJECT_REFERENCE_CONFIG);
		addElement(e);
	}

	/**
	 * Save dashboard layout
	 * 
	 * @param viewPart
	 */
	public void saveDashboard(IViewPart viewPart)
	{
		final NXCObjectModificationData md = new NXCObjectModificationData(dashboard.getObjectId());
		md.setDashboardElements(new ArrayList<DashboardElement>(elements));
		final NXCSession session = (NXCSession)ConsoleSharedData.getSession();
		new ConsoleJob(Messages.DashboardControl_SaveLayout, viewPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.modifyObject(md);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						if (isDisposed())
							return;
						
						modified = false;
						if (modifyListener != null)
							modifyListener.save();
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.DashboardControl_SaveError + dashboard.getObjectName();
			}
		}.start();
	}
	
	/**
	 * Set modified flag
	 */
	private void setModified()
	{
		modified = true;
		if (modifyListener != null)
			modifyListener.modify();
	}

	/**
	 * @return the modifyListener
	 */
	public DashboardModifyListener getModifyListener()
	{
		return modifyListener;
	}

	/**
	 * @param modifyListener the modifyListener to set
	 */
	public void setModifyListener(DashboardModifyListener modifyListener)
	{
		this.modifyListener = modifyListener;
	}

	/**
	 * @return the modified
	 */
	public boolean isModified()
	{
		return modified;
	}

	/**
	 * @return the elements
	 */
	public List<DashboardElement> getElements()
	{
		return elements;
	}
}
