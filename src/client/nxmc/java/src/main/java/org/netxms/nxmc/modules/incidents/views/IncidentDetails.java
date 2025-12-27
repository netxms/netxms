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
   private NXCSession session = Registry.getSession();
   private ImageCache imageCache;
   private BaseObjectLabelProvider objectLabelProvider = new BaseObjectLabelProvider();
   private Image[] stateImages;

   private Composite content;

   // Layout components
   private ScrolledComposite leftScroller;
   private Composite leftColumn;
   private Composite rightColumn;

   // Header section (title + save)
   private Text textTitle;
   private Button buttonSave;

   // Description section
   private Text textDescription;

   // Overview section (right sidebar)
   private CLabel labelId;
   private CLabel labelState;
   private CLabel labelSource;
   private CLabel labelCreated;
   private CLabel labelAssigned;
   private CLabel labelLastChange;
   private CLabel labelResolved;
   private CLabel labelClosed;

   // Comments section (left column)
   private Composite commentsArea;
   private ImageHyperlink linkAddComment;
   private Map<Long, IncidentCommentsEditor> commentEditors = new HashMap<>();

   // Alarms section (left column)
   private SortableTableViewer alarmViewer;

   // Action buttons (right sidebar)
   private Button buttonBlock;
   private Button buttonResolve;
   private Button buttonClose;
   private Button buttonAssign;

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
   }

   /**
    * Default constructor for cloning.
    */
   protected IncidentDetails()
   {
      super(LocalizationHelper.getI18n(IncidentDetails.class).tr("Incident Details"),
            ResourceManager.getImageDescriptor("icons/object-views/incidents.png"),
            "objects.incident-details", 0, 0, false);
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
      imageCache = new ImageCache(parent);
      content = parent;

      stateImages = new Image[5];
      stateImages[IncidentState.OPEN.getValue()] = imageCache.create(ResourceManager.getImageDescriptor("icons/incidents/incident-open.png"));
      stateImages[IncidentState.IN_PROGRESS.getValue()] = imageCache.create(ResourceManager.getImageDescriptor("icons/incidents/incident-in-progress.png"));
      stateImages[IncidentState.BLOCKED.getValue()] = imageCache.create(ResourceManager.getImageDescriptor("icons/incidents/incident-blocked.png"));
      stateImages[IncidentState.RESOLVED.getValue()] = imageCache.create(ResourceManager.getImageDescriptor("icons/incidents/incident-resolved.png"));
      stateImages[IncidentState.CLOSED.getValue()] = imageCache.create(ResourceManager.getImageDescriptor("icons/incidents/incident-closed.png"));

      // Main layout: vertical stack (header, description, content area)
      GridLayout layout = new GridLayout();
      layout.marginWidth = WidgetHelper.OUTER_SPACING;
      layout.marginHeight = WidgetHelper.OUTER_SPACING;
      layout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      parent.setLayout(layout);

      createHeaderSection(parent);
      createDescriptionSection(parent);
      createContentArea(parent);

      createActions();
      createContextMenu();
   }

   /**
    * Create header section with title and save button
    */
   private void createHeaderSection(Composite parent)
   {
      Composite header = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout(2, false);
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      header.setLayout(layout);
      header.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      textTitle = new Text(header, SWT.BORDER | SWT.SINGLE);
      textTitle.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      textTitle.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            setDetailsModified(true);
         }
      });

      buttonSave = new Button(header, SWT.PUSH);
      buttonSave.setText(i18n.tr("Save"));
      buttonSave.setEnabled(false);
      buttonSave.addListener(SWT.Selection, (e) -> saveDetails());
   }

   /**
    * Create description section
    */
   private void createDescriptionSection(Composite parent)
   {
      textDescription = new Text(parent, SWT.BORDER | SWT.MULTI | SWT.V_SCROLL | SWT.WRAP);
      GridData gd = new GridData(SWT.FILL, SWT.CENTER, true, false);
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
    * Create two-column content area
    */
   private void createContentArea(Composite parent)
   {
      Composite contentArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout(2, false);
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
      contentArea.setLayout(layout);
      contentArea.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));

      // Left column with ScrolledComposite (scrollable)
      leftScroller = new ScrolledComposite(contentArea, SWT.V_SCROLL);
      leftScroller.setExpandHorizontal(true);
      leftScroller.setExpandVertical(true);
      GridData leftGd = new GridData(SWT.FILL, SWT.FILL, true, true);
      leftGd.widthHint = 500;
      leftScroller.setLayoutData(leftGd);
      WidgetHelper.setScrollBarIncrement(leftScroller, SWT.VERTICAL, 20);
      leftScroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            leftColumn.layout(true, true);
            leftScroller.setMinSize(leftColumn.computeSize(leftScroller.getClientArea().width, SWT.DEFAULT));
         }
      });

      leftColumn = new Composite(leftScroller, SWT.NONE);
      GridLayout leftLayout = new GridLayout();
      leftLayout.marginWidth = 0;
      leftLayout.marginHeight = 0;
      leftLayout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      leftColumn.setLayout(leftLayout);
      leftScroller.setContent(leftColumn);

      // Right column (fixed sidebar, no scroll)
      rightColumn = new Composite(contentArea, SWT.NONE);
      GridLayout rightLayout = new GridLayout();
      rightLayout.marginWidth = 0;
      rightLayout.marginHeight = 0;
      rightLayout.verticalSpacing = WidgetHelper.OUTER_SPACING;
      rightColumn.setLayout(rightLayout);
      GridData rightGd = new GridData(SWT.FILL, SWT.TOP, false, false);
      rightGd.widthHint = 280;
      rightColumn.setLayoutData(rightGd);

      // Create sections in the appropriate columns
      createCommentsSection(leftColumn);
      createAlarmsSection(leftColumn);
      createOverviewSection(rightColumn);
      createActionsSection(rightColumn);
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

      actionBlock = new Action(i18n.tr("&Block..."), ResourceManager.getImageDescriptor("icons/incidents/incident-blocked.png")) {
         @Override
         public void run()
         {
            blockIncident();
         }
      };

      actionResolve = new Action(i18n.tr("&Resolve"), ResourceManager.getImageDescriptor("icons/incidents/incident-resolved.png")) {
         @Override
         public void run()
         {
            resolveIncident();
         }
      };

      actionClose = new Action(i18n.tr("C&lose"), ResourceManager.getImageDescriptor("icons/incidents/incident-closed.png")) {
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
    * Create overview section (right sidebar)
    */
   private void createOverviewSection(Composite parent)
   {
      final Section section = new Section(parent, i18n.tr("Overview"), false);
      section.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      final Composite clientArea = section.getClient();
      GridLayout layout = new GridLayout(2, false);
      clientArea.setLayout(layout);

      // ID
      Label label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("ID:"));
      labelId = new CLabel(clientArea, SWT.NONE);
      labelId.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // State
      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("State:"));
      labelState = new CLabel(clientArea, SWT.NONE);
      labelState.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Source
      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Source:"));
      labelSource = new CLabel(clientArea, SWT.NONE);
      labelSource.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Assigned
      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Assigned:"));
      labelAssigned = new CLabel(clientArea, SWT.NONE);
      labelAssigned.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Created
      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Created:"));
      labelCreated = new CLabel(clientArea, SWT.NONE);
      labelCreated.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Last Change
      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Last change:"));
      labelLastChange = new CLabel(clientArea, SWT.NONE);
      labelLastChange.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Resolved
      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Resolved:"));
      labelResolved = new CLabel(clientArea, SWT.NONE);
      labelResolved.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // Closed
      label = new Label(clientArea, SWT.NONE);
      label.setText(i18n.tr("Closed:"));
      labelClosed = new CLabel(clientArea, SWT.NONE);
      labelClosed.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
   }

   /**
    * Create actions section with buttons (right sidebar)
    */
   private void createActionsSection(Composite parent)
   {
      final Section section = new Section(parent, i18n.tr("Actions"), false);
      section.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      final Composite clientArea = section.getClient();
      GridLayout layout = new GridLayout(2, true);
      clientArea.setLayout(layout);

      buttonBlock = new Button(clientArea, SWT.PUSH);
      buttonBlock.setText(i18n.tr("Block..."));
      buttonBlock.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      buttonBlock.addListener(SWT.Selection, (e) -> blockIncident());

      buttonResolve = new Button(clientArea, SWT.PUSH);
      buttonResolve.setText(i18n.tr("Resolve"));
      buttonResolve.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      buttonResolve.addListener(SWT.Selection, (e) -> resolveIncident());

      buttonClose = new Button(clientArea, SWT.PUSH);
      buttonClose.setText(i18n.tr("Close"));
      buttonClose.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      buttonClose.addListener(SWT.Selection, (e) -> closeIncident());

      buttonAssign = new Button(clientArea, SWT.PUSH);
      buttonAssign.setText(i18n.tr("Assign..."));
      buttonAssign.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));
      buttonAssign.addListener(SWT.Selection, (e) -> assignIncident());
   }

   /**
    * Create comments section (left column)
    */
   private void createCommentsSection(Composite parent)
   {
      final Section section = new Section(parent, i18n.tr("Comments"), false);
      section.setLayoutData(new GridData(SWT.FILL, SWT.TOP, true, false));

      commentsArea = section.getClient();
      commentsArea.setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      commentsArea.setLayout(layout);

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
    * Create alarms section (left column)
    */
   private void createAlarmsSection(Composite parent)
   {
      final Section section = new Section(parent, i18n.tr("Related Alarms"), false);
      GridData sectionGd = new GridData(SWT.FILL, SWT.TOP, true, false);
      sectionGd.heightHint = 200;
      section.setLayoutData(sectionGd);

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
      buttonSave.setEnabled(false);
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
      boolean canBlock = canModify && incident.getState() != IncidentState.BLOCKED;
      boolean canResolve = canModify && !incident.isResolved();
      boolean canClose = incident != null && !incident.isClosed();

      // Update toolbar actions
      actionAssign.setEnabled(canModify);
      actionBlock.setEnabled(canBlock);
      actionResolve.setEnabled(canResolve);
      actionClose.setEnabled(canClose);

      // Update sidebar buttons
      buttonAssign.setEnabled(canModify);
      buttonBlock.setEnabled(canBlock);
      buttonResolve.setEnabled(canResolve);
      buttonClose.setEnabled(canClose);
   }

   /**
    * Update layout
    */
   private void updateLayout()
   {
      content.layout(true, true);
      leftColumn.layout(true, true);
      leftScroller.setMinSize(leftColumn.computeSize(leftScroller.getClientArea().width, SWT.DEFAULT));
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
