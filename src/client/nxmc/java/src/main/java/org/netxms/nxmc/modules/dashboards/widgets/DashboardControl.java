/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.dashboards.widgets;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Dashboard;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.layout.DashboardLayout;
import org.netxms.nxmc.base.layout.DashboardLayoutData;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.DashboardElementPropertiesManager;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfigFactory;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementLayout;
import org.netxms.nxmc.modules.dashboards.dialogs.EditElementXmlDlg;
import org.netxms.nxmc.modules.dashboards.views.AbstractDashboardView;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.IntermediateSelectionProvider;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Dashboard rendering control
 */
public class DashboardControl extends Composite
{
   private static final Logger logger = LoggerFactory.getLogger(DashboardControl.class);

   public static final String DEFAULT_CHART_CONFIG = "<element>\n\t<showIn3D>true</showIn3D>\n\t<dciList length=\"0\">\n\t</dciList>\n</element>";
   public static final String DEFAULT_LINE_CHART_CONFIG = "<element>\n\t<dciList length=\"0\">\n\t</dciList>\n</element>";
   public static final String DEFAULT_DIAL_CHART_CONFIG = "<element>\n\t<maxValue>100</maxValue>\n\t<yellowZone>70</yellowZone>\n\t<redZone>90</redZone>\n\t<dciList length=\"0\">\n\t</dciList>\n</element>";
   public static final String DEFAULT_AVAILABILITY_CHART_CONFIG = "<element>\n\t<objectId>9</objectId>\n\t<showIn3D>true</showIn3D>\n</element>";
   public static final String DEFAULT_TABLE_CHART_CONFIG = "<element>\n\t<showIn3D>true</showIn3D>\n\t<nodeId>0</nodeId>\n\t<dciId>0</dciId>\n\t<dataColumn>DATA</dataColumn>\n</element>";
   public static final String DEFAULT_OBJECT_REFERENCE_CONFIG = "<element>\n\t<objectId>0</objectId>\n</element>";
   public static final String DEFAULT_LABEL_CONFIG = "<element>\n\t<title>Label</title>\n</element>";
   public static final String DEFAULT_NETWORK_MAP_CONFIG = "<element>\n\t<objectId>0</objectId>\n\t<title></title>\n</element>";
   public static final String DEFAULT_GEO_MAP_CONFIG = "<element>\n\t<latitude>0</latitude>\n\t<longitude>0</longitude>\n\t<zoom>8</zoom>\t<title></title>\n</element>";
   public static final String DEFAULT_WEB_PAGE_CONFIG = "<element>\n\t<url>http://</url>\n\t<title></title>\n</element>";
   public static final String DEFAULT_TABLE_VALUE_CONFIG = "<element>\n\t<objectId>0</objectId>\n\t<dciId>0</dciId>\n\t<title></title>\n</element>";
   public static final String DEFAULT_SUMMARY_TABLE_CONFIG = "<element>\n\t<baseObjectId>0</baseObjectId>\n\t<tableId>0</tableId>\n</element>";

   private final I18n i18n = LocalizationHelper.getI18n(DashboardControl.class);

	private Dashboard dashboard;
   private AbstractObject context;
	private List<DashboardElement> elements;
	private Map<DashboardElement, ElementWidget> elementWidgets = new HashMap<DashboardElement, ElementWidget>();
   private int columnCount;
	private boolean embedded = false;
	private boolean editMode = false;
	private boolean modified = false;
	private DashboardModifyListener modifyListener = null;
   private AbstractDashboardView view;
	private IntermediateSelectionProvider selectionProvider;

   /**
    * Create new dashboard control.
    *
    * @param parent parent composite
    * @param style control style
    * @param dashboard dashboard object
    * @param context context for dashboard (can be null)
    * @param view owning view
    * @param selectionProvider selection provider
    * @param embedded true if dashboard control is embedded into another dashboard
    */
   public DashboardControl(Composite parent, int style, Dashboard dashboard, AbstractObject context, AbstractDashboardView view, IntermediateSelectionProvider selectionProvider, boolean embedded)
	{
		super(parent, style);
		this.dashboard = dashboard;
      this.context = context;
		this.embedded = embedded;
		this.elements = new ArrayList<DashboardElement>(dashboard.getElements());
      this.columnCount = dashboard.getNumColumns();
      this.view = view;
		this.selectionProvider = selectionProvider;
		createContent();
	}

