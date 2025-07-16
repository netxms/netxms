/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.events.views;

import java.util.ArrayList;
import java.util.Collection;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.actions.views.helpers.DecoratingActionLabelProvider;
import org.netxms.nxmc.modules.events.views.helpers.RuleClipboard;
import org.netxms.nxmc.modules.events.widgets.RuleEditor;
import org.netxms.nxmc.modules.objects.widgets.helpers.BaseObjectLabelProvider;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.ThemeEngine;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Event processing policy editor
 */
public class EventProcessingPolicyEditor extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(EventProcessingPolicyEditor.class);

   private NXCSession session;
   private boolean policyLocked = false;
   private EventProcessingPolicy policy;
   private SessionListener sessionListener;
   private Map<Long, ServerAction> actions = new HashMap<Long, ServerAction>();
   private String filterText = null;
   private ScrolledComposite scroller;
   private Composite dataArea;
   private List<RuleEditor> ruleEditors = new ArrayList<RuleEditor>();
   private boolean verticalLayout = false;
   private boolean modified = false;
   private Set<RuleEditor> selection;
   private int lastSelectedRule = -1;
   private RuleClipboard clipboard = new RuleClipboard();

   private BaseObjectLabelProvider objectLabelProvider;
   private DecoratingActionLabelProvider actionLabelProvider;

   private Font normalFont;
   private Font boldFont;

   private Image imageAlarm;
   private Image imagePersistentStorage;
   private Image imageExecute;
   private Image imageCancelTimer;
   private Image imageTerminate;
   private Image imageStop;
   private Image imageStartDowntime;
   private Image imageEndDowntime;

   private Action actionHorizontal;
   private Action actionVertical;
   private Action actionSave;
   private Action actionCollapseAll;
   private Action actionExpandAll;
   private Action actionInsertAbove;
   private Action actionInsertBelow;
   private Action actionCut;
   private Action actionCopy;
   private Action actionPaste;
   private Action actionDelete;
   private Action actionEnableRule;
   private Action actionDisableRule;
   private Action actionAddRule;

   /**
    * Create event processing policy editor view
    */
   public EventProcessingPolicyEditor()
   {
      super(LocalizationHelper.getI18n(EventProcessingPolicyEditor.class).tr("Event Processing Policy"), ResourceManager.getImageDescriptor("icons/config-views/epp-editor.png"), "configuration.epp", true);
      session = Registry.getSession();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   public void createContent(Composite parent)
   {
      imageStop = ResourceManager.getImage("icons/epp/stop.png");
      imageAlarm = ResourceManager.getImage("icons/alarm.png");
      imagePersistentStorage = ResourceManager.getImage("icons/epp/update-pstorage.png");
      imageExecute = ResourceManager.getImage("icons/epp/execute.png");
      imageCancelTimer = ResourceManager.getImage("icons/epp/cancel-timer.png");
      imageTerminate = ResourceManager.getImage("icons/epp/terminate-alarm.png");
      imageStartDowntime = ResourceManager.getImage("icons/epp/start-downtime.png");
      imageEndDowntime = ResourceManager.getImage("icons/epp/end-downtime.png");

      objectLabelProvider = new BaseObjectLabelProvider();
      actionLabelProvider = new DecoratingActionLabelProvider();

      scroller = new ScrolledComposite(parent, SWT.V_SCROLL);

      dataArea = new Composite(scroller, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      dataArea.setLayout(layout);
      dataArea.setBackground(ThemeEngine.getBackgroundColor("RuleEditor"));

      scroller.setContent(dataArea);
      scroller.setExpandVertical(true);
      scroller.setExpandHorizontal(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            Rectangle r = scroller.getClientArea();
            scroller.setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
         }
      });

      normalFont = JFaceResources.getDefaultFont();
      boldFont = JFaceResources.getFontRegistry().getBold(JFaceResources.DEFAULT_FONT);

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            processSessionNotification(n);
         }
      };
      session.addListener(sessionListener);

      selection = new TreeSet<RuleEditor>(new Comparator<RuleEditor>() {
         @Override
         public int compare(RuleEditor arg0, RuleEditor arg1)
         {
            return arg0.getRuleNumber() - arg1.getRuleNumber();
         }
      });
      
      createActions();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      openEventProcessingPolicy();
      super.postContentCreate();
   }

   /**
    * Create actions
    */
   private void createActions()
   {
      actionHorizontal = new Action(i18n.tr("&Horizontal layout"), Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            verticalLayout = false;
            updateLayout();
         }
      };
      actionHorizontal.setChecked(!verticalLayout);
      actionHorizontal.setImageDescriptor(ResourceManager.getImageDescriptor("icons/epp/h-layout.png"));

      actionVertical = new Action(i18n.tr("&Vertical layout"), Action.AS_RADIO_BUTTON) {
         @Override
         public void run()
         {
            verticalLayout = true;
            updateLayout();
         }
      };
      actionVertical.setChecked(verticalLayout);
      actionVertical.setImageDescriptor(ResourceManager.getImageDescriptor("icons/epp/v-layout.png"));

      actionSave = new Action(i18n.tr("&Save"), SharedIcons.SAVE) {
         @Override
         public void run()
         {
            savePolicy(false);
         }
      };
      actionSave.setEnabled(false);
      addKeyBinding("M1+S", actionSave);

      actionCollapseAll = new Action(i18n.tr("&Collapse all"), SharedIcons.COLLAPSE_ALL) {
         @Override
         public void run()
         {
            setAllRulesCollapsed(true);
         }
      };

      actionExpandAll = new Action(i18n.tr("&Expand all"), SharedIcons.EXPAND_ALL) {
         @Override
         public void run()
         {
            setAllRulesCollapsed(false);
         }
      };

      actionDelete = new Action(i18n.tr("&Delete"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteSelectedRules();
         }
      };
      actionDelete.setEnabled(false);

      actionInsertAbove = new Action(i18n.tr("Insert &above")) {
         @Override
         public void run()
         {
            insertRule(lastSelectedRule - 1);
         }
      };

      actionInsertBelow = new Action(i18n.tr("Insert &below")) {
         @Override
         public void run()
         {
            insertRule(lastSelectedRule);
         }
      };

      actionCut = new Action(i18n.tr("Cu&t"), SharedIcons.CUT) {
         @Override
         public void run()
         {
            cutRules();
         }
      };
      actionCut.setEnabled(false);

      actionCopy = new Action(i18n.tr("&Copy"), SharedIcons.COPY) {
         @Override
         public void run()
         {
            copyRules();
         }
      };
      actionCopy.setEnabled(false);

      actionPaste = new Action(i18n.tr("&Paste"), SharedIcons.PASTE) {
         @Override
         public void run()
         {
            pasteRules();
         }
      };
      actionPaste.setEnabled(false);

      actionEnableRule = new Action(i18n.tr("E&nable")) {
         @Override
         public void run()
         {
            enableRules(true);
         }
      };

      actionDisableRule = new Action(i18n.tr("D&isable")) {
         @Override
         public void run()
         {
            enableRules(false);
         }
      };

      actionAddRule = new Action(i18n.tr("&Add new rule"), ResourceManager.getImageDescriptor("icons/epp/add-rule.png")) {
         @Override
         public void run()
         {
            insertRule(ruleEditors.size());
         }
      };
   }

   /**
    * Fill local pull-down menu
    * 
    * @param manager Menu manager for pull-down menu
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionExpandAll);
      manager.add(actionCollapseAll);
      manager.add(new Separator());
      manager.add(actionHorizontal);
      manager.add(actionVertical);
   }

   /**
    * Fill local tool bar
    * 
    * @param manager Menu manager for local toolbar
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionAddRule);
      manager.add(actionSave);
      manager.add(new Separator());
      manager.add(actionExpandAll);
      manager.add(actionCollapseAll);
      manager.add(new Separator());
      manager.add(actionHorizontal);
      manager.add(actionVertical);
      manager.add(new Separator());
      manager.add(actionCut);
      manager.add(actionCopy);
      manager.add(actionPaste);
      manager.add(actionDelete);
   }

   /**
    * Open event processing policy
    */
   private void openEventProcessingPolicy()
   {
      Job job = new Job(i18n.tr("Loading event processing policy"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            List<ServerAction> actions = session.getActions();
            synchronized(EventProcessingPolicyEditor.this.actions)
            {
               for(ServerAction a : actions)
               {
                  EventProcessingPolicyEditor.this.actions.put(a.getId(), a);
               }
            }
            policy = session.openEventProcessingPolicy();
            policyLocked = true;
            runInUIThread(new Runnable() {
               @Override
               public void run()
               {
                  initPolicyEditor();
               }
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot load event processing policy");
         }
      };
      job.start();
   }

   /**
    * Init policy editor
    */
   private void initPolicyEditor()
   {
      for(EventProcessingPolicyRule rule : policy.getRules())
      {
         if (isRuleVisible(rule))
         {
            RuleEditor editor = new RuleEditor(dataArea, rule, this);
            ruleEditors.add(editor);
            GridData gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            editor.setLayoutData(gd);
         }
      }
      dataArea.layout();

      Rectangle r = scroller.getClientArea();
      scroller.setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
   }

   /**
    * Update editor's layout
    */
   private void updateLayout()
   {
      for(RuleEditor editor : ruleEditors)
      {
         if (!editor.isDisposed())
            editor.setVerticalLayout(verticalLayout, false);
      }
      updateEditorAreaLayout();
   }

   /**
    * Process session notifications
    * 
    * @param n notification
    */
   private void processSessionNotification(SessionNotification n)
   {
      switch(n.getCode())
      {
         case SessionNotification.ACTION_CREATED:
            synchronized(actions)
            {
               actions.put(n.getSubCode(), (ServerAction)n.getObject());
            }
            break;
         case SessionNotification.ACTION_MODIFIED:
            synchronized(actions)
            {
               actions.put(n.getSubCode(), (ServerAction)n.getObject());
            }
            break;
         case SessionNotification.ACTION_DELETED:
            synchronized(actions)
            {
               actions.remove(n.getSubCode());
            }
            break;
      }
   }

   /**
    * Set all rules to collapsed or expanded state
    * 
    * @param collapsed true to collapse all, false to expand
    */
   private void setAllRulesCollapsed(boolean collapsed)
   {
      for(RuleEditor editor : ruleEditors)
      {
         if (!editor.isDisposed())
            editor.setCollapsed(collapsed, false);
      }
      updateEditorAreaLayout();
   }

   /**
    * Save policy to server
    * 
    * @param unlockAfterSave if true, remove EPP lock after save
    */
   public void savePolicy(final boolean unlockAfterSave)
   {
      actionSave.setEnabled(false);
      if (unlockAfterSave)
         policyLocked = false;
      new Job(i18n.tr("Saving event processing policy"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.saveEventProcessingPolicy(policy);
            if (unlockAfterSave)
               session.closeEventProcessingPolicy();
            runInUIThread(() -> {
               modified = false;
            });
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> actionSave.setEnabled(modified));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save event processing policy");
         }
      }.start();
   }

   /**
    * @see org.eclipse.ui.part.WorkbenchPart#dispose()
    */
   @Override
   public void dispose()
   {
      if (sessionListener != null)
         session.removeListener(sessionListener);

      if (policyLocked)
      {
         new Job(i18n.tr("Closing event processing policy"), null) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               session.closeEventProcessingPolicy();
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot close event processing policy");
            }
         }.start();
      }

      imageStop.dispose();
      imageAlarm.dispose();
      imagePersistentStorage.dispose();
      imageExecute.dispose();
      imageCancelTimer.dispose();
      imageTerminate.dispose();
      imageStartDowntime.dispose();
      imageEndDowntime.dispose();

      objectLabelProvider.dispose();
      actionLabelProvider.dispose();

      modified = false;

      super.dispose();
   }

   /**
    * Update entire editor area layout after change in rule editor windget's size
    */
   public void updateEditorAreaLayout()
   {
      dataArea.layout();
      Rectangle r = scroller.getClientArea();
      scroller.setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
   }

   /**
    * Find server action by ID
    * 
    * @param id action id
    * @return server action object or null
    */
   public ServerAction findActionById(Long id)
   {
      return actions.get(id);
   }

   /**
    * Find server actions for list of Ids
    * 
    * @param idList list of action identifiers
    * @return list of server actions
    */
   public Map<Long, ServerAction> findServerActions(List<Long> idList)
   {
      Map<Long, ServerAction> resultSet = new HashMap<Long, ServerAction>();
      for(Long id : idList)
      {
         ServerAction action = actions.get(id);
         if (action != null)
            resultSet.put(id, action);
      }
      return resultSet;
   }

   /**
    * Return complete actions list
    * 
    * @return actions list
    */
   public Collection<ServerAction> getActions()
   {
      return actions.values();
   }

   /**
    * @return the normalFont
    */
   public Font getNormalFont()
   {
      return normalFont;
   }

   /**
    * @return the boldFont
    */
   public Font getBoldFont()
   {
      return boldFont;
   }

   /**
    * @return the imageAlarm
    */
   public Image getImageAlarm()
   {
      return imageAlarm;
   }

   /**
    * @return the imageExecute
    */
   public Image getImageExecute()
   {
      return imageExecute;
   }

   /**
    * Get "cancel timer" image
    * 
    * @return "cancel timer" image
    */
   public Image getImageCancelTimer()
   {
      return imageCancelTimer;
   }

   /**
    * @return the imageTerminate
    */
   public Image getImageTerminate()
   {
      return imageTerminate;
   }

   /**
    * @return the imageStop
    */
   public Image getImageStop()
   {
      return imageStop;
   }

   /**
    * @return the imageStartDowntime
    */
   public Image getImageStartDowntime()
   {
      return imageStartDowntime;
   }

   /**
    * @return the imageStopDowntime
    */
   public Image getImageEndDowntime()
   {
      return imageEndDowntime;
   }

   /**
    * @return the imageSituation
    */
   public Image getImagePersistentStorage()
   {
      return imagePersistentStorage;
   }

   /**
    * @return the objectLabelProvider
    */
   public BaseObjectLabelProvider getObjectLabelProvider()
   {
      return objectLabelProvider;
   }

   /**
    * @return the actionLabelProvider
    */
   public DecoratingActionLabelProvider getActionLabelProvider()
   {
      return actionLabelProvider;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#isModified()
    */
   @Override
   public boolean isModified()
   {
      return modified;
   }

   /**
    * @param modified the modified to set
    */
   public void setModified(boolean modified)
   {
      this.modified = modified;
      actionSave.setEnabled(modified);
   }

   /**
    * Clear selection
    */
   private void clearSelection()
   {
      for(RuleEditor e : selection)
         e.setSelected(false);
      selection.clear();
      lastSelectedRule = -1;
   }

   /**
    * Set selection to given rule
    * 
    * @param e rule editor
    */
   public void setSelection(RuleEditor e)
   {
      clearSelection();      
      addToSelection(e, false);
   }

   /**
    * Handle drag detect.
    *
    * @param e editor being dragged
    */
   public void onDragDetect(RuleEditor e)
   {
      if (!selection.contains(e))
         setSelection(e);
   }

   /**
    * Add rule to selection
    * 
    * @param e rule editor
    */
   public void addToSelection(RuleEditor e, boolean allFromPrevSelection)
   {
      if (allFromPrevSelection && (lastSelectedRule != -1))
      {
         int direction = Integer.signum(e.getRuleNumber() - lastSelectedRule);
         for(int i = lastSelectedRule + direction; i != e.getRuleNumber(); i += direction)
         {
            RuleEditor r = ruleEditors.get(i - 1);
            selection.add(r);
            r.setSelected(true);
         }
      }
      selection.add(e);
      e.setSelected(true);
      lastSelectedRule = e.getRuleNumber();
      
      onSelectionChange();
   }
   
   /**
    * Remove rule from selection
    * 
    * @param e rule editor
    */
   public void removeFromSelection(RuleEditor e)
   {
      selection.remove(e);
      e.setSelected(false);
      lastSelectedRule = -1;
      
      onSelectionChange();
   }

   /**
    * Internal handler for selection change
    */
   private void onSelectionChange()
   {
      actionDelete.setEnabled(selection.size() > 0);
      actionInsertAbove.setEnabled(selection.size() == 1);
      actionInsertBelow.setEnabled(selection.size() == 1);
      actionCut.setEnabled(selection.size() > 0);
      actionCopy.setEnabled(selection.size() > 0);
      actionPaste.setEnabled((selection.size() == 1) && !clipboard.isEmpty());
   }

   /**
    * Delete selected rules
    */
   private void deleteSelectedRules()
   {
      for(RuleEditor e : selection)
      {
         policy.deleteRule(e.getRule());
         ruleEditors.remove(e);
         e.dispose();
      }

      // Renumber rules
      for(int i = 0; i < ruleEditors.size(); i++)
         ruleEditors.get(i).setRuleNumber(i + 1);

      selection.clear();
      lastSelectedRule = -1;
      onSelectionChange();

      updateEditorAreaLayout();
      setModified(true);
   }

   /**
    * Insert new rule at given position
    * 
    * @param position
    */
   private void insertRule(int position)
   {
      EventProcessingPolicyRule rule = new EventProcessingPolicyRule();
      rule.setRuleNumber(position + 1);
      policy.insertRule(rule, position);

      RuleEditor editor = new RuleEditor(dataArea, rule, this);
      ruleEditors.add(position, editor);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      editor.setLayoutData(gd);

      for(int i = position + 1; i < ruleEditors.size(); i++)
         ruleEditors.get(i).setRuleNumber(i + 1);

      if (position < ruleEditors.size() - 1)
      {
         RuleEditor anchor = null;
         for(int i = position + 1; i < ruleEditors.size(); i++)
            if (!ruleEditors.get(i).isDisposed())
            {
               anchor = ruleEditors.get(i);
               break;
            }
         if (anchor != null)
            editor.moveAbove(anchor);
      }
      updateEditorAreaLayout();

      setModified(true);
   }

   /**
    * Cut selected rules to internal clipboard
    */
   private void cutRules()
   {
      clipboard.clear();
      actionPaste.setEnabled(true);

      for(RuleEditor e : selection)
      {
         clipboard.add(e.getRule());
         policy.deleteRule(e.getRule());
         ruleEditors.remove(e);
         e.dispose();
      }

      // Renumber rules
      for(int i = 0; i < ruleEditors.size(); i++)
         ruleEditors.get(i).setRuleNumber(i + 1);

      selection.clear();
      lastSelectedRule = -1;
      onSelectionChange();

      updateEditorAreaLayout();
      setModified(true);
   }

   /**
    * Copy selected rules to internal clipboard
    */
   private void copyRules()
   {
      clipboard.clear();
      actionPaste.setEnabled(true);

      for(RuleEditor e : selection)
         clipboard.add(new EventProcessingPolicyRule(e.getRule()));
   }

   /**
    * Paste rules from internal clipboard
    */
   private void pasteRules()
   {
      int position = lastSelectedRule;

      RuleEditor anchor = null;
      if (position < ruleEditors.size() - 1)
      {
         for(int i = position; i < ruleEditors.size(); i++)
            if (!ruleEditors.get(i).isDisposed())
            {
               anchor = ruleEditors.get(i);
               break;
            }
      }

      for(EventProcessingPolicyRule rule : clipboard.paste())
      {
         rule.setRuleNumber(position + 1);
         policy.insertRule(rule, position);

         RuleEditor editor = new RuleEditor(dataArea, rule, this);
         ruleEditors.add(position, editor);
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         editor.setLayoutData(gd);

         if (anchor != null)
            editor.moveAbove(anchor);

         position++;
      }

      for(int i = position; i < ruleEditors.size(); i++)
         ruleEditors.get(i).setRuleNumber(i + 1);

      updateEditorAreaLayout();
      setModified(true);
   }
   
   /**
    * Moves rule selection
    * 
    * @param anchor - where the selection is being moved
    */
   public void moveSelection(RuleEditor anchor)
   {
      if (selection.contains(anchor))
         return;
      
      List<RuleEditor> movedRuleEditors = new ArrayList<RuleEditor>();
      for(RuleEditor e : ruleEditors)
      {
         if (!selection.contains(e))
         {
            movedRuleEditors.add(e);
            if (e.equals(anchor))
            {
               RuleEditor curr = anchor;
               for(RuleEditor s : selection)
               {
                  movedRuleEditors.add(s);
                  s.moveBelow(curr);
                  curr = s;
               }
            }
         }
      }

      policy = new EventProcessingPolicy(movedRuleEditors.size());
      int i = 0;
      for(RuleEditor e : movedRuleEditors)
      {
         policy.addRule(e.getRule());
         e.setRuleNumber(++i);
      }

      ruleEditors = movedRuleEditors;

      updateEditorAreaLayout();
      setModified(true);
   }

   /**
    * Enable selected rules
    */
   private void enableRules(boolean enabled)
   {
      for(RuleEditor e : selection)
         e.enableRule(enabled);
   }

   /**
    * Fill context menu for rule
    * 
    * @param manager menu manager
    */
   public void fillRuleContextMenu(IMenuManager manager)
   {
      manager.add(actionEnableRule);
      manager.add(actionDisableRule);
      manager.add(new Separator());
      manager.add(actionInsertAbove);
      manager.add(actionInsertBelow);
      manager.add(new Separator());
      manager.add(actionCut);
      manager.add(actionCopy);
      manager.add(actionPaste);
      manager.add(new Separator());
      manager.add(actionDelete);
   }

   /**
    * Handler for filter modification
    */
   @Override
   protected void onFilterModify()
   {
      filterText = getFilterText().trim().toLowerCase();

      // change editors visibility
      RuleEditor prev = null;
      for(int i = 0; i < ruleEditors.size(); i++)
      {
         RuleEditor e = ruleEditors.get(i);
         boolean visible = isRuleVisible(e.getRule());
         if (!e.isDisposed() && !visible)
         {
            e.dispose();
            selection.remove(e);
         }
         else if (e.isDisposed() && visible)
         {
            e = new RuleEditor(dataArea, e.getRule(), this);
            GridData gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.grabExcessHorizontalSpace = true;
            e.setLayoutData(gd);
            if (prev != null)
               e.moveBelow(prev);
            else
               e.moveAbove(null);
            ruleEditors.set(i, e);
         }
         if (!e.isDisposed())
            prev = e;
      }

      updateEditorAreaLayout();
   }

   /**
    * Check if given rule should be visible
    * 
    * @param rule
    * @return
    */
   private boolean isRuleVisible(EventProcessingPolicyRule rule)
   {
      if ((filterText == null) || filterText.isEmpty())
         return true;

      if (rule.getComments().toLowerCase().contains(filterText))
         return true;

      // check event names
      for(Integer code : rule.getEvents())
      {
         EventTemplate evt = session.findEventTemplateByCode(code);
         if ((evt != null) && evt.getName().toLowerCase().contains(filterText))
            return true;
      }

      // check object names
      for(Long id : rule.getSources())
      {
         String name = session.getObjectName(id);
         if ((name != null) && name.toLowerCase().contains(filterText))
            return true;
      }
      for(Long id : rule.getSourceExclusions())
      {
         String name = session.getObjectName(id);
         if ((name != null) && name.toLowerCase().contains(filterText))
            return true;
      }

      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
      // This method should be called only when save on close is selected, so do unlock after save
      savePolicy(true);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      if (!policyLocked)
      {
         openEventProcessingPolicy();
         return;
      }
      
      if (isModified())
      {
         if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Unsaved Changes"), 
               i18n.tr("Are you sure you want to refresh and lose all not saved changes?")))
         {
            return;
         }
      }
      new Job(i18n.tr("Closing event processing policy"), null) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.closeEventProcessingPolicy();
            runInUIThread(() -> openEventProcessingPolicy());
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot close event processing policy");
         }
      }.start();
   }
}
