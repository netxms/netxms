/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
import java.util.Arrays;
import java.util.Collection;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeSet;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.IMenuManager;
import org.eclipse.jface.action.IToolBarManager;
import org.eclipse.jface.action.Separator;
import org.eclipse.jface.action.ToolBarManager;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.ToolBar;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.UserAccessRights;
import org.netxms.client.events.EPPChainCaller;
import org.netxms.client.events.EPPSaveResult;
import org.netxms.client.events.EventProcessingPolicy;
import org.netxms.client.events.EventProcessingPolicyChain;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.ConfigurationView;
import org.netxms.nxmc.base.widgets.MessageArea;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.actions.views.helpers.DecoratingActionLabelProvider;
import org.netxms.nxmc.modules.events.dialogs.EPPConflictDialog;
import org.netxms.nxmc.modules.events.dialogs.EppChainPropertiesDialog;
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
 * Event processing policy editor. One tab per rule chain; rules of a chain are loaded from the server when its tab is first
 * opened and saved per chain.
 */
public class EventProcessingPolicyEditor extends ConfigurationView
{
   private final I18n i18n = LocalizationHelper.getI18n(EventProcessingPolicyEditor.class);

   private NXCSession session;
   private EventProcessingPolicy policy;
   private SessionListener sessionListener;
   private Map<Long, ServerAction> actions = new HashMap<Long, ServerAction>();
   private String filterText = null;
   private CTabFolder tabFolder;
   private List<ChainTab> chainTabs = new ArrayList<ChainTab>();
   private boolean globalRights = false;
   private boolean verticalLayout = false;
   private boolean modified = false;
   private Set<Integer> dirtyChains = new HashSet<Integer>();
   private int localChangesInProgress = 0;   // Server round-trips started by this editor; their notifications are not echoed back
   private RuleClipboard clipboard = new RuleClipboard();

   private BaseObjectLabelProvider objectLabelProvider;
   private DecoratingActionLabelProvider actionLabelProvider;

   private Font normalFont;
   private Font boldFont;
   private Font italicFont;

   private Image imageAlarm;
   private Image imagePersistentStorage;
   private Image imageExecute;
   private Image imageCancelTimer;
   private Image imageTerminate;
   private Image imageStop;
   private Image imageStartDowntime;
   private Image imageEndDowntime;
   private Image imageLogEvent;
   private Image imageNoLogEvent;
   private Image imageError;
   private Image imageChain;

   private Action actionHorizontal;
   private Action actionVertical;
   private Action actionSave;
   private Action actionSaveAll;
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
   private Action actionExplain;
   private Action actionNewChain;
   private Action actionChainProperties;
   private Action actionDeleteChain;

   /**
    * Editing surface for one rule chain (one tab in the editor)
    */
   private class ChainTab
   {
      EventProcessingPolicyChain chain;
      CTabItem tabItem;
      ScrolledComposite scroller;
      Composite dataArea;
      List<RuleEditor> ruleEditors = new ArrayList<RuleEditor>();
      Set<RuleEditor> selection;
      int lastSelectedRule = -1;
      boolean loading = false;

      ChainTab(EventProcessingPolicyChain chain)
      {
         this.chain = chain;

         selection = new TreeSet<RuleEditor>(new Comparator<RuleEditor>() {
            @Override
            public int compare(RuleEditor arg0, RuleEditor arg1)
            {
               return arg0.getRuleNumber() - arg1.getRuleNumber();
            }
         });

         tabItem = new CTabItem(tabFolder, SWT.NONE);
         tabItem.setImage(imageChain);
         updateTabText();

         scroller = new ScrolledComposite(tabFolder, SWT.V_SCROLL);
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

         tabItem.setControl(scroller);
         tabItem.setData(this);

         if (chain.isLoaded())
            showRules();
      }

      /**
       * @return chain ID
       */
      int getChainId()
      {
         return chain.getId();
      }

      /**
       * Check if the current user can edit this chain
       */
      boolean isEditable()
      {
         return globalRights || chain.isEditable();
      }

      /**
       * Get chain name for display
       */
      String getName()
      {
         return chain.isMain() ? i18n.tr("Main") : chain.getName();
      }

      /**
       * Update tab title and tooltip from chain metadata
       */
      void updateTabText()
      {
         String name = getName();
         if (!isEditable())
            name = String.format(i18n.tr("%s (read-only)"), name);
         boolean dirty = dirtyChains.contains(chain.getId());
         tabItem.setText(dirty ? "*" + name : name);
         tabItem.setFont(dirty ? italicFont : normalFont);
         if (chain.isMain())
         {
            tabItem.setToolTipText(i18n.tr("Main rule chain"));
         }
         else
         {
            String description = ((chain.getDescription() != null) && !chain.getDescription().isEmpty()) ? chain.getDescription() + " - " : "";
            tabItem.setToolTipText(description + String.format(i18n.tr("called by %d rule(s)"), chain.getCallerCount()));
         }
      }

