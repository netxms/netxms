/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Raden Solutions
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
package org.netxms.nxmc.modules.incidents.views;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.DoubleClickEvent;
import org.eclipse.jface.viewers.IDoubleClickListener;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.constants.IncidentState;
import org.netxms.client.events.Alarm;
import org.netxms.client.events.Incident;
import org.netxms.client.events.IncidentComment;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.users.AbstractUserObject;
import org.netxms.client.users.User;
import org.netxms.nxmc.Memento;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.actions.CopyTableRowsAction;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.layout.DashboardLayout;
import org.netxms.nxmc.base.layout.DashboardLayoutData;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.base.views.ViewNotRestoredException;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.Section;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.alarms.views.AlarmDetails;
import org.netxms.nxmc.modules.incidents.dialogs.EditIncidentCommentDialog;
import org.netxms.nxmc.modules.incidents.widgets.IncidentCommentsEditor;
import org.netxms.nxmc.modules.objects.views.AdHocObjectView;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.modules.users.dialogs.UserSelectionDialog;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.tools.ImageCache;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Incident details view
 */
public class IncidentDetails extends AdHocObjectView
{
   private final I18n i18n = LocalizationHelper.getI18n(IncidentDetails.class);

   // Alarm table columns
   public static final int AL_COLUMN_SEVERITY = 0;
   public static final int AL_COLUMN_STATE = 1;
   public static final int AL_COLUMN_SOURCE = 2;
   public static final int AL_COLUMN_MESSAGE = 3;
   public static final int AL_COLUMN_CREATED = 4;

   private long incidentId;
   private Incident incident;
   private NXCSession session;
   private ImageCache imageCache;
   private BaseObjectLabelProvider objectLabelProvider;
   private Image[] stateImages;

   private Composite content;

   // Overview section
   private CLabel labelId;
   private CLabel labelState;
   private CLabel labelSource;
   private CLabel labelCreated;
   private CLabel labelAssigned;
   private CLabel labelLastChange;
   private CLabel labelResolved;
   private CLabel labelClosed;

   // Details section
   private Text textTitle;
   private Text textDescription;
   private Button buttonSave;

   // Comments section
   private ScrolledComposite commentsScroller;
   private Composite commentsArea;
   private ImageHyperlink linkAddComment;
   private Map<Long, IncidentCommentsEditor> commentEditors = new HashMap<>();

   // Alarms section
   private SortableTableViewer alarmViewer;

   // Actions
   private Action actionAssign;
   private Action actionBlock;
   private Action actionResolve;
   private Action actionClose;
   private CopyTableRowsAction copyAlarmAction;

   /**
    * Create incident details view.
    *
    * @param incidentId incident ID
    * @param contextObject context object ID
    */
   public IncidentDetails(long incidentId, long contextObject)
   {
      super(String.format(LocalizationHelper.getI18n(IncidentDetails.class).tr("Incident [%d]"), incidentId),
            ResourceManager.getImageDescriptor("icons/object-views/incidents.png"),
            "objects.incident-details", contextObject, contextObject, false);
      this.incidentId = incidentId;
      this.session = Registry.getSession();
      this.objectLabelProvider = new BaseObjectLabelProvider();

      stateImages = new Image[5];
      stateImages[IncidentState.OPEN.getValue()] = ResourceManager.getImage("icons/incident-open.png");
      stateImages[IncidentState.IN_PROGRESS.getValue()] = ResourceManager.getImage("icons/incident-in-progress.png");
      stateImages[IncidentState.BLOCKED.getValue()] = ResourceManager.getImage("icons/incident-blocked.png");
      stateImages[IncidentState.RESOLVED.getValue()] = ResourceManager.getImage("icons/incident-resolved.png");
      stateImages[IncidentState.CLOSED.getValue()] = ResourceManager.getImage("icons/incident-closed.png");
   }

