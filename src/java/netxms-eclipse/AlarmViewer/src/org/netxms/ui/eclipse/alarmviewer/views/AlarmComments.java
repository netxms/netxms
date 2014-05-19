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
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.ui.IActionBars;
import org.eclipse.ui.IViewSite;
import org.eclipse.ui.PartInitException;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.eclipse.ui.model.WorkbenchLabelProvider;
import org.eclipse.ui.part.ViewPart;
import org.netxms.client.NXCSession;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.AlarmComment;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.actions.RefreshAction;
import org.netxms.ui.eclipse.alarmviewer.Activator;
import org.netxms.ui.eclipse.alarmviewer.Messages;
import org.netxms.ui.eclipse.alarmviewer.dialogs.EditCommentDialog;
import org.netxms.ui.eclipse.alarmviewer.widgets.AlarmCommentsEditor;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.ImageCache;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

/**
 * Alarm comments
 */
public class AlarmComments extends ViewPart
{
	public static final String ID = "org.netxms.ui.eclipse.alarmviewer.views.AlarmComments"; //$NON-NLS-1$
	
	private static final String[] stateImage = { "icons/outstanding.png", "icons/acknowledged.png", "icons/terminated.png" }; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
	private static final String[] stateText = { Messages.get().AlarmListLabelProvider_AlarmState_Outstanding, Messages.get().AlarmListLabelProvider_AlarmState_Acknowledged, Messages.get().AlarmListLabelProvider_AlarmState_Terminated };	
	
	private NXCSession session;
	private long alarmId;
	private ImageCache imageCache;
	private WorkbenchLabelProvider wbLabelProvider;
	private FormToolkit toolkit;
	private ScrolledForm form;
	private CLabel alarmSeverity;
	private CLabel alarmState;
	private CLabel alarmSource;
	private Label alarmText;
	private Composite editorsArea;
	private ImageHyperlink linkAddComment;
	private Map<Long, AlarmCommentsEditor> editors = new HashMap<Long, AlarmCommentsEditor>();
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
		
		toolkit = new FormToolkit(parent.getDisplay());
		form = toolkit.createScrolledForm(parent);

		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 1;
		form.getBody().setLayout(layout);
		
		createAlarmDetailsSection();
		createEditorsSection();
		
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
		final Section details = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		details.setText(Messages.get().AlarmComments_Details);
		TableWrapData twd = new TableWrapData();
		twd.grabHorizontal = true;
		twd.align = TableWrapData.FILL;
		details.setLayoutData(twd);

		final Composite clientArea = toolkit.createComposite(details);
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		layout.makeColumnsEqualWidth = true;
		clientArea.setLayout(layout);
		details.setClient(clientArea);

		alarmSeverity = new CLabel(clientArea, SWT.NONE);
		toolkit.adapt(alarmSeverity);
		GridData gd = new GridData();
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
	 * Create comment list
	 */
	private void createEditorsSection()
	{
		final Section details = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		details.setText(Messages.get().AlarmComments_Comments);
		TableWrapData twd = new TableWrapData();
		twd.grabHorizontal = true;
		twd.align = TableWrapData.FILL;
		details.setLayoutData(twd);

		editorsArea = toolkit.createComposite(details);
		GridLayout layout = new GridLayout();
		editorsArea.setLayout(layout);
		details.setClient(editorsArea);
		
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
		new ConsoleJob(Messages.get().AlarmComments_GetComments, this, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				final Alarm alarm = session.getAlarm(alarmId);
				final List<AlarmComment> comments = session.getAlarmComments(alarmId);
				runInUIThread(new Runnable() {
					@Override
					public void run()
					{
						updateAlarmDetails(alarm);
						
						for(AlarmCommentsEditor e : editors.values())
							e.dispose();
						
						for(AlarmComment n : comments)
							editors.put(n.getId(), createEditor(n));
						
						updateLayout();
					}
				});
			}
			
			@Override
			protected String getErrorMessage()
			{
				return Messages.get().AlarmComments_GetError;
			}
		}.start();
	}
	
	/**
	 * Update layout after internal change
	 */
	private void updateLayout()
	{
		form.reflow(true);
		form.getParent().layout(true, true);
	}
	
	/**
	 * Create comment editor widget
	 * 
	 * @param note alarm note associated with this widget
	 * @return
	 */
	private AlarmCommentsEditor createEditor(final AlarmComment note)
	{
	   HyperlinkAdapter editAction = new HyperlinkAdapter()
      {
	      @Override
         public void linkActivated(HyperlinkEvent e)
         {
	         editComment(note.getId(), note.getText());
         }
      };
      HyperlinkAdapter deleteAction = new HyperlinkAdapter()
      {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            deleteComment(note.getId());
         }
      };
		final AlarmCommentsEditor e = new AlarmCommentsEditor(editorsArea, toolkit, imageCache, note, editAction, deleteAction);
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

	   editComment(0, ""); //$NON-NLS-1$
	}
	
	/**
    * Edit comment
    */
   private void editComment(final long noteId, String noteText)
   {
      final EditCommentDialog dlg = new EditCommentDialog(getSite().getShell(), noteId , noteText);
      if (dlg.open() != Window.OK)
         return;
      
      new ConsoleJob(Messages.get().AlarmComments_AddCommentJob, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
            return Messages.get().AlarmComments_AddError;
         }
      }.start();
   }
   
   /**
    * Delete comment
    */
   private void deleteComment(final long noteId)
   {
      if (!MessageDialogHelper.openConfirm(getSite().getShell(), Messages.get().AlarmComments_Confirmation, Messages.get().AlarmComments_AckToDeleteComment))
         return;
      
      new ConsoleJob(Messages.get().AlarmComments_DeleteCommentJob, this, Activator.PLUGIN_ID, null) {
         @Override
         protected void runInternal(IProgressMonitor monitor) throws Exception
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
            return Messages.get().AlarmComments_ErrorDeleteAlarmComment;
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
		
		AbstractObject object = session.findObjectById(alarm.getSourceObjectId());
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
