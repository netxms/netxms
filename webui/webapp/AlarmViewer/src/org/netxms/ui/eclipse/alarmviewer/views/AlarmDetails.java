/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2012 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.alarmviewer.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.forms.events.ExpansionEvent;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.events.IExpansionListener;
import org.eclipse.ui.forms.widgets.Form;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.AlarmNote;
import org.netxms.client.events.EventInfo;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.alarmviewer.dialogs.EditCommentDialog;
import org.netxms.ui.eclipse.alarmviewer.views.helpers.EventTreeContentProvider;
import org.netxms.ui.eclipse.alarmviewer.views.helpers.EventTreeLabelProvider;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmCommentsEditor;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.datacollection.widgets.LastValuesWidget;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.shared.SharedIcons;
import org.netxms.ui.eclipse.tools.ImageCache;
import org.netxms.ui.eclipse.widgets.SortableTreeViewer;

/**
 * Alarm comments
 */
public class AlarmDetails extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.alarmviewer.views.AlarmDetails"; //$NON-NLS-1$
	
	public static final int EV_COLUMN_SEVERITY = 0;
	public static final int EV_COLUMN_SOURCE = 1;
	public static final int EV_COLUMN_NAME = 2;
	public static final int EV_COLUMN_MESSAGE = 3;
	public static final int EV_COLUMN_TIMESTAMP = 4;
	
	private static final String[] stateImage = { "icons/outstanding.png", "icons/acknowledged.png", "icons/terminated.png" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
	private static final String[] stateText = { Messages.get().AlarmListLabelProvider_AlarmState_Outstanding, Messages.get().AlarmListLabelProvider_AlarmState_Acknowledged, Messages.get().AlarmListLabelProvider_AlarmState_Terminated };	
	
	private NXCSession session;
	private long alarmId;
	private ImageCache imageCache;
	private WorkbenchLabelProvider wbLabelProvider;
	private ScrolledComposite scroller;
	private Composite formContainer;
	private FormToolkit toolkit;
	private Form form;
	private CLabel alarmSeverity;
	private CLabel alarmState;
	private CLabel alarmSource;
	private Label alarmText;
	private Composite editorsArea;
	private ImageHyperlink linkAddComment;
	private Map<Long, AlarmCommentsEditor> editors = new HashMap<Long, AlarmCommentsEditor>();
	private Composite dataArea;
	private LastValuesWidget lastValuesWidget = null;
	private SortableTreeViewer eventViewer;
	private Action actionRefresh;
	
	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.ViewPart#init(org.eclipse.ui.IViewSite)
	 */
	@Override
	public void init(IViewSite site) throws PartInitException
	{
		super.init(site);
		session = (NXCSession)ConsoleSharedData.getSession();
		wbLabelProvider = new WorkbenchLabelProvider();
		
		try
		{
			alarmId = Long.parseLong(site.getSecondaryId());
		}
		catch(NumberFormatException e)
		{
			throw new PartInitException(Messages.get().AlarmComments_InternalError, e);
		}
		
		setPartName(getPartName() + " [" + Long.toString(alarmId) + "]"); //$NON-NLS-1$ //$NON-NLS-2$
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#createPartControl(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	public void createPartControl(Composite parent)
	{
		imageCache = new ImageCache();
		
		scroller = new ScrolledComposite(parent, SWT.V_SCROLL);
		scroller.setExpandVertical(true);
		//scroller.getVerticalBar().setIncrement(20);
		
		formContainer = new Composite(scroller, SWT.NONE);
		GridLayout containerLayout = new GridLayout();
		containerLayout.marginHeight = 0;
		containerLayout.marginWidth = 0;
		formContainer.setLayout(containerLayout);
		scroller.setContent(formContainer);
		scroller.addControlListener(new ControlAdapter() {
			private static final long serialVersionUID = 1L;

			@Override
			public void controlResized(ControlEvent e)
			{
				Rectangle r = scroller.getClientArea();
				Point formSize = formContainer.computeSize(r.width, SWT.DEFAULT);
				formContainer.setSize(r.width, formSize.y);
				scroller.setMinHeight(formSize.y);
			}
		});
		
		toolkit = new FormToolkit(parent.getDisplay());
		form = toolkit.createForm(formContainer);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		form.setLayoutData(gd);

		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		form.getBody().setLayout(layout);
		
		createAlarmDetailsSection();
		createEventsSection();
		createCommentsSection();
		createDataSection();
		
		createActions();
		contributeToActionBars();

		refresh();
	}
	

	/**
	 * Create actions
	 */
	private void createActions()
	{
		actionRefresh = new RefreshAction() {
			private static final long serialVersionUID = 1L;

			@Override
			public void run()
			{
				refresh();
			}
		};
	}
	
	/**
	 * Contribute actions to action bar
	 */
	private void contributeToActionBars()
	{
		IActionBars bars = getViewSite().getActionBars();
		fillLocalPullDown(bars.getMenuManager());
		fillLocalToolBar(bars.getToolBarManager());
	}

	/**
	 * Fill local pull-down menu
	 * 
	 * @param manager
	 *           Menu manager for pull-down menu
	 */
	private void fillLocalPullDown(IMenuManager manager)
	{
		manager.add(actionRefresh);
	}

	/**
	 * Fill local tool bar
	 * 
	 * @param manager
	 *           Menu manager for local toolbar
	 */
	private void fillLocalToolBar(IToolBarManager manager)
	{
		manager.add(actionRefresh);
	}

	/**
	 * Create alarm details section
	 */
	private void createAlarmDetailsSection()
	{
		final Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR | Section.EXPANDED | Section.TWISTIE | Section.COMPACT);
		section.setText(Messages.get().AlarmDetails_Overview);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		section.setLayoutData(gd);

		final Composite clientArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		clientArea.setLayout(layout);
		section.setClient(clientArea);

		alarmSeverity = new CLabel(clientArea, SWT.NONE);
		toolkit.adapt(alarmSeverity);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		alarmSeverity.setLayoutData(gd);

		alarmState = new CLabel(clientArea, SWT.NONE);
		toolkit.adapt(alarmState);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		alarmState.setLayoutData(gd);
		
		alarmSource = new CLabel(clientArea, SWT.NONE);
		toolkit.adapt(alarmSource);
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		alarmSource.setLayoutData(gd);

		alarmText = toolkit.createLabel(clientArea, "", SWT.WRAP); //$NON-NLS-1$
		gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalSpan = 2;
		alarmText.setLayoutData(gd);
	}
	
	/**
	 * Create comment section
	 */
	private void createCommentsSection()
	{
		final Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR | Section.EXPANDED | Section.TWISTIE | Section.COMPACT);
		section.setText(Messages.get().AlarmComments_Comments);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		gd.verticalSpan = 2;
		section.setLayoutData(gd);

		editorsArea = toolkit.createComposite(section);
		GridLayout layout = new GridLayout();
		editorsArea.setLayout(layout);
		section.setClient(editorsArea);
		
		linkAddComment = toolkit.createImageHyperlink(editorsArea, SWT.NONE);
		linkAddComment.setImage(imageCache.add(Activator.getImageDescriptor("icons/new_comment.png"))); //$NON-NLS-1$
		linkAddComment.setText(Messages.get().AlarmComments_AddCommentLink);
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
	private void createEventsSection()
	{
		final Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR | Section.EXPANDED | Section.TWISTIE | Section.COMPACT);
		section.setText(Messages.get().AlarmDetails_RelatedEvents);
		final GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		section.setLayoutData(gd);
		section.addExpansionListener(new IExpansionListener() {
			@Override
			public void expansionStateChanging(ExpansionEvent e)
			{
				gd.grabExcessVerticalSpace = e.getState();
			}
			
			@Override
			public void expansionStateChanged(ExpansionEvent e)
			{
			}
		});
		
		final String[] names = { Messages.get().AlarmDetails_Column_Severity, Messages.get().AlarmDetails_Column_Source, Messages.get().AlarmDetails_Column_Name, Messages.get().AlarmDetails_Column_Message, Messages.get().AlarmDetails_Column_Timestamp };
		final int[] widths = { 130, 160, 160, 400, 120 };
		eventViewer = new SortableTreeViewer(section, names, widths, 0, SWT.UP, SWT.BORDER | SWT.FULL_SELECTION);
		section.setClient(eventViewer.getControl());
		eventViewer.setContentProvider(new EventTreeContentProvider());
		eventViewer.setLabelProvider(new EventTreeLabelProvider());
	}

	/**
	 * Create data section
	 */
	private void createDataSection()
	{
		final Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR | Section.EXPANDED | Section.TWISTIE | Section.COMPACT);
		section.setText(Messages.get().AlarmDetails_LastValues);
		final GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessVerticalSpace = true;
		section.setLayoutData(gd);
		section.addExpansionListener(new IExpansionListener() {
			@Override
			public void expansionStateChanging(ExpansionEvent e)
			{
				gd.grabExcessVerticalSpace = e.getState();
			}
			
			@Override
			public void expansionStateChanged(ExpansionEvent e)
			{
			}
		});
		
		dataArea = toolkit.createComposite(section);
		section.setClient(dataArea);
		dataArea.setLayout(new FillLayout());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#setFocus()
	 */
	@Override
	public void setFocus()
	{
		form.setFocus();
	}
	
	/**
	 * Refresh view
	 */
	private void refresh()
	{
		new ConsoleJob(Messages.get().AlarmDetails_RefreshJobTitle, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final Alarm alarm = session.getAlarm(alarmId);
				final List<AlarmNote> comments = session.getAlarmNotes(alarmId);
				final List<EventInfo> events = session.getAlarmEvents(alarmId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						updateAlarmDetails(alarm);
						
						for(AlarmCommentsEditor e : editors.values())
							e.dispose();
						
						for(AlarmNote n : comments)
							editors.put(n.getId(), createEditor(n));
						
						if (lastValuesWidget == null)
						{
							GenericObject object = session.findObjectById(alarm.getSourceObjectId());
							if (object != null)
							{
								lastValuesWidget = new LastValuesWidget(AlarmDetails.this, dataArea, SWT.BORDER, object, "AlarmDetails.LastValues"); //$NON-NLS-1$
								lastValuesWidget.refresh();
							}
						}
						
						eventViewer.setInput(events);
						eventViewer.expandAll();
						
						updateLayout();
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().AlarmDetails_RefreshJobError;
			}
		}.start();
	}
	
	/**
	 * Update layout after internal change
	 */
	private void updateLayout()
	{
		formContainer.layout(true, true);
		Rectangle r = scroller.getClientArea();
		Point formSize = formContainer.computeSize(r.width, SWT.DEFAULT);
		formContainer.setSize(r.width, formSize.y);
		scroller.setMinHeight(formSize.y);
	}
	
	/**
	 * Create comment editor widget
	 * 
	 * @param note alarm note associated with this widget
	 * @return
	 */
	private AlarmCommentsEditor createEditor(AlarmNote note)
	{
		final AlarmCommentsEditor e = new AlarmCommentsEditor(editorsArea, toolkit, imageCache, note);
		toolkit.adapt(e);
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
		final EditCommentDialog dlg = new EditCommentDialog(getSite().getShell());
		if (dlg.open() != Window.OK)
			return;
		
		new ConsoleJob(Messages.get().AlarmComments_AddCommentJob, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				session.updateAlarmNote(alarmId, 0, dlg.getText());
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
				return Messages.get().AlarmComments_AddError;
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
		
		alarmState.setImage(imageCache.add(Activator.getImageDescriptor(stateImage[alarm.getState()])));
		alarmState.setText(stateText[alarm.getState()]);
		
		GenericObject object = session.findObjectById(alarm.getSourceObjectId());
		alarmSource.setImage((object != null) ? wbLabelProvider.getImage(object) : SharedIcons.IMG_UNKNOWN_OBJECT);
		alarmSource.setText((object != null) ? object.getObjectName() : ("[" + Long.toString(alarm.getSourceObjectId()) + "]")); //$NON-NLS-1$ //$NON-NLS-2$
		
		alarmText.setText(alarm.getMessage());
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.part.WorkbenchPart#dispose()
	 */
	@Override
	public void dispose()
	{
		imageCache.dispose();
		wbLabelProvider.dispose();
		super.dispose();
	}
}