      /**
       * Load rules of this chain from server unless already loaded or being loaded
       */
      void load()
      {
         if (chain.isLoaded() || loading)
            return;

         loading = true;
         new Job(i18n.tr("Loading event processing policy chain"), EventProcessingPolicyEditor.this) {
            @Override
            protected void run(IProgressMonitor monitor) throws Exception
            {
               final EventProcessingPolicyChain loadedChain = session.getEventProcessingPolicyChain(chain.getId());
               runInUIThread(() -> {
                  if (tabItem.isDisposed())
                     return;
                  replaceChain(loadedChain);
                  showRules();
                  onSelectionChange();
               });
            }

            @Override
            protected void jobFinalize()
            {
               runInUIThread(() -> loading = false);
            }

            @Override
            protected String getErrorMessage()
            {
               return i18n.tr("Cannot load event processing policy chain");
            }
         }.start();
      }

      /**
       * Replace chain object with one loaded from server
       *
       * @param loadedChain chain loaded from server
       */
      void replaceChain(EventProcessingPolicyChain loadedChain)
      {
         chain = loadedChain;
         policy.putChain(loadedChain);
         updateTabText();
      }

      /**
       * Build rule editors for the loaded rules
       */
      void showRules()
      {
         clearSelection();
         int number = 1;
         for(EventProcessingPolicyRule rule : chain.getRules())
            rule.setRuleNumber(number++);
         rebuildFilteredEditors();
         updateLayout();
      }

      /**
       * Create rule editor at given position within this tab
       */
      RuleEditor addRuleEditor(EventProcessingPolicyRule rule, int position)
      {
         RuleEditor ruleEditor = new RuleEditor(dataArea, rule, EventProcessingPolicyEditor.this);
         ruleEditors.add(position, ruleEditor);
         GridData gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         ruleEditor.setLayoutData(gd);
         return ruleEditor;
      }

      /**
       * Rebuild editors according to current filter
       */
      void rebuildFilteredEditors()
      {
         for(RuleEditor e : ruleEditors)
            if (!e.isDisposed())
               e.dispose();
         ruleEditors.clear();
         if (chain.isLoaded())
         {
            for(EventProcessingPolicyRule rule : chain.getRules())
            {
               if (isRuleVisible(rule))
                  addRuleEditor(rule, ruleEditors.size());
            }
         }
      }

      /**
       * Update scrolled area layout
       */
      void updateLayout()
      {
         dataArea.layout();
         Rectangle r = scroller.getClientArea();
         scroller.setMinSize(dataArea.computeSize(r.width, SWT.DEFAULT));
      }

      /**
       * Renumber rules according to their current positions
       */
      void renumberRules()
      {
         for(int i = 0; i < ruleEditors.size(); i++)
            ruleEditors.get(i).setRuleNumber(i + 1);
      }

      /**
       * Clear rule selection
       */
      void clearSelection()
      {
         for(RuleEditor e : selection)
            e.setSelected(false);
         selection.clear();
         lastSelectedRule = -1;
      }

      /**
       * Dispose tab and all its editors
       */
      void dispose()
      {
         for(RuleEditor e : ruleEditors)
            if (!e.isDisposed())
               e.dispose();
         ruleEditors.clear();
         selection.clear();
         tabItem.dispose();
         scroller.dispose();
      }
   }

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
      imageLogEvent = ResourceManager.getImage("icons/epp/log-event.png");
      imageNoLogEvent = ResourceManager.getImage("icons/epp/no-log-event.png");
      imageError = ResourceManager.getImage("icons/messages/error.png");
      imageChain = ResourceManager.getImage("icons/config-views/epp-editor.png");

      objectLabelProvider = new BaseObjectLabelProvider();
      actionLabelProvider = new DecoratingActionLabelProvider();

      globalRights = (session.getUserSystemRights() & UserAccessRights.SYSTEM_ACCESS_EPP) != 0;

      normalFont = JFaceResources.getDefaultFont();
      boldFont = JFaceResources.getFontRegistry().getBold(JFaceResources.DEFAULT_FONT);
      italicFont = JFaceResources.getFontRegistry().getItalic(JFaceResources.DEFAULT_FONT);

      createActions();