   /**
    * Default constructor for cloning.
    */
   protected IncidentDetails()
   {
      super(LocalizationHelper.getI18n(IncidentDetails.class).tr("Incident Details"),
            ResourceManager.getImageDescriptor("icons/object-views/incidents.png"),
            "objects.incident-details", 0, 0, false);
      this.session = Registry.getSession();
      this.objectLabelProvider = new BaseObjectLabelProvider();

      stateImages = new Image[5];
      stateImages[IncidentState.OPEN.getValue()] = ResourceManager.getImage("icons/incident-open.png");
      stateImages[IncidentState.IN_PROGRESS.getValue()] = ResourceManager.getImage("icons/incident-in-progress.png");
      stateImages[IncidentState.BLOCKED.getValue()] = ResourceManager.getImage("icons/incident-blocked.png");
      stateImages[IncidentState.RESOLVED.getValue()] = ResourceManager.getImage("icons/incident-resolved.png");
      stateImages[IncidentState.CLOSED.getValue()] = ResourceManager.getImage("icons/incident-closed.png");
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#cloneView()
    */
   @Override
   public View cloneView()
   {
      IncidentDetails view = (IncidentDetails)super.cloneView();
      view.incidentId = incidentId;
      return view;
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#postClone(org.netxms.nxmc.base.views.View)
    */
   @Override
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

      createOverviewSection(parent);
      createDetailsSection(parent);
      createCommentsSection(parent);
      createAlarmsSection(parent);

      createActions();
      createContextMenu();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionAssign = new Action(i18n.tr("&Assign..."), SharedIcons.USER) {
         @Override
         public void run()
         {
            assignIncident();
         }
      };

      actionBlock = new Action(i18n.tr("&Block...")) {
         @Override
         public void run()
         {
            blockIncident();
         }
      };

      actionResolve = new Action(i18n.tr("&Resolve")) {
         @Override
         public void run()
         {
            resolveIncident();
         }
      };

      actionClose = new Action(i18n.tr("C&lose")) {
         @Override
         public void run()
         {
            closeIncident();
         }
      };

      copyAlarmAction = new CopyTableRowsAction(alarmViewer, true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(org.eclipse.jface.action.IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionAssign);
      manager.add(new Separator());
      manager.add(actionBlock);
      manager.add(actionResolve);
      manager.add(actionClose);
   }

   /**
    * Create context menu for alarm viewer
    */
   private void createContextMenu()
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> fillAlarmContextMenu(m));

      Menu menu = menuMgr.createContextMenu(alarmViewer.getControl());
      alarmViewer.getControl().setMenu(menu);
   }

   /**
    * Fill alarm context menu
    */
   private void fillAlarmContextMenu(IMenuManager manager)
   {
      manager.add(copyAlarmAction);
   }

   /**
    * Create overview section
    */
   private void createOverviewSection(Composite parent)
   {
      final Section section = new Section(parent, i18n.tr("Overview"), true);
      DashboardLayoutData dd = new DashboardLayoutData();
      dd.fill = false;
      section.setLayoutData(dd);

      final Composite clientArea = section.getClient();
      GridLayout layout = new GridLayout();
      layout.numColumns = 4;
      clientArea.setLayout(layout);

      // Row 1: ID and State
      Label label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("ID:"));

