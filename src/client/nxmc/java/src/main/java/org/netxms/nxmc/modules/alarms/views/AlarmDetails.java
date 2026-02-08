/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.alarms.views;

import java.util.Date;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IMenuListener;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCException;
import org.netxms.client.constants.DataType;
import org.netxms.client.constants.HistoricalDataType;
import org.netxms.client.constants.RCC;
import org.netxms.client.constants.Severity;
import org.netxms.client.datacollection.ChartConfiguration;
import org.netxms.client.datacollection.ChartDciConfig;
import org.netxms.client.datacollection.DataSeries;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.datacollection.Threshold;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.AlarmComment;
import org.netxms.client.events.EventInfo;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.DataCollectionTarget;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.base.actions.CopyTableRowsAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.layout.DashboardLayout;
import org.netxms.nxmc.base.layout.DashboardLayoutData;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.Section;
import org.netxms.nxmc.base.widgets.SortableTreeViewer;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.dialogs.EditCommentDialog;
import org.netxms.nxmc.modules.alarms.views.helpers.EventTreeComparator;
import org.netxms.nxmc.modules.alarms.views.helpers.EventTreeContentProvider;
import org.netxms.nxmc.modules.alarms.views.helpers.EventTreeLabelProvider;
import org.netxms.nxmc.modules.alarms.widgets.AlarmCommentsEditor;
import org.netxms.nxmc.modules.charts.api.ChartType;
import org.netxms.nxmc.modules.charts.widgets.Chart;
import org.netxms.nxmc.modules.datacollection.views.helpers.HistoricalDataLabelProvider;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.ColorConverter;
import org.netxms.nxmc.tools.ImageCache;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.ViewRefreshController;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Alarm details
 */