      tabFolder = new CTabFolder(parent, SWT.TOP | SWT.BORDER);
      WidgetHelper.disableTabFolderSelectionBar(tabFolder);
      tabFolder.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            onTabSelectionChange();
         }
      });

      // Tab row: "new chain" right after the last tab, chain properties and deletion at the right edge
      Composite topRightControl = new Composite(tabFolder, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      topRightControl.setLayout(layout);
      ToolBarManager newChainToolBarManager = new ToolBarManager(SWT.FLAT | SWT.RIGHT);
      newChainToolBarManager.add(actionNewChain);
      ToolBar newChainToolBar = newChainToolBarManager.createControl(topRightControl);
      newChainToolBar.setLayoutData(new GridData(SWT.LEFT, SWT.FILL, true, true));
      ToolBarManager chainToolBarManager = new ToolBarManager(SWT.FLAT | SWT.RIGHT);
      chainToolBarManager.add(actionChainProperties);
      chainToolBarManager.add(actionDeleteChain);
      ToolBar chainToolBar = chainToolBarManager.createControl(topRightControl);
      chainToolBar.setLayoutData(new GridData(SWT.RIGHT, SWT.FILL, false, true));
      tabFolder.setTopRight(topRightControl, SWT.FILL);

      sessionListener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            processSessionNotification(n);
         }
      };
      session.addListener(sessionListener);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#postContentCreate()
    */
   @Override
   protected void postContentCreate()
   {
      reloadPolicy();
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
            ChainTab tab = activeTab();
            if (tab != null)
               saveChains(Arrays.asList(tab));
         }
      };
      actionSave.setEnabled(false);
      addKeyBinding("M1+S", actionSave);

      actionSaveAll = new Action(i18n.tr("Save a&ll"), SharedIcons.SAVE_ALL) {
         @Override
         public void run()
         {
            saveAllChains();
         }
      };
      actionSaveAll.setEnabled(false);

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
            ChainTab tab = activeTab();
            if (tab != null)
               insertRule(tab, tab.lastSelectedRule - 1);
         }
      };

      actionInsertBelow = new Action(i18n.tr("Insert &below")) {
         @Override
         public void run()
         {
            ChainTab tab = activeTab();
            if (tab != null)
               insertRule(tab, tab.lastSelectedRule);
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
            ChainTab tab = activeTab();
            if (tab != null)
               insertRule(tab, tab.ruleEditors.size());
         }
      };

      actionExplain = new Action(i18n.tr("E&xplain")) {
         @Override
         public void run()
         {
            explainRule();
         }
      };

      actionNewChain = new Action(i18n.tr("New c&hain..."), SharedIcons.ADD_OBJECT) {
         @Override
         public void run()
         {
            createChain();
         }
      };
      actionNewChain.setEnabled(globalRights);

      actionChainProperties = new Action(i18n.tr("Chain p&roperties..."), SharedIcons.PROPERTIES) {
         @Override
         public void run()
         {
            editChainProperties();
         }
      };

      actionDeleteChain = new Action(i18n.tr("Delete chai&n"), SharedIcons.DELETE_OBJECT) {
         @Override
         public void run()
         {
            deleteChain();
         }
      };
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalMenu(IMenuManager)
    */
   @Override
   protected void fillLocalMenu(IMenuManager manager)
   {
      manager.add(actionSave);
      manager.add(actionSaveAll);
      manager.add(new Separator());
      manager.add(actionNewChain);
      manager.add(actionChainProperties);
      manager.add(actionDeleteChain);
      manager.add(new Separator());
      manager.add(actionExpandAll);
      manager.add(actionCollapseAll);
      manager.add(new Separator());
      manager.add(actionHorizontal);
      manager.add(actionVertical);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#fillLocalToolBar(IToolBarManager)
    */
   @Override
   protected void fillLocalToolBar(IToolBarManager manager)
   {
      manager.add(actionAddRule);
      manager.add(actionSave);
      manager.add(actionSaveAll);
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
    * Get currently active chain tab
    *
    * @return active tab or null
    */
   private ChainTab activeTab()
   {
      CTabItem item = tabFolder.getSelection();
      return (item != null) ? (ChainTab)item.getData() : null;
   }

   /**
    * Get chain tab owning given rule
    *
    * @param rule policy rule
    * @return owning chain tab or null
    */
   private ChainTab tabForRule(EventProcessingPolicyRule rule)
   {
      return tabForChain(rule.getChainId());
   }

   /**
    * Get tab of given chain
    *
    * @param chainId chain ID (0 = main chain)
    * @return chain tab or null
    */
   private ChainTab tabForChain(int chainId)
   {
      for(ChainTab tab : chainTabs)
         if (tab.getChainId() == chainId)
            return tab;
      return null;
   }

   /**
    * Find chain by ID in loaded policy
    *
    * @param chainId chain ID
    * @return chain or null
    */
   public EventProcessingPolicyChain findChain(int chainId)
   {
      return (policy != null) ? policy.findChain(chainId) : null;
   }

   /**
    * Get display name of chain with given ID
    *
    * @param chainId chain ID
    * @return chain name, localized name of the main chain, or chain ID in brackets if the chain is unknown
    */
   public String getChainName(int chainId)
   {
      if (chainId == 0)
         return i18n.tr("Main");
      EventProcessingPolicyChain chain = findChain(chainId);
      return (chain != null) ? chain.getName() : "[" + chainId + "]";
   }

   /**
    * Get chains of loaded policy
    *
    * @return list of chains readable by current user
    */
   public List<EventProcessingPolicyChain> getChains()
   {
      return (policy != null) ? policy.getChains() : new ArrayList<EventProcessingPolicyChain>();
   }

   /**
    * Build editor tabs for loaded policy (main chain first, other chains sorted by name) and load the first tab
    */
   private void initPolicyEditor()
   {
      for(ChainTab tab : chainTabs)
         tab.dispose();
      chainTabs.clear();

      List<EventProcessingPolicyChain> chains = new ArrayList<>(policy.getChains());
      chains.sort((c1, c2) -> {
         if (c1.isMain() != c2.isMain())
            return c1.isMain() ? -1 : 1;
         return c1.getName().compareToIgnoreCase(c2.getName());
      });
      for(EventProcessingPolicyChain chain : chains)
         chainTabs.add(new ChainTab(chain));

      if (!chainTabs.isEmpty())
         tabFolder.setSelection(chainTabs.get(0).tabItem);
      onTabSelectionChange();
   }

   /**
    * Update editor's layout
    */
   private void updateLayout()
   {
      for(ChainTab tab : chainTabs)
         for(RuleEditor ruleEditor : tab.ruleEditors)
            if (!ruleEditor.isDisposed())
               ruleEditor.setVerticalLayout(verticalLayout, false);
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
         case SessionNotification.EPP_RULES_CHANGED:
            getDisplay().asyncExec(() -> onRulesChangedOnServer((int)n.getSubCode()));
            break;
         case SessionNotification.EPP_CHAIN_UPDATED:
         case SessionNotification.EPP_CHAIN_DELETED:
            getDisplay().asyncExec(() -> onChainListChangedOnServer());
            break;
      }
   }

   /**
    * Handle change of chain rules made by another session: reload the chain if it is loaded and has no local changes, warn if it
    * has (local changes are merged with the server version on save).
    *
    * @param chainId ID of changed chain
    */
   private void onRulesChangedOnServer(int chainId)
   {
      if (tabFolder.isDisposed() || (localChangesInProgress > 0))
         return;

      ChainTab tab = tabForChain(chainId);
      if ((tab == null) || !tab.chain.isLoaded())
         return;

      if (dirtyChains.contains(chainId))
      {
         addMessage(MessageArea.WARNING, String.format(i18n.tr("Chain \"%s\" was modified on the server. Your changes will be merged with the server version on save."), tab.getName()));
         return;
      }
      reloadChain(tab);
   }

   /**
    * Handle chain creation, modification, or deletion made by another session: reload the policy if there are no local changes,
    * otherwise warn.
    */
   private void onChainListChangedOnServer()
   {
      if (tabFolder.isDisposed() || (localChangesInProgress > 0))
         return;

      if (modified)
      {
         addMessage(MessageArea.WARNING, i18n.tr("Rule chains were changed on the server. Refresh the view after saving to see the current chains."));
         return;
      }
      reloadPolicy();
   }

   /**
    * Reload rules of one chain from server, discarding local changes of that chain
    *
    * @param tab chain tab
    */
   private void reloadChain(final ChainTab tab)
   {
      new Job(i18n.tr("Reloading event processing policy chain"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final EventProcessingPolicyChain loadedChain = session.getEventProcessingPolicyChain(tab.getChainId());
            runInUIThread(() -> {
               if (tab.tabItem.isDisposed())
                  return;
               tab.replaceChain(loadedChain);
               tab.showRules();
               onSelectionChange();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot reload event processing policy chain");
         }
      }.start();
   }

   /**
    * Set all rules in the active chain to collapsed or expanded state
    *
    * @param collapsed true to collapse all, false to expand
    */
   private void setAllRulesCollapsed(boolean collapsed)
   {
      ChainTab tab = activeTab();
      if (tab == null)
         return;
      tab.dataArea.setRedraw(false);
      for(RuleEditor e : tab.ruleEditors)
         if (!e.isDisposed())
            e.setCollapsed(collapsed, false);
      tab.updateLayout();
      tab.dataArea.setRedraw(true);
   }

   /**
    * Save all chains with unsaved changes, one at a time
    */
   private void saveAllChains()
   {
      final List<ChainTab> tabs = new ArrayList<>();
      for(ChainTab tab : chainTabs)
         if (dirtyChains.contains(tab.getChainId()))
            tabs.add(tab);
      saveChains(tabs);
   }

   /**
    * Save given chains to server, one at a time. Saving stops at the first conflict, which is shown to the user.
    *
    * @param tabs chains to save
    */
   private void saveChains(final List<ChainTab> tabs)
   {
      actionSave.setEnabled(false);
      actionSaveAll.setEnabled(false);
      localChangesInProgress++;
      new Job(i18n.tr("Saving event processing policy"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            for(final ChainTab tab : tabs)
            {
               EPPSaveResult result = session.saveEventProcessingPolicy(tab.chain);
               if (result.isSuccess())
               {
                  runInUIThread(() -> {
                     dirtyChains.remove(tab.getChainId());
                     modified = !dirtyChains.isEmpty();
                     tab.updateTabText();
                  });
               }
               else
               {
                  runInUIThread(() -> handleSaveConflicts(tab, result));
                  return;
               }
            }
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> {
               localChangesInProgress--;
               updateSaveActions();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot save event processing policy");
         }
      }.start();
   }

   /**
    * Handle save conflicts by showing dialog and taking appropriate action.
    *
    * @param tab tab of the chain that failed to save
    * @param result save result containing conflict information
    */
   private void handleSaveConflicts(ChainTab tab, EPPSaveResult result)
   {
      EPPConflictDialog dialog = new EPPConflictDialog(getWindow().getShell(), result.getConflicts(), tab.chain);
      if (dialog.open() == EPPConflictDialog.RELOAD)
      {
         dirtyChains.remove(tab.getChainId());
         modified = !dirtyChains.isEmpty();
         tab.updateTabText();
         updateSaveActions();
         reloadChain(tab);
      }
   }

   /**
    * Reload chain registry from server, discarding local changes.
    */
   private void reloadPolicy()
   {
      new Job(i18n.tr("Reloading event processing policy"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final EventProcessingPolicy loadedPolicy = session.getEventProcessingPolicyChains();
            List<ServerAction> serverActions = session.getActions();
            synchronized(actions)
            {
               actions.clear();
               for(ServerAction a : serverActions)
                  actions.put(a.getId(), a);
            }
            runInUIThread(() -> {
               policy = loadedPolicy;
               modified = false;
               dirtyChains.clear();
               initPolicyEditor();
            });
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot reload event processing policy");
         }
      }.start();
   }

   /**
    * Create new chain
    */
   private void createChain()
   {
      final EppChainPropertiesDialog dlg = new EppChainPropertiesDialog(getWindow().getShell(), null);
      if (dlg.open() != Window.OK)
         return;

      localChangesInProgress++;
      new Job(i18n.tr("Creating event processing policy chain"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final EventProcessingPolicyChain chain = session.createEppChain(dlg.getName(), dlg.getDescription(), dlg.getAccessList());
            chain.setAccessList(dlg.getAccessList());
            runInUIThread(() -> {
               policy.addChain(chain);
               ChainTab tab = new ChainTab(chain);
               chainTabs.add(tab);
               tabFolder.setSelection(tab.tabItem);
               onTabSelectionChange();
            });
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> localChangesInProgress--);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot create event processing policy chain");
         }
      }.start();
   }

   /**
    * Edit active chain's properties (name, description, access control list)
    */
   private void editChainProperties()
   {
      final ChainTab tab = activeTab();
      if ((tab == null) || tab.chain.isMain())
         return;

      final EppChainPropertiesDialog dlg = new EppChainPropertiesDialog(getWindow().getShell(), tab.chain);
      if (dlg.open() != Window.OK)
         return;

      localChangesInProgress++;
      new Job(i18n.tr("Updating event processing policy chain"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            session.modifyEppChain(tab.getChainId(), dlg.getName(), dlg.getDescription(), dlg.getAccessList());
            runInUIThread(() -> {
               tab.chain.setName(dlg.getName());
               tab.chain.setDescription(dlg.getDescription());
               tab.chain.setAccessList(dlg.getAccessList());
               tab.updateTabText();
               // Rules calling the renamed chain show its name
               for(ChainTab t : chainTabs)
               {
                  if (t.chain.isLoaded())
                  {
                     t.rebuildFilteredEditors();
                     t.updateLayout();
                  }
               }
               onSelectionChange();
            });
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> localChangesInProgress--);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot update event processing policy chain");
         }
      }.start();
   }

   /**
    * Delete active chain together with its rules. Rules calling the chain are listed for confirmation; their calls are removed.
    */
   private void deleteChain()
   {
      final ChainTab tab = activeTab();
      if ((tab == null) || tab.chain.isMain())
         return;

      new Job(i18n.tr("Checking event processing policy chain usage"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final List<EPPChainCaller> callers = session.getEppChainCallers(tab.getChainId());
            runInUIThread(() -> confirmChainDeletion(tab, callers));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot get event processing policy chain usage");
         }
      }.start();
   }

   /**
    * Ask for confirmation and delete chain
    *
    * @param tab chain tab
    * @param callers rules calling the chain
    */
   private void confirmChainDeletion(final ChainTab tab, List<EPPChainCaller> callers)
   {
      String message;
      if (callers.isEmpty())
      {
         message = String.format(i18n.tr("Chain \"%s\" and all its rules will be deleted. Are you sure?"), tab.getName());
      }
      else
      {
         Set<String> callingChains = new TreeSet<>();
         for(EPPChainCaller caller : callers)
            callingChains.add(getChainName(caller.getChainId()));
         message = String.format(i18n.tr("Chain \"%s\" and all its rules will be deleted. It is called by %d rule(s) in chain(s) %s; these calls will be removed. Are you sure?"),
               tab.getName(), callers.size(), String.join(", ", callingChains));
      }
      if (!MessageDialogHelper.openQuestion(getWindow().getShell(), i18n.tr("Delete Chain"), message))
         return;

      localChangesInProgress++;
      new Job(i18n.tr("Deleting event processing policy chain"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            final int deletedChainId = tab.getChainId();
            final Map<Integer, Integer> updatedChains = session.deleteEppChain(deletedChainId);
            runInUIThread(() -> {
               policy.removeChain(deletedChainId);
               for(EventProcessingPolicyChain chain : policy.getChains())
               {
                  if (chain.isLoaded())
                     for(EventProcessingPolicyRule rule : chain.getRules())
                        rule.getChainCalls().remove(Integer.valueOf(deletedChainId));
                  Integer version = updatedChains.get(chain.getId());
                  if (version != null)
                     chain.setVersion(version);
               }
               dirtyChains.remove(Integer.valueOf(deletedChainId));
               modified = !dirtyChains.isEmpty();
               initPolicyEditor();
            });
         }

         @Override
         protected void jobFinalize()
         {
            runInUIThread(() -> localChangesInProgress--);
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot delete event processing policy chain");
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

      imageStop.dispose();
      imageAlarm.dispose();
      imagePersistentStorage.dispose();
      imageExecute.dispose();
      imageCancelTimer.dispose();
      imageTerminate.dispose();
      imageStartDowntime.dispose();
      imageEndDowntime.dispose();
      imageLogEvent.dispose();
      imageNoLogEvent.dispose();
      imageError.dispose();
      imageChain.dispose();

      objectLabelProvider.dispose();
      actionLabelProvider.dispose();

      modified = false;
      super.dispose();
   }

   /**
    * Update layout of all chain tabs
    */
   public void updateEditorAreaLayout()
   {
      ChainTab tab = activeTab();
      if (tab != null)
         tab.updateLayout();
   }

   /**
    * Find server action by ID
    *
    * @param id action ID
    * @return server action or null
    */
   public ServerAction findActionById(Long id)
   {
      return actions.get(id);
   }

   /**
    * Find server actions by ID list
    *
    * @param idList list of action IDs
    * @return actions found, keyed by ID
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
    * @return all server actions
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
    * @return the imageCancelTimer
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
    * @return the imageEndDowntime
    */
   public Image getImageEndDowntime()
   {
      return imageEndDowntime;
   }

   /**
    * Get image used as marker for rules with errors
    *
    * @return error marker image
    */
   public Image getImageError()
   {
      return imageError;
   }

   /**
    * @return the imageLogEvent
    */
   public Image getImageLogEvent()
   {
      return imageLogEvent;
   }

   /**
    * @return the imageNoLogEvent
    */
   public Image getImageNoLogEvent()
   {
      return imageNoLogEvent;
   }

   /**
    * @return the imagePersistentStorage
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
    * Set modified flag. Setting it marks the active chain as dirty; clearing it forgets all dirty chains.
    *
    * @param modified the modified to set
    */
   public void setModified(boolean modified)
   {
      this.modified = modified;
      if (modified)
      {
         ChainTab tab = activeTab();
         if (tab != null)
            dirtyChains.add(tab.getChainId());
      }
      else
      {
         dirtyChains.clear();
      }
      for(ChainTab tab : chainTabs)
         tab.updateTabText();
      updateSaveActions();
   }

   /**
    * Mark specific chain as modified
    *
    * @param tab chain tab
    */
   private void markDirty(ChainTab tab)
   {
      dirtyChains.add(tab.getChainId());
      modified = true;
      tab.updateTabText();
      updateSaveActions();
   }

   /**
    * Update save actions: "Save" follows the active chain, "Save all" any chain with unsaved changes
    */
   private void updateSaveActions()
   {
      ChainTab tab = activeTab();
      actionSave.setEnabled((tab != null) && dirtyChains.contains(tab.getChainId()));
      actionSaveAll.setEnabled(!dirtyChains.isEmpty());
   }

   /**
    * Set selection to given rule
    *
    * @param e rule editor
    */
   public void setSelection(RuleEditor e)
   {
      ChainTab tab = tabForRule(e.getRule());
      if (tab == null)
         return;
      tab.clearSelection();
      addToSelection(e, false);
   }

   /**
    * Handle drag detect on rule editor: select it unless already part of selection
    *
    * @param e rule editor
    */
   public void onDragDetect(RuleEditor e)
   {
      ChainTab tab = tabForRule(e.getRule());
      if ((tab != null) && !tab.selection.contains(e))
         setSelection(e);
   }

   /**
    * Add rule to selection
    *
    * @param e rule editor
    * @param allFromPrevSelection true to also select all rules between previously selected rule and this one
    */
   public void addToSelection(RuleEditor e, boolean allFromPrevSelection)
   {
      ChainTab tab = tabForRule(e.getRule());
      if (tab == null)
         return;

      if (allFromPrevSelection && (tab.lastSelectedRule != -1))
      {
         int direction = Integer.signum(e.getRuleNumber() - tab.lastSelectedRule);
         for(int i = tab.lastSelectedRule + direction; i != e.getRuleNumber(); i += direction)
         {
            RuleEditor r = tab.ruleEditors.get(i - 1);
            tab.selection.add(r);
            r.setSelected(true);
         }
      }
      tab.selection.add(e);
      e.setSelected(true);
      tab.lastSelectedRule = e.getRuleNumber();
      onSelectionChange();
   }

   /**
    * Remove rule from selection
    *
    * @param e rule editor
    */
   public void removeFromSelection(RuleEditor e)
   {
      ChainTab tab = tabForRule(e.getRule());
      if (tab == null)
         return;
      tab.selection.remove(e);
      e.setSelected(false);
      tab.lastSelectedRule = -1;
      onSelectionChange();
   }

   /**
    * Handle change of active tab: load its rules if needed and update actions
    */
   private void onTabSelectionChange()
   {
      ChainTab tab = activeTab();
      boolean subChain = (tab != null) && !tab.chain.isMain();
      actionChainProperties.setEnabled(globalRights && subChain);
      actionDeleteChain.setEnabled(globalRights && subChain);
      if (tab != null)
      {
         tab.load();
         tab.updateLayout();   // Layout changes made while the tab was hidden are applied on activation
      }
      onSelectionChange();
   }

   /**
    * Update actions after selection change
    */
   private void onSelectionChange()
   {
      ChainTab tab = activeTab();
      boolean loaded = (tab != null) && tab.chain.isLoaded();
      boolean editable = loaded && tab.isEditable();
      int selectionSize = (tab != null) ? tab.selection.size() : 0;
      actionAddRule.setEnabled(editable);
      actionDelete.setEnabled(editable && (selectionSize > 0));
      actionInsertAbove.setEnabled(editable && (selectionSize == 1));
      actionInsertBelow.setEnabled(editable && (selectionSize == 1));
      actionCut.setEnabled(editable && (selectionSize > 0));
      actionCopy.setEnabled(selectionSize > 0);
      actionPaste.setEnabled(editable && (selectionSize == 1) && !clipboard.isEmpty());
      actionExplain.setEnabled(selectionSize == 1);
      updateSaveActions();
   }

   /**
    * Request AI explanation of the selected rule
    */
   private void explainRule()
   {
      ChainTab tab = activeTab();
      if ((tab == null) || (tab.selection.size() != 1))
         return;
      RuleEditor ruleEditor = tab.selection.iterator().next();
      ruleEditor.updateExplanation();
   }

   /**
    * Delete selected rules in the active chain
    */
   private void deleteSelectedRules()
   {
      ChainTab tab = activeTab();
      if (tab == null)
         return;

      for(RuleEditor e : tab.selection)
      {
         tab.chain.deleteRule(e.getRule());
         tab.ruleEditors.remove(e);
         e.dispose();
      }
      tab.renumberRules();
      tab.selection.clear();
      tab.lastSelectedRule = -1;
      onSelectionChange();
      tab.updateLayout();
      markDirty(tab);
   }

   /**
    * Insert new rule at given position within chain
    *
    * @param tab chain tab
    * @param position position (0-based)
    */
   private void insertRule(ChainTab tab, int position)
   {
      EventProcessingPolicyRule rule = new EventProcessingPolicyRule();
      rule.setChainId(tab.getChainId());
      rule.setRuleNumber(position + 1);
      tab.chain.insertRule(rule, position);

      RuleEditor ruleEditor = tab.addRuleEditor(rule, position);
      tab.renumberRules();

      if (position < tab.ruleEditors.size() - 1)
      {
         RuleEditor anchor = null;
         for(int i = position + 1; i < tab.ruleEditors.size(); i++)
            if (!tab.ruleEditors.get(i).isDisposed())
            {
               anchor = tab.ruleEditors.get(i);
               break;
            }
         if (anchor != null)
            ruleEditor.moveAbove(anchor);
      }
      tab.updateLayout();
      markDirty(tab);
   }

   /**
    * Cut selected rules to clipboard
    */
   private void cutRules()
   {
      ChainTab tab = activeTab();
      if (tab == null)
         return;

      clipboard.clear();
      actionPaste.setEnabled(true);
      for(RuleEditor e : tab.selection)
      {
         clipboard.add(e.getRule());
         tab.chain.deleteRule(e.getRule());
         tab.ruleEditors.remove(e);
         e.dispose();
      }
      tab.renumberRules();
      tab.selection.clear();
      tab.lastSelectedRule = -1;
      onSelectionChange();
      tab.updateLayout();
      markDirty(tab);
   }

   /**
    * Copy selected rules to clipboard
    */
   private void copyRules()
   {
      ChainTab tab = activeTab();
      if (tab == null)
         return;

      clipboard.clear();
      for(RuleEditor e : tab.selection)
         clipboard.add(new EventProcessingPolicyRule(e.getRule()));
      onSelectionChange();
   }

   /**
    * Paste rules from clipboard after the selected rule
    */
   private void pasteRules()
   {
      ChainTab tab = activeTab();
      if (tab == null)
         return;

      int position = tab.lastSelectedRule;
      RuleEditor anchor = null;
      if (position < tab.ruleEditors.size() - 1)
      {
         for(int i = position; i < tab.ruleEditors.size(); i++)
            if (!tab.ruleEditors.get(i).isDisposed())
            {
               anchor = tab.ruleEditors.get(i);
               break;
            }
      }

      for(EventProcessingPolicyRule rule : clipboard.paste())
      {
         rule.setChainId(tab.getChainId());
         rule.setRuleNumber(position + 1);
         tab.chain.insertRule(rule, position);
         RuleEditor ruleEditor = tab.addRuleEditor(rule, position);
         if (anchor != null)
            ruleEditor.moveAbove(anchor);
         position++;
      }
      tab.renumberRules();
      tab.updateLayout();
      markDirty(tab);
   }

   /**
    * Move selected rules below given anchor rule (drag and drop)
    *
    * @param anchor anchor rule editor
    */
   public void moveSelection(RuleEditor anchor)
   {
      ChainTab tab = tabForRule(anchor.getRule());
      if ((tab == null) || tab.selection.contains(anchor))
         return;

      List<RuleEditor> movedRuleEditors = new ArrayList<RuleEditor>();
      for(RuleEditor e : tab.ruleEditors)
      {
         if (!tab.selection.contains(e))
         {
            movedRuleEditors.add(e);
            if (e.equals(anchor))
            {
               RuleEditor curr = anchor;
               for(RuleEditor s : tab.selection)
               {
                  movedRuleEditors.add(s);
                  s.moveBelow(curr);
                  curr = s;
               }
            }
         }
      }
      tab.ruleEditors = movedRuleEditors;
      tab.renumberRules();

      List<EventProcessingPolicyRule> rules = tab.chain.getRules();
      rules.clear();
      for(RuleEditor e : tab.ruleEditors)
         rules.add(e.getRule());

      tab.updateLayout();
      markDirty(tab);
   }

   /**
    * Enable or disable selected rules
    *
    * @param enabled true to enable
    */
   private void enableRules(boolean enabled)
   {
      ChainTab tab = activeTab();
      if (tab == null)
         return;
      for(RuleEditor e : tab.selection)
         e.enableRule(enabled);
   }

   /**
    * Fill rule context menu
    *
    * @param manager menu manager
    */
   public void fillRuleContextMenu(IMenuManager manager)
   {
      manager.add(actionExplain);
      manager.add(new Separator());
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
    * @see org.netxms.nxmc.base.views.View#onFilterModify()
    */
   @Override
   protected void onFilterModify()
   {
      filterText = getFilterText().trim().toLowerCase();
      for(ChainTab tab : chainTabs)
      {
         // change editors visibility
         RuleEditor prev = null;
         for(int i = 0; i < tab.ruleEditors.size(); i++)
         {
            RuleEditor e = tab.ruleEditors.get(i);
            boolean visible = isRuleVisible(e.getRule());
            if (!e.isDisposed() && !visible)
            {
               e.dispose();
               tab.selection.remove(e);
            }
            else if (e.isDisposed() && visible)
            {
               e = new RuleEditor(tab.dataArea, e.getRule(), this);
               GridData gd = new GridData();
               gd.horizontalAlignment = SWT.FILL;
               gd.grabExcessHorizontalSpace = true;
               e.setLayoutData(gd);
               if (prev != null)
                  e.moveBelow(prev);
               else
                  e.moveAbove(null);
               tab.ruleEditors.set(i, e);
            }
            if (!e.isDisposed())
               prev = e;
         }
         tab.updateLayout();
      }
   }

   /**
    * Check if given rule should be visible
    *
    * @param rule rule to check
    * @return true if rule should be visible
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

      // Check rule GUID
      if (rule.getGuid().toString().toLowerCase().contains(filterText))
         return true;

      return false;
   }

   /**
    * @see org.netxms.nxmc.base.views.ConfigurationView#save()
    */
   @Override
   public void save()
   {
      saveAllChains();
   }

   /**
    * @see org.netxms.nxmc.base.views.View#refresh()
    */
   @Override
   public void refresh()
   {
      if (isModified())
      {
         if (!MessageDialogHelper.openConfirm(getWindow().getShell(), i18n.tr("Unsaved Changes"),
               i18n.tr("Are you sure you want to refresh and lose all not saved changes?")))
         {
            return;
         }
      }
      reloadPolicy();
   }
}