      labelId = new CLabel(clientArea, SWT.NONE);
      labelId.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("State:"));

      labelState = new CLabel(clientArea, SWT.NONE);
      labelState.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Row 2: Source and Assigned
      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Source:"));

      labelSource = new CLabel(clientArea, SWT.NONE);
      labelSource.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Assigned:"));

      labelAssigned = new CLabel(clientArea, SWT.NONE);
      labelAssigned.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Row 3: Created and Last Change
      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Created:"));

      labelCreated = new CLabel(clientArea, SWT.NONE);
      labelCreated.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Last change:"));

      labelLastChange = new CLabel(clientArea, SWT.NONE);
      labelLastChange.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Row 4: Resolved and Closed
      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Resolved:"));

      labelResolved = new CLabel(clientArea, SWT.NONE);
      labelResolved.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Closed:"));

      labelClosed = new CLabel(clientArea, SWT.NONE);
      labelClosed.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
   }

   /**
    * Create details section (editable title and description)
    */
   private void createDetailsSection(Composite parent)
   {
      final Section section = new Section(parent, i18n.tr("Details"), true);
      DashboardLayoutData dd = new DashboardLayoutData();
      dd.fill = false;
      section.setLayoutData(dd);

      final Composite clientArea = section.getClient();
      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      clientArea.setLayout(layout);

      // Title row
      Label label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Title:"));

      textTitle = new Text(clientArea, SWT.BORDER);
      textTitle.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      textTitle.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            setDetailsModified(true);
         }
      });

      buttonSave = new Button(clientArea, SWT.PUSH);
      buttonSave.setText(i18n.tr("Save"));
      buttonSave.setEnabled(false);
      GridData gd = new GridData();
      gd.verticalSpan = 2;
      gd.verticalAlignment = SWT.TOP;
      buttonSave.setLayoutData(gd);
      buttonSave.addListener(SWT.Selection, (e) -> saveDetails());

      // Description row
      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Description:"));
      label.setLayoutData(new GridData(SWT.LEFT, SWT.TOP, false, false));

      textDescription = new Text(clientArea, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.WRAP);
      gd = new GridData(SWT.FILL, SWT.FILL, true, false);
      gd.heightHint = 80;
      textDescription.setLayoutData(gd);
      textDescription.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            setDetailsModified(true);
         }
      });
   }

   /**
    * Create comments section
    */
   private void createCommentsSection(Composite parent)
   {
      final Section section = new Section(parent, i18n.tr("Comments"), true);
      final DashboardLayoutData dd = new DashboardLayoutData();
      dd.fill = true;
      section.setLayoutData(dd);
      section.addExpansionListener((e) -> dd.fill = e.getState());

      commentsScroller = new ScrolledComposite(section.getClient(), SWT.V_SCROLL);
      commentsScroller.setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
      commentsScroller.setExpandHorizontal(true);
      commentsScroller.setExpandVertical(true);
      WidgetHelper.setScrollBarIncrement(commentsScroller, SWT.VERTICAL, 20);
      commentsScroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            commentsArea.layout(true, true);
            commentsScroller.setMinSize(commentsArea.computeSize(commentsScroller.getClientArea().width, SWT.DEFAULT));
         }
      });

      commentsArea = new Composite(commentsScroller, SWT.NONE);
      commentsArea.setBackground(commentsScroller.getBackground());
      GridLayout layout = new GridLayout();
      commentsArea.setLayout(layout);

      commentsScroller.setContent(commentsArea);

      linkAddComment = new ImageHyperlink(commentsArea, SWT.NONE);
      linkAddComment.setImage(imageCache.create(ResourceManager.getImageDescriptor("icons/new_comment.png")));
      linkAddComment.setText(i18n.tr("Add comment"));
      linkAddComment.setBackground(commentsArea.getBackground());
      linkAddComment.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addComment();
         }
      });
   }

   /**
    * Create alarms section
    */
   private void createAlarmsSection(Composite parent)
   {
      final Section section = new Section(parent, i18n.tr("Related Alarms"), true);
      final DashboardLayoutData dd = new DashboardLayoutData();
      dd.fill = true;
      section.setLayoutData(dd);
      section.addExpansionListener((e) -> dd.fill = e.getState());

      final String[] names = { i18n.tr("Severity"), i18n.tr("State"), i18n.tr("Source"), i18n.tr("Message"), i18n.tr("Created") };
      final int[] widths = { 90, 90, 150, 300, 150 };
      alarmViewer = new SortableTableViewer(section.getClient(), names, widths, AL_COLUMN_CREATED, SWT.DOWN, SWT.FULL_SELECTION);
      alarmViewer.setContentProvider(new ArrayContentProvider());
      alarmViewer.setLabelProvider(new AlarmListLabelProvider());

      alarmViewer.addDoubleClickListener(new IDoubleClickListener() {
         @Override
         public void doubleClick(DoubleClickEvent event)
         {
            IStructuredSelection selection = alarmViewer.getStructuredSelection();
            if (selection.size() == 1)
            {
               Alarm alarm = (Alarm)selection.getFirstElement();
               openView(new AlarmDetails(alarm.getId(), alarm.getSourceObjectId()));
            }
         }
      });
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
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      new Job(i18n.tr("Loading incident details"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final Incident incidentData = session.getIncident(incidentId);

            // Fetch linked alarm details
            long[] linkedAlarmIds = incidentData.getLinkedAlarmIds();
            final Alarm[] linkedAlarms = new Alarm[linkedAlarmIds.length];
            for(int i = 0; i < linkedAlarmIds.length; i++)
            {
               try
               {
                  linkedAlarms[i] = session.getAlarm(linkedAlarmIds[i]);
               }
               catch(Exception e)
               {
                  linkedAlarms[i] = null;
               }
            }

            runInUIThread(() -> {
               incident = incidentData;
               updateOverview();
               updateDetails();
               updateComments(incidentData.getComments());
               updateAlarms(linkedAlarms);
               updateActionStates();
               updateLayout();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load incident details");
         }
      }.start();
   }

   /**
    * Update overview section
    */
   private void updateOverview()
   {
      labelId.setText(Long.toString(incident.getId()));

      labelState.setImage(stateImages[incident.getState().getValue()]);
      labelState.setText(incident.getState().name());

      AbstractObject object = session.findObjectById(incident.getSourceObjectId());
      if (object != null)
      {
         labelSource.setImage(objectLabelProvider.getImage(object));
         labelSource.setText(object.getObjectName());
      }
      else
      {
         labelSource.setImage(SharedIcons.IMG_UNKNOWN_OBJECT);
         labelSource.setText("[" + incident.getSourceObjectId() + "]");
      }

      updateUserLabel(labelCreated, incident.getCreatedByUser(), incident.getCreationTime());
      updateUserLabel(labelAssigned, incident.getAssignedUserId(), null);
      labelLastChange.setText(DateFormatFactory.getDateTimeFormat().format(incident.getLastChangeTime()));

      if (incident.getResolveTime() != null)
         updateUserLabel(labelResolved, incident.getResolvedByUser(), incident.getResolveTime());
      else
         labelResolved.setText("-");

      if (incident.getCloseTime() != null)
         updateUserLabel(labelClosed, incident.getClosedByUser(), incident.getCloseTime());
      else
         labelClosed.setText("-");
   }

   /**
    * Update user label with user name and optional timestamp
    */
   private void updateUserLabel(final CLabel label, int userId, java.util.Date timestamp)
   {
      if (userId == 0)
      {
         label.setText(timestamp != null ? DateFormatFactory.getDateTimeFormat().format(timestamp) : "-");
         return;
      }

      AbstractUserObject user = session.findUserDBObjectById(userId, new Runnable() {
         @Override
         public void run()
         {
            label.getDisplay().asyncExec(new Runnable() {
               @Override
               public void run()
               {
                  if (!label.isDisposed())
                  {
                     AbstractUserObject user = session.findUserDBObjectById(userId, null);
                     String text = (user != null) ? user.getName() : "[" + userId + "]";
                     if (timestamp != null)
                        text = DateFormatFactory.getDateTimeFormat().format(timestamp) + " by " + text;
                     label.setText(text);
                  }
               }
            });
         }
      });

      String text = (user != null) ? user.getName() : "[" + userId + "]";
      if (timestamp != null)
         text = DateFormatFactory.getDateTimeFormat().format(timestamp) + " by " + text;
      label.setText(text);
   }

   /**
    * Update details section
    */
   private void updateDetails()
   {
      textTitle.setText(incident.getTitle() != null ? incident.getTitle() : "");
      textDescription.setText(incident.getDescription() != null ? incident.getDescription() : "");
      setDetailsModified(false);

      boolean editable = !incident.isClosed();
      textTitle.setEditable(editable);
      textDescription.setEditable(editable);
      buttonSave.setVisible(editable);
   }

   /**
    * Update comments section
    */
   private void updateComments(List<IncidentComment> comments)
   {
      for(IncidentCommentsEditor editor : commentEditors.values())
         editor.dispose();
      commentEditors.clear();

      for(IncidentComment comment : comments)
      {
         IncidentCommentsEditor editor = new IncidentCommentsEditor(commentsArea, imageCache, comment);
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         editor.setLayoutData(gd);
         editor.moveBelow(linkAddComment);
         commentEditors.put(comment.getId(), editor);
      }

      linkAddComment.setVisible(!incident.isClosed());
   }

   /**
    * Update alarms section
    */
   private void updateAlarms(Alarm[] alarms)
   {
      // Filter out null entries (alarms that couldn't be fetched)
      java.util.List<Alarm> validAlarms = new java.util.ArrayList<>();
      for(Alarm alarm : alarms)
      {
         if (alarm != null)
            validAlarms.add(alarm);
      }
      alarmViewer.setInput(validAlarms.toArray());
   }

   /**
    * Update action states
    */
   private void updateActionStates()
   {
      boolean canModify = incident != null && !incident.isClosed();
      actionAssign.setEnabled(canModify);
      actionBlock.setEnabled(canModify && incident.getState() != IncidentState.BLOCKED);
      actionResolve.setEnabled(canModify && !incident.isResolved());
      actionClose.setEnabled(incident != null && !incident.isClosed());
   }

   /**
    * Update layout
    */
   private void updateLayout()
   {
      content.layout(true, true);
      commentsArea.layout(true, true);
      commentsScroller.setMinSize(commentsArea.computeSize(commentsScroller.getClientArea().width, SWT.DEFAULT));
   }

   /**
    * Set details modified state
    */
   private void setDetailsModified(boolean modified)
   {
      buttonSave.setEnabled(modified && incident != null && !incident.isClosed());
   }

   /**
    * Save details (title and description)
    */
   private void saveDetails()
   {
      final String title = textTitle.getText().trim();
      final String description = textDescription.getText().trim();

      if (title.isEmpty())
      {
         MessageDialogHelper.openWarning(getWindow().getShell(), i18n.tr("Warning"), i18n.tr("Title cannot be empty"));
         return;
      }

      new Job(i18n.tr("Updating incident"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.updateIncident(incidentId, title, description.isEmpty() ? null : description);
            runInUIThread(() -> {
               setDetailsModified(false);
               refresh();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update incident");
         }
      }.start();
   }

   /**
    * Assign incident to user
    */
   private void assignIncident()
   {
      UserSelectionDialog dialog = new UserSelectionDialog(getWindow().getShell(), User.class);
      dialog.enableMultiSelection(false);
      if (dialog.open() != Window.OK)
         return;

      final int userId = dialog.getSelection()[0].getId();
      new Job(i18n.tr("Assigning incident"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.assignIncident(incidentId, userId);
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot assign incident");
         }
      }.start();
   }

   /**
    * Block incident
    */
   private void blockIncident()
   {
      EditIncidentCommentDialog dialog = new EditIncidentCommentDialog(getWindow().getShell(), null,
            i18n.tr("Block Incident"), i18n.tr("Reason for blocking"));
      if (dialog.open() != Window.OK)
         return;

      final String comment = dialog.getText();
      if (comment.trim().isEmpty())
      {
         MessageDialogHelper.openWarning(getWindow().getShell(), i18n.tr("Warning"),
               i18n.tr("Comment is required when blocking an incident"));
         return;
      }

      new Job(i18n.tr("Blocking incident"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.changeIncidentState(incidentId, IncidentState.BLOCKED, comment);
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot block incident");
         }
      }.start();
   }

   /**
    * Resolve incident
    */
   private void resolveIncident()
   {
      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Resolve Incident"),
            i18n.tr("Are you sure you want to resolve this incident? All linked alarms will also be resolved.")))
         return;

      new Job(i18n.tr("Resolving incident"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.resolveIncident(incidentId);
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot resolve incident");
         }
      }.start();
   }

   /**
    * Close incident
    */
   private void closeIncident()
   {
      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Close Incident"),
            i18n.tr("Are you sure you want to close this incident?")))
         return;

      new Job(i18n.tr("Closing incident"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.closeIncident(incidentId);
            runInUIThread(() -> refresh());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot close incident");
         }
      }.start();
   }

   /**
    * Add comment
    */
   private void addComment()
   {
      EditIncidentCommentDialog dialog = new EditIncidentCommentDialog(getWindow().getShell(), null);
      if (dialog.open() == Window.OK)
      {
         final String comment = dialog.getText();
         new Job(i18n.tr("Adding incident comment"), this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.addIncidentComment(incidentId, comment);
               runInUIThread(() -> refresh());
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot add comment");
            }
         }.start();
      }
   }

   /**
    * @see org.netxms.nxmc.base.views.View#dispose()
    */
   @Override
   public void dispose()
   {
      imageCache.dispose();
      objectLabelProvider.dispose();
      super.dispose();
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
      if (getObjectId() == 0 && getContextId() == 0)
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
      memento.set("incidentId", incidentId);
   }

   /**
    * @see org.netxms.nxmc.base.views.ViewWithContext#restoreState(org.netxms.nxmc.Memento)
    */
   @Override
   public void restoreState(Memento memento) throws ViewNotRestoredException
   {
      super.restoreState(memento);
      incidentId = memento.getAsLong("incidentId", 0);
      setName(String.format(LocalizationHelper.getI18n(IncidentDetails.class).tr("Incident [%d]"), incidentId));
      if (incidentId == 0)
         throw(new ViewNotRestoredException(i18n.tr("Invalid incident id")));
   }

   /**
    * Label provider for alarm list in incident details
    */
   private class AlarmListLabelProvider extends org.eclipse.jface.viewers.LabelProvider
         implements org.eclipse.jface.viewers.ITableLabelProvider
   {
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         if (columnIndex == AL_COLUMN_SEVERITY)
         {
            Alarm alarm = (Alarm)element;
            return StatusDisplayInfo.getStatusImage(alarm.getCurrentSeverity());
         }
         return null;
      }

      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         Alarm alarm = (Alarm)element;
         switch(columnIndex)
         {
            case AL_COLUMN_SEVERITY:
               return StatusDisplayInfo.getStatusText(alarm.getCurrentSeverity());
            case AL_COLUMN_STATE:
               return getAlarmState(alarm);
            case AL_COLUMN_SOURCE:
               return session.getObjectName(alarm.getSourceObjectId());
            case AL_COLUMN_MESSAGE:
               return alarm.getMessage();
            case AL_COLUMN_CREATED:
               return DateFormatFactory.getDateTimeFormat().format(alarm.getCreationTime());
         }
         return "";
      }

      private String getAlarmState(Alarm alarm)
      {
         switch(alarm.getState())
         {
            case Alarm.STATE_OUTSTANDING:
               return i18n.tr("Outstanding");
            case Alarm.STATE_ACKNOWLEDGED:
               return alarm.isSticky() ? i18n.tr("Acknowledged (sticky)") : i18n.tr("Acknowledged");
            case Alarm.STATE_RESOLVED:
               return i18n.tr("Resolved");
            case Alarm.STATE_TERMINATED:
               return i18n.tr("Terminated");
            default:
               return i18n.tr("Unknown");
         }
      }
   }
}