public class AlarmDetails extends AdHocObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(AlarmDetails.class);
	public static final String ID = "AlarmDetails";

	public static final int EV_COLUMN_SEVERITY = 0;
	public static final int EV_COLUMN_SOURCE = 1;
	public static final int EV_COLUMN_NAME = 2;
	public static final int EV_COLUMN_MESSAGE = 3;
	public static final int EV_COLUMN_TIMESTAMP = 4;

	private static final String[] stateText = 
	   { 
	      LocalizationHelper.getI18n(AlarmDetails.class).tr("Outstanding"), 
	      LocalizationHelper.getI18n(AlarmDetails.class).tr("Acknowledged"), 
	      LocalizationHelper.getI18n(AlarmDetails.class).tr("Resolved"), 
	      LocalizationHelper.getI18n(AlarmDetails.class).tr("Terminated")
      };

	private long alarmId;
	private ImageCache imageCache;
   private BaseObjectLabelProvider objectLabelProvider;
   private Composite content;
	private CLabel alarmSeverity;
	private CLabel alarmState;
	private CLabel alarmSource;
   private Label alarmDCI;
   private CLabel alarmKey;
   private CLabel alarmRule;
	private Text alarmText;
   private ScrolledComposite editorsScroller;
	private Composite editorsArea;
	private ImageHyperlink linkAddComment;
	private Map<Long, AlarmCommentsEditor> editors = new HashMap<Long, AlarmCommentsEditor>();
	private Composite dataArea;
	private SortableTreeViewer eventViewer;
	private CLabel labelAccessDenied = null;
	private boolean dataSectionCreated = false;
   private boolean dataSectionPopulated = false;
	private Section dataSection;
   private Chart chart;
	private TableViewer dataViewer;
	private Control dataViewControl;
	private long nodeId;
	private long dciId;
	private ViewRefreshController refreshController = null;
	private boolean updateInProgress = false;
   private Image[] stateImages = new Image[5];
   private CopyTableRowsAction copyEvent;

   /**
    * Create alarm view
    */
   public AlarmDetails(long alarmId, long contextObject)
   {
      super(String.format(LocalizationHelper.getI18n(AlarmDetails.class).tr("Alarm Details [%d]"), alarmId), ResourceManager.getImageDescriptor("icons/object-views/alarms.png"),
            "objects.alarm-details", contextObject, contextObject, false);
      this.alarmId = alarmId;
      objectLabelProvider = new BaseObjectLabelProvider();

      stateImages[0] = ResourceManager.getImage("icons/alarms/outstanding.png");
      stateImages[1] = ResourceManager.getImage("icons/alarms/acknowledged.png");
      stateImages[2] = ResourceManager.getImage("icons/alarms/resolved.png");
      stateImages[3] = ResourceManager.getImage("icons/alarms/terminated.png");
      stateImages[4] = ResourceManager.getImage("icons/alarms/acknowledged_sticky.png");
   }
   
   protected AlarmDetails()
   {
      super(LocalizationHelper.getI18n(AlarmDetails.class).tr("Alarm Details"), ResourceManager.getImageDescriptor("icons/object-views/alarms.png"),
            "objects.alarm-details", 0, 0, false); 
      objectLabelProvider = new BaseObjectLabelProvider();

      stateImages[0] = ResourceManager.getImage("icons/alarms/outstanding.png");
      stateImages[1] = ResourceManager.getImage("icons/alarms/acknowledged.png");
      stateImages[2] = ResourceManager.getImage("icons/alarms/resolved.png");
      stateImages[3] = ResourceManager.getImage("icons/alarms/terminated.png");
      stateImages[4] = ResourceManager.getImage("icons/alarms/acknowledged_sticky.png");      
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      AlarmDetails view = (AlarmDetails)super.cloneView();
      view.alarmId = alarmId;
      return view;
   }  

   /**
    * Post clone action
    */
   protected void postClone(View origin)
   {      
      super.postClone(origin);
      refresh();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
		imageCache = new ImageCache();
      content = parent;

		DashboardLayout layout = new DashboardLayout();
		layout.marginWidth = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.OUTER_SPACING;
      parent.setLayout(layout);

      createAlarmDetailsSection(parent);
      createEventsSection(parent);
      createCommentsSection(parent);
      createDataSection(parent);
      
      createActions();
      createContextMenu();
	}

   /**
    * Create actions
    */
   private void createActions()
   {
      copyEvent = new CopyTableRowsAction(eventViewer, true);
   }

   /**
    * Create pop-up menu
    */
   protected void createContextMenu()
   {
      // Create menu manager.
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener(new IMenuListener() {
         public void menuAboutToShow(IMenuManager mgr)
         {
            fillContextMenu(mgr);
         }
      });

      // Create menu.
      Menu menu = menuMgr.createContextMenu(eventViewer.getControl());
      eventViewer.getControl().setMenu(menu);
   }

   /**
    * Fill context menu
    * 
    * @param mgr Menu manager
    */
   protected void fillContextMenu(final IMenuManager manager)
   {
      manager.add(copyEvent);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      super.postContentCreate();
      refresh();
   }  
   
	/**
	 * Create alarm details section
	 */
   private void createAlarmDetailsSection(Composite parent)
	{
      final Section section = new Section(parent, i18n.tr("Overview"), true);
		DashboardLayoutData dd = new DashboardLayoutData();
		dd.fill = false;
		section.setLayoutData(dd);

      final Composite clientArea = section.getClient();
		GridLayout layout = new GridLayout();
		layout.numColumns = 3;
		clientArea.setLayout(layout);

		alarmSeverity = new CLabel(clientArea, SWT.NONE);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.TOP;
		alarmSeverity.setLayoutData(gd);

		Label sep = new Label(clientArea, SWT.VERTICAL | SWT.SEPARATOR);
		gd = new GridData();
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 4;
		sep.setLayoutData(gd);

		final ScrolledComposite textContainer = new ScrolledComposite(clientArea, SWT.H_SCROLL | SWT.V_SCROLL) {
			@Override
			public Point computeSize(int wHint, int hHint, boolean changed)
			{
				Point size = super.computeSize(wHint, hHint, changed);
				if (size.y > 200)
					size.y = 200;
				return size;
			}
		};
		textContainer.setExpandHorizontal(true);
      WidgetHelper.setScrollBarIncrement(textContainer, SWT.HORIZONTAL, 20);
		textContainer.setExpandVertical(true);
      WidgetHelper.setScrollBarIncrement(textContainer, SWT.VERTICAL, 20);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
      gd.verticalSpan = 4;
		textContainer.setLayoutData(gd);
		textContainer.addControlListener(new ControlAdapter() {
			@Override
			public void controlResized(ControlEvent e)
			{
				Point size = alarmText.computeSize(SWT.DEFAULT, SWT.DEFAULT);
				alarmText.setSize(size.x, size.y);
				textContainer.setMinWidth(size.x);
				textContainer.setMinHeight(size.y);
			}
		});

      alarmText = new Text(textContainer, SWT.MULTI);
		alarmText.setEditable(false);
      alarmText.setBackground(clientArea.getBackground());
		textContainer.setContent(alarmText);

		alarmState = new CLabel(clientArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.TOP;
		alarmState.setLayoutData(gd);

		alarmSource = new CLabel(clientArea, SWT.NONE);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.TOP;
		alarmSource.setLayoutData(gd);

      alarmKey = new CLabel(clientArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      alarmKey.setLayoutData(gd);

      final Image keyImage = ResourceManager.getImageDescriptor("icons/key.png").createImage();
      alarmKey.setImage(keyImage);
      alarmKey.addDisposeListener((e) -> keyImage.dispose());

      alarmRule = new CLabel(clientArea, SWT.NONE);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.BOTTOM;
      gd.horizontalSpan = 3;
      alarmRule.setLayoutData(gd);

      final Image eppImage = ResourceManager.getImageDescriptor("icons/epp.png").createImage();
      alarmRule.setImage(eppImage);
      alarmRule.addDisposeListener((e) -> eppImage.dispose());
	}

	/**
	 * Create comment section
	 */
   private void createCommentsSection(Composite parent)
	{
      final Section section = new Section(parent, i18n.tr("Comments"), true);
      final DashboardLayoutData dd = new DashboardLayoutData();
      dd.fill = true;
      section.setLayoutData(dd);
      section.addExpansionListener((e) -> dd.fill = e.getState());

      editorsScroller = new ScrolledComposite(section.getClient(), SWT.V_SCROLL);
      editorsScroller.setBackground(section.getClient().getBackground());
      editorsScroller.setExpandHorizontal(true);
      editorsScroller.setExpandVertical(true);
      WidgetHelper.setScrollBarIncrement(editorsScroller, SWT.VERTICAL, 20);
      editorsScroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            editorsArea.layout(true, true);
            editorsScroller.setMinSize(editorsArea.computeSize(editorsScroller.getClientArea().width, SWT.DEFAULT));
         }
      });

      editorsArea = new Composite(editorsScroller, SWT.NONE);
      editorsArea.setBackground(section.getClient().getBackground());
		GridLayout layout = new GridLayout();
		editorsArea.setLayout(layout);

      editorsScroller.setContent(editorsArea);

      linkAddComment = new ImageHyperlink(editorsArea, SWT.NONE);
		linkAddComment.setImage(imageCache.create(ResourceManager.getImageDescriptor("icons/new_comment.png"))); //$NON-NLS-1$
		linkAddComment.setText(i18n.tr("Add comment"));
      linkAddComment.setBackground(editorsArea.getBackground());
		linkAddComment.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addComment();
			}
		});  
	}

	/**
	 * Create events section
	 */
   private void createEventsSection(Composite parent)
	{
      final Section section = new Section(parent, i18n.tr("Related Events"), true);
		final DashboardLayoutData dd = new DashboardLayoutData();
		dd.fill = true;
		section.setLayoutData(dd);
      section.addExpansionListener((e) -> dd.fill = e.getState());

		final String[] names = { i18n.tr("Severity"), i18n.tr("Source"), i18n.tr("Name"), i18n.tr("Message"), i18n.tr("Timestamp")};
		final int[] widths = { 130, 160, 160, 400, 150 };
      eventViewer = new SortableTreeViewer(section.getClient(), names, widths, EV_COLUMN_TIMESTAMP, SWT.DOWN, SWT.FULL_SELECTION);
		eventViewer.setContentProvider(new EventTreeContentProvider());
		eventViewer.setLabelProvider(new EventTreeLabelProvider());
		eventViewer.setComparator(new EventTreeComparator());

		WidgetHelper.restoreTreeViewerSettings(eventViewer, "AlarmDetails.Events"); //$NON-NLS-1$
		eventViewer.getControl().addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            WidgetHelper.saveTreeViewerSettings(eventViewer, "AlarmDetails.Events"); //$NON-NLS-1$
         }
      });
	}

	/**
	 * Create data section
	 */
   private void createDataSection(Composite parent)
	{
      dataSection = new Section(parent, i18n.tr("DCI Data"), true);
      final DashboardLayoutData dd = new DashboardLayoutData();
      dd.fill = true;
      dataSection.setLayoutData(dd);
      dataSection.addExpansionListener((e) -> dd.fill = e.getState() && dataSectionPopulated);

      dataArea = new Composite(dataSection.getClient(), SWT.NONE);
      dataArea.setLayout(new GridLayout());
      dataArea.setBackground(ThemeEngine.getBackgroundColor("Chart.Base"));
	}

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
		new Job(i18n.tr("Reading alarm details"), this) {
			@Override
			protected void run(IProgressMonitor monitor) throws Exception
			{
				final Alarm alarm = session.getAlarm(alarmId);
				final List<AlarmComment> comments = session.getAlarmComments(alarmId);

				AbstractObject sourceObject = session.findObjectById(alarm.getSourceObjectId());
				final DciValue[] lastValues; 
				if (sourceObject instanceof DataCollectionTarget)
				{
				   lastValues = session.getLastValues(alarm.getSourceObjectId());
				}
				else
				{
				   lastValues = new DciValue[] {};
				}

            List<EventInfo> _events = null;
            try
            {
               _events = session.getAlarmEvents(alarmId);
            }
            catch(NXCException e)
            {
               if (e.getErrorCode() != RCC.ACCESS_DENIED)
                  throw e;
            }
				final List<EventInfo> events = _events;
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						updateAlarmDetails(alarm);
						
						for(AlarmCommentsEditor e : editors.values())
							e.dispose();

						for(AlarmComment n : comments)
							editors.put(n.getId(), createEditor(n, alarm.getState() != Alarm.STATE_TERMINATED));
				      linkAddComment.setVisible(alarm.getState() != Alarm.STATE_TERMINATED);    

						if (events != null)
						{
   						eventViewer.setInput(events);
   						eventViewer.expandAll();
                     if (labelAccessDenied != null)
                     {
                        labelAccessDenied.dispose();
                        labelAccessDenied = null;
                     }
						}
						else if (labelAccessDenied == null)
                  {
                     labelAccessDenied = new CLabel(eventViewer.getControl().getParent(), SWT.NONE);
                     labelAccessDenied.setImage(StatusDisplayInfo.getStatusImage(Severity.CRITICAL));
                     labelAccessDenied.setText(i18n.tr("Cannot get list of related events - access denied"));
                     labelAccessDenied.moveAbove(null);
                     labelAccessDenied.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));
                  }
						
						if (!dataSectionCreated)
						{
   						if (alarm.getDciId() != 0)
   						{
   						   DciValue dci = null;
   						   for(DciValue d : lastValues)
   						   {
   						      if (d.getId() == alarm.getDciId())
   						      {
   						         dci = d;
   						         break;
   						      }
   						   }
   						   if (dci != null)
   						   {
   						      createDataAreaElements(alarm, dci);
      				         refreshData();
                           dataSectionPopulated = true;
   						   }
   						   else
   						   {
   	                     Label label = new Label(dataArea, SWT.NONE);
   	                     label.setText(String.format("DCI with ID %d is not accessible", alarm.getDciId()));
                           dataSection.setExpanded(false);
   	                     DashboardLayoutData dd = (DashboardLayoutData)dataSection.getLayoutData();
   	                     dd.fill = false;
   						   }
   						}
   						else
   						{
   						   Label label = new Label(dataArea, SWT.NONE);
   						   label.setText("No DCI associated with this alarm");
                        dataSection.setExpanded(false);
   						   DashboardLayoutData dd = (DashboardLayoutData)dataSection.getLayoutData();
   						   dd.fill = false;
   						}
   						dataSectionCreated = true;
						}						
						else if (dataViewControl != null)
						{
						   refreshData();
						}
						updateLayout();
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return i18n.tr("Cannot get alarm details from server");
			}
		}.start();
	}
	
	/**
	 * Create elements of data area
	 * 
	 * @param alarm current alarm
	 * @param dci current DCI
	 */
	private void createDataAreaElements(Alarm alarm, DciValue dci)
	{
      nodeId = alarm.getSourceObjectId();
      dciId = alarm.getDciId();

      alarmDCI = new Label(dataArea, SWT.NONE);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.CENTER;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.CENTER;
      alarmDCI.setLayoutData(gd);

      FontData fd = alarmDCI.getFont().getFontData()[0];
      fd.setStyle(SWT.BOLD);
      final Font font = new Font(alarmDCI.getDisplay(), fd);
      alarmDCI.setFont(font);
      alarmDCI.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            font.dispose();
         }
      });

      Threshold t = dci.getActiveThreshold();
      alarmDCI.setText(dci.getDescription() + ((t != null) ? (" (" + t.getTextualRepresentation() + ")") : " (OK)"));

      if (dci.getDataType() != DataType.STRING)
      {
         ChartConfiguration chartConfiguration = new ChartConfiguration();
         chartConfiguration.setZoomEnabled(true);
         chartConfiguration.setTitleVisible(false);
         chartConfiguration.setTitle(dci.getDescription());
         chartConfiguration.setLegendVisible(false);
         chartConfiguration.setLegendPosition(ChartConfiguration.POSITION_BOTTOM);
         chartConfiguration.setExtendedLegend(true);
         chartConfiguration.setGridVisible(true);
         chartConfiguration.setTranslucent(true);

         chart = new Chart(dataArea, SWT.NONE, ChartType.LINE, chartConfiguration, this);
         ChartDciConfig item = new ChartDciConfig(dci);
         item.lineChartType = ChartDciConfig.AREA;
         item.setColor(ColorConverter.rgbToInt(new RGB(127, 154, 72)));
         chart.addParameter(item);
         chart.rebuild();

         dataViewControl = chart;
      }
      else
      {
         ((GridLayout)dataArea.getLayout()).marginWidth = 0;
         ((GridLayout)dataArea.getLayout()).marginTop = ((GridLayout)dataArea.getLayout()).marginHeight;
         ((GridLayout)dataArea.getLayout()).marginHeight = 0;
         ((GridLayout)dataArea.getLayout()).verticalSpacing = 0;

         Label separator = new Label(dataArea, SWT.SEPARATOR | SWT.HORIZONTAL);
         gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
         gd.verticalIndent = 5;
         separator.setLayoutData(gd);

         dataViewer = new TableViewer(dataArea, SWT.FULL_SELECTION);
         dataViewer.getTable().setHeaderVisible(true);

         TableColumn tc = new TableColumn(dataViewer.getTable(), SWT.LEFT);
         tc.setText("Timestamp");
         tc = new TableColumn(dataViewer.getTable(), SWT.LEFT);
         tc.setText("Value");

         dataViewer.setContentProvider(new ArrayContentProvider());
         dataViewer.setLabelProvider(new HistoricalDataLabelProvider());

         dataViewControl = dataViewer.getControl();
      }

      dataViewControl.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      refreshController = new ViewRefreshController(AlarmDetails.this, 30, new Runnable() {
         @Override
         public void run()
         {
            if (dataViewControl.isDisposed())
               return;
            
            refreshData();
         }
      });
	}

	/**
	 * Update layout after internal change
	 */
	private void updateLayout()
	{
      content.layout(true, true);
      editorsArea.layout(true, true);
      editorsScroller.setMinSize(editorsArea.computeSize(editorsScroller.getClientArea().width, SWT.DEFAULT));
	}

	/**
	 * Create comment editor widget
	 * 
	 * @param note alarm note associated with this widget
	 * @return
	 */
	private AlarmCommentsEditor createEditor(final AlarmComment note, boolean enableEditing)
	{
      HyperlinkAdapter editAction = new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            editComment(note.getId(), note.getText());
         }
      };
      HyperlinkAdapter deleteAction = new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            deleteComment(note.getId());
         }
      };
      final AlarmCommentsEditor e = new AlarmCommentsEditor(editorsArea, imageCache, note, editAction, deleteAction, enableEditing);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		e.setLayoutData(gd);
		e.moveBelow(linkAddComment);
		return e;
	}

	/**
    * Add new comment
    */
   private void addComment()
   {
      editComment(0, "");//$NON-NLS-1$
   }
   
   /**
    * Edit comment
    */
   private void editComment(final long noteId, String noteText)
   {
      final EditCommentDialog dlg = new EditCommentDialog(getWindow().getShell(), noteId , noteText);
      if (dlg.open() != Window.OK)
         return;

      new Job(i18n.tr("Add new or edit alarm comment"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateAlarmComment(alarmId, noteId, dlg.getText());
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot add alarm comment");
         }
      }.start();
   }
	
   /**
    * Delete comment
    */
   private void deleteComment(final long noteId)
   {
      if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Confirmation"), i18n.tr("Are you sure you want to delete this alarm comment?")))
         return;
      
      new Job(i18n.tr("Delete comment job"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.deleteAlarmComment(alarmId, noteId);
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  refresh();
               }
            });
         }
         
         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete alarm comment");
         }
      }.start();
   }
   
	/**
	 * Update alarm details
	 * 
	 * @param alarm
	 */
	private void updateAlarmDetails(Alarm alarm)
	{
		alarmSeverity.setImage(StatusDisplayInfo.getStatusImage(alarm.getCurrentSeverity()));
		alarmSeverity.setText(StatusDisplayInfo.getStatusText(alarm.getCurrentSeverity()));

		int state = alarm.getState();
		if ((state == Alarm.STATE_ACKNOWLEDGED) && alarm.isSticky())
			state = Alarm.STATE_TERMINATED + 1;
		alarmState.setImage(stateImages[state]);
		alarmState.setText(stateText[alarm.getState()]);

		AbstractObject object = session.findObjectById(alarm.getSourceObjectId());
		alarmSource.setImage((object != null) ? objectLabelProvider.getImage(object) : SharedIcons.IMG_UNKNOWN_OBJECT);
		alarmSource.setText((object != null) ? object.getObjectName() : ("[" + Long.toString(alarm.getSourceObjectId()) + "]")); //$NON-NLS-1$ //$NON-NLS-2$

      alarmKey.setText(alarm.getKey());
		alarmText.setText(alarm.getMessage());
      alarmRule.setText(String.format("Created by rule \"%s\" (%s)", alarm.getRuleDescription(), alarm.getRuleId().toString()));
	}

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
	@Override
	public void dispose()
	{
	   if (refreshController != null)
	      refreshController.dispose();
      for(int i = 0; i < stateImages.length; i++)
         stateImages[i].dispose();
		imageCache.dispose();
		objectLabelProvider.dispose();
		super.dispose();
	}

   /**
    * Refresh graph's data
    */
   private void refreshData()
   {
      if (updateInProgress)
         return;
      
      updateInProgress = true;
      
      Job job = new Job("Update DCI data view", this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            if (chart != null)
            {
               final Date from = new Date(System.currentTimeMillis() - 86400000);
               final Date to = new Date(System.currentTimeMillis());
               final DataSeries data = session.getCollectedData(nodeId, dciId, from, to, 0, HistoricalDataType.PROCESSED);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (!dataViewControl.isDisposed())
                     {
                        chart.setTimeRange(from, to);
                        chart.updateParameter(0, data, true);
                     }
                     updateInProgress = false;
                  }
               });
            }
            else
            {
               final DataSeries data = session.getCollectedData(nodeId, dciId, null, null, 20, HistoricalDataType.PROCESSED);
               runInUIThread(new Runnable() {
                  @Override
                  public void run()
                  {
                     if (!dataViewControl.isDisposed())
                     {
                        dataViewer.setInput(data.getValues());
                        for(TableColumn tc : dataViewer.getTable().getColumns())
                        {
                           tc.pack();
                           tc.setWidth(tc.getWidth() + 10); // compensate for pack issues on Linux
                        }
                     }
                     updateInProgress = false;
                  }
               });
            }
         }

         @Override
         protected String getErrorMessage()
         {
            return "Cannot read DCI data from server";
         }

         @Override
         protected void jobFailureHandler(Exception e)
         {
            updateInProgress = false;
            super.jobFailureHandler(e);
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#getFullName()
    */
   @Override
   public String getFullName()
   {
      return getName();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#isValidForContext(java.lang.Object)
    */
   @Override
   public boolean isValidForContext(Object context)
   {
      if (getObjectId() == 0  && getContextId() == 0)
         return true;
      
      return (context != null) && (context instanceof AbstractObject) && 
            ((((AbstractObject)context).getObjectId() == getObjectId()) || (((AbstractObject)context).getObjectId() == getContextId()));
   }

   /**
    * @see org.netxms.nxmc.base.views.View#saveState(org.netxms.nxmc.Memento)
    */
   @Override
   public void saveState(Memento memento)
   {
      super.saveState(memento);
      memento.set("alarmId", alarmId);
   }

   /**
    * @throws ViewNotRestoredException 
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {      
      super.restoreState(memento);
      alarmId = memento.getAsLong("alarmId", 0);  
      setName(String.format(LocalizationHelper.getI18n(AlarmDetails.class).tr("Alarm Details [%d]"), alarmId));
      if (alarmId == 0)
         throw(new ViewNotRestoredException(i18n.tr("Invalid alarm id")));
   }
}