	/**
    * Create clone of existing dashboard control.
    *
    * @param parent parent composite
    * @param style control style
    * @param originalControl original dashboard control
    */
   public DashboardControl(Composite parent, int style, DashboardControl originalControl)
	{
		super(parent, style);
      this.dashboard = originalControl.dashboard;
      this.embedded = originalControl.embedded;
      this.modified = originalControl.modified;
      this.elements = new ArrayList<DashboardElement>(originalControl.elements);
      this.columnCount = originalControl.columnCount;
      this.view = originalControl.view;
      this.selectionProvider = originalControl.selectionProvider;
		createContent();
	}

	/**
	 * Create dashboard's content
	 */
	private void createContent()
	{
      setBackground(ThemeEngine.getBackgroundColor("Dashboard"));

		DashboardLayout layout = new DashboardLayout();
      layout.numColumns = columnCount;
      layout.marginWidth = embedded ? 0 : 8;
      layout.marginHeight = embedded ? 0 : 8;
      layout.horizontalSpacing = 8;
      layout.verticalSpacing = 8;
		setLayout(layout);

		for(final DashboardElement e : elements)
		{
			createElementWidget(e);
		}
	}

	/**
    * Factory method for creating dashboard elements
    * 
    * @param e element to create widget for
    */
	private ElementWidget createElementWidget(DashboardElement e)
	{
		ElementWidget w;
		switch(e.getType())
		{   
         case DashboardElement.ALARM_VIEWER:
            w = new AlarmViewerElement(this, e, view);
            break;
         case DashboardElement.BAR_CHART:
         case DashboardElement.TUBE_CHART:
            w = new BarChartElement(this, e, view);
         	break;
         case DashboardElement.CUSTOM:
            w = new CustomWidgetElement(this, e, view);
         	break;
         case DashboardElement.DASHBOARD:
            w = new EmbeddedDashboardElement(this, e, view);
         	break;
         case DashboardElement.DCI_SUMMARY_TABLE:
            w = new DciSummaryTableElement(this, e, view);
            break;
         case DashboardElement.DIAL_CHART:
            w = new GaugeElement(this, e, view);
         	break;
         case DashboardElement.EVENT_MONITOR:
            w = new EventMonitorElement(this, e, view);
            break;
         case DashboardElement.FILE_MONITOR:
            w = new FileMonitorElement(this, e, view);
            break;
         case DashboardElement.GEO_MAP:
            w = new GeoMapElement(this, e, view);
         	break;
         case DashboardElement.LABEL:
            w = new LabelElement(this, e, view);
         	break;
         case DashboardElement.LINE_CHART:
            w = new LineChartElement(this, e, view);
         	break;
         case DashboardElement.NETWORK_MAP:
            w = new NetworkMapElement(this, e, view);
         	break;
         case DashboardElement.OBJECT_QUERY:
            w = new ObjectQuery(this, e, view);
            break;
         case DashboardElement.OBJECT_TOOLS:
            w = new ObjectTools(this, e, view);
            break;
         case DashboardElement.PIE_CHART:
            w = new PieChartElement(this, e, view);
            break;
         case DashboardElement.PORT_VIEW:
            w = new PortViewElement(this, e, view);
            break;
         case DashboardElement.RACK_DIAGRAM:
            w = new RackDiagramElement(this, e, view);
            break;
         case DashboardElement.SCRIPTED_BAR_CHART:
            w = new ScriptedBarChartElement(this, e, view);
            break;
         case DashboardElement.SCRIPTED_PIE_CHART:
            w = new ScriptedPieChartElement(this, e, view);
            break;
         case DashboardElement.SEPARATOR:
            w = new SeparatorElement(this, e, view);
            break;
         case DashboardElement.SERVICE_COMPONENTS:
            w = new ServiceComponentsElement(this, e, view);
            break;
         case DashboardElement.SNMP_TRAP_MONITOR:
            w = new SnmpTrapMonitorElement(this, e, view);
            break;
         case DashboardElement.STATUS_CHART:
            w = new ObjectStatusChartElement(this, e, view);
            break;
         case DashboardElement.STATUS_INDICATOR:
            w = new StatusIndicatorElement(this, e, view);
            break;
         case DashboardElement.STATUS_MAP:
            w = new StatusMapElement(this, e, view);
            break;
         case DashboardElement.SYSLOG_MONITOR:
            w = new SyslogMonitorElement(this, e, view);
            break;
         case DashboardElement.TABLE_BAR_CHART:
         case DashboardElement.TABLE_TUBE_CHART:
            w = new TableBarChartElement(this, e, view);
            break;
         case DashboardElement.TABLE_PIE_CHART:
            w = new TablePieChartElement(this, e, view);
            break;
         case DashboardElement.TABLE_VALUE:
            w = new TableValueElement(this, e, view);
            break;
         case DashboardElement.WEB_PAGE:
            w = new WebPageElement(this, e, view);
            break;
         default:
            w = new ElementWidget(this, e, view);
            break;
		}

		final DashboardElementLayout el = w.getElementLayout();
		final DashboardLayoutData gd = new DashboardLayoutData();
		gd.fill = el.grabVerticalSpace;
		gd.horizontalSpan = el.horizontalSpan;
		gd.verticalSpan = el.verticalSpan;
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
    * Move dashboard element.
    *
    * @param element element to move
    * @param direction either <code>SWT.LEFT</code> or <code>SWT.RIGHT</code>
    */
   void moveElement(DashboardElement element, int direction)
   {
      int index = elements.indexOf(element);
      if (index == -1)
         return;

      ElementWidget w = elementWidgets.get(element);
      if ((direction == SWT.LEFT) && (index > 0))
      {
         Collections.swap(elements, index, index - 1);
         w.moveAbove(elementWidgets.get(elements.get(index)));
      }
      else if ((direction == SWT.RIGHT) && (index < elements.size() - 1))
      {
         Collections.swap(elements, index, index + 1);
         w.moveBelow(elementWidgets.get(elements.get(index)));
      }
      else
      {
         return;
      }

      redoLayout();
      setModified();
   }

   /**
    * Change element's span.
    *
    * @param element element to update
    * @param hSpanChange horizontal span change
    * @param vSpanChange vertical span change
    */
   void changeElementSpan(DashboardElement element, int hSpanChange, int vSpanChange)
   {
      ElementWidget w = elementWidgets.get(element);
      if (w == null)
         return;

      DashboardElementLayout el = w.getElementLayout();
      int hSpan = el.horizontalSpan + hSpanChange;
      int vSpan = el.verticalSpan + vSpanChange;
      if ((hSpan < 1) || (vSpan < 1) || (hSpan > columnCount))
         return;

      el.horizontalSpan = hSpan;
      el.verticalSpan = vSpan;
      try
      {
         element.setLayout(el.createXml());
      }
      catch(Exception e)
      {
         logger.error("Error serializing updated dashboard element layout", e);
      }

      DashboardLayoutData gd = (DashboardLayoutData)w.getLayoutData();
      gd.horizontalSpan = el.horizontalSpan;
      gd.verticalSpan = el.verticalSpan;

      redoLayout();
      setModified();
   }

   /**
    * Change element's horizontal span to max possible value.
    *
    * @param element element to update
    */
   void setElementFullHSpan(DashboardElement element)
   {
      ElementWidget w = elementWidgets.get(element);
      if (w == null)
         return;

      DashboardElementLayout el = w.getElementLayout();
      if (el.horizontalSpan == columnCount)
         return;

      el.horizontalSpan = columnCount;
      try
      {
         element.setLayout(el.createXml());
      }
      catch(Exception e)
      {
         logger.error("Error serializing updated dashboard element layout", e);
      }

      DashboardLayoutData gd = (DashboardLayoutData)w.getLayoutData();
      gd.horizontalSpan = el.horizontalSpan;

      redoLayout();
      setModified();
   }

   /**
    * Duplicate element
    * 
    * @param src source element
    */
   void duplicateElement(DashboardElement src)
   {
      DashboardElement element = new DashboardElement(src);
      elements.add(element);
      createElementWidget(element);
      redoLayout();
      setModified();
   }

	/**
    * Delete element
    * 
    * @param element element to delete
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
      DashboardElementConfig config = DashboardElementConfigFactory.create(element);
		if (config == null)
		{
		   view.addMessage(MessageArea.ERROR, i18n.tr("Internal error (cannot parse element configuration)"));
		   return;
		}

      try
      {
			config.setLayout(DashboardElementLayout.createFromXml(element.getLayout()));

         if (!DashboardElementPropertiesManager.openElementPropertiesDialog(config, getShell()))
            return; // element edit cancelled

			element.setData(config.createXml());
			element.setLayout(config.getLayout().createXml());
			recreateElement(element);
			redoLayout();
			setModified();
		}
		catch(Exception e)
		{
         view.addMessage(MessageArea.ERROR, String.format(i18n.tr("Internal error (%s)"), e.getLocalizedMessage()));
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
    * Add new element.
    *
    * @param element new element
    */
   public void addElement(DashboardElement element)
	{
      DashboardElementConfig config = DashboardElementConfigFactory.create(element);
      if (config == null)
      {
         view.addMessage(MessageArea.ERROR, i18n.tr("Cannot add dashboard element (internal error)"));
         return;
      }

      try
		{
         config.setLayout(DashboardElementLayout.createFromXml(element.getLayout()));

         if (!DashboardElementPropertiesManager.openElementPropertiesDialog(config, getShell()))
            return; // element creation cancelled

         element.setData(config.createXml());
         element.setLayout(config.getLayout().createXml());
         elements.add(element);
         createElementWidget(element);
         redoLayout();
         setModified();
		}
      catch(Exception e)
		{
         view.addMessage(MessageArea.ERROR, String.format(i18n.tr("Cannot add dashboard element (internal error - %s)"), e.getLocalizedMessage()));
		}
	}

   /**
    * Add column
    */
   public void addColumn()
   {
      if (columnCount == 128)
         return;

      columnCount++;
      ((DashboardLayout)getLayout()).numColumns = columnCount;
      layout(true, true);
      setModified();
   }

   /**
    * Remove column
    */
   public void removeColumn()
   {
      if (columnCount == getMinimalColumnCount())
         return;

      columnCount--;
      ((DashboardLayout)getLayout()).numColumns = columnCount;
      layout(true, true);
      setModified();
   }

	/**
    * @return the columnCount
    */
   public int getColumnCount()
   {
      return columnCount;
   }

   /**
    * Get minimal column count
    *
    * @return minimal column count
    */
   public int getMinimalColumnCount()
   {
      int count = 1;
      for(DashboardElement element : elements)
      {
         try
         {
            DashboardElementLayout layout = DashboardElementLayout.createFromXml(element.getLayout());
            if (layout.horizontalSpan > count)
               count = layout.horizontalSpan;
         }
         catch(Exception e)
         {
            logger.error("Cannot parse dashboard element layout", e);
         }
      }
      return count;
   }

   /**
    * Save dashboard
    */
   public void saveDashboard()
	{
		final NXCObjectModificationData md = new NXCObjectModificationData(dashboard.getObjectId());
		md.setDashboardElements(new ArrayList<DashboardElement>(elements));
      md.setColumnCount(columnCount);
      final NXCSession session = Registry.getSession();
      new Job(i18n.tr("Saving dashboard"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
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
            return String.format(i18n.tr("Cannot save dashboard %s"), dashboard.getObjectName());
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
	
	/**
	 * Get dashboard's element widgets
	 * 
	 * @return
	 */
	public List<ElementWidget> getElementWidgets()
	{
	   return new ArrayList<ElementWidget>(elementWidgets.values());
	}

   /**
    * @return the selectionProvider
    */
   public IntermediateSelectionProvider getSelectionProvider()
   {
      return selectionProvider;
   }

   /**
    * @return the dashboard
    */
   public Dashboard getDashboardObject()
   {
      return dashboard;
   }

   /**
    * Get context.
    *
    * @return context for dashboard (can be null)
    */
   public AbstractObject getContext()
   {
      return context;
   }
}
