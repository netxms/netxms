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
package org.netxms.nxmc.modules.events.widgets;

import java.text.DateFormat;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Comparator;
import java.util.List;
import java.util.Locale;
import java.util.Map.Entry;
import org.eclipse.jface.action.Action;
import org.eclipse.jface.action.MenuManager;
import org.eclipse.jface.preference.PreferenceManager;
import org.eclipse.jface.preference.PreferenceNode;
import org.eclipse.jface.util.LocalSelectionTransfer;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.jface.viewers.TreeViewer;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CLabel;
import org.eclipse.swt.dnd.DND;
import org.eclipse.swt.dnd.DragSource;
import org.eclipse.swt.dnd.DragSourceEvent;
import org.eclipse.swt.dnd.DragSourceListener;
import org.eclipse.swt.dnd.DropTarget;
import org.eclipse.swt.dnd.DropTargetEvent;
import org.eclipse.swt.dnd.DropTargetListener;
import org.eclipse.swt.dnd.Transfer;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.eclipse.swt.widgets.Text;
import org.netxms.client.NXCSession;
import org.netxms.client.ServerAction;
import org.netxms.client.constants.Severity;
import org.netxms.client.events.ActionExecutionConfiguration;
import org.netxms.client.events.AlarmCategory;
import org.netxms.client.events.EventProcessingPolicyRule;
import org.netxms.client.events.EventTemplate;
import org.netxms.client.events.TimeFrame;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.propertypages.PropertyDialog;
import org.netxms.nxmc.base.widgets.Card;
import org.netxms.nxmc.base.widgets.helpers.DashboardElementButton;
import org.netxms.nxmc.localization.DateFormatFactory;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.propertypages.RuleAction;
import org.netxms.nxmc.modules.events.propertypages.RuleActionScript;
import org.netxms.nxmc.modules.events.propertypages.RuleAiAgentInstructions;
import org.netxms.nxmc.modules.events.propertypages.RuleAlarm;
import org.netxms.nxmc.modules.events.propertypages.RuleComments;
import org.netxms.nxmc.modules.events.propertypages.RuleCondition;
import org.netxms.nxmc.modules.events.propertypages.RuleCustomAttribute;
import org.netxms.nxmc.modules.events.propertypages.RuleDowntimeControl;
import org.netxms.nxmc.modules.events.propertypages.RuleEvents;
import org.netxms.nxmc.modules.events.propertypages.RuleFilteringScript;
import org.netxms.nxmc.modules.events.propertypages.RulePersistentStorage;
import org.netxms.nxmc.modules.events.propertypages.RuleServerActions;
import org.netxms.nxmc.modules.events.propertypages.RuleSeverityFilter;
import org.netxms.nxmc.modules.events.propertypages.RuleSourceObjects;
import org.netxms.nxmc.modules.events.propertypages.RuleTimeFilter;
import org.netxms.nxmc.modules.events.propertypages.RuleTimerCancellations;
import org.netxms.nxmc.modules.events.views.EventProcessingPolicyEditor;
import org.netxms.nxmc.modules.nxsl.widgets.ScriptEditor;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.xnap.commons.i18n.I18n;

/**
 * Rule editor widget
 */
public class RuleEditor extends Composite
{
   private static final int INDENT = 20;

   private final I18n i18n = LocalizationHelper.getI18n(RuleEditor.class);

   private EventProcessingPolicyRule rule;
   private int ruleNumber;
   private EventProcessingPolicyEditor editor;
   private NXCSession session;
   private boolean collapsed = true;
   private boolean verticalLayout = false;
   private Composite leftPanel;
   private Label ruleNumberLabel;
   private Composite header;
   private Label headerLabel;
   private Composite mainArea;
   private Card condition;
   private Card action;
   private Label expandButton;
   private Label editButton;
   private boolean modified = false;
   private boolean selected = false;
   private MouseListener ruleMouseListener;

   /**
    * @param parent
    * @param rule
    */
   public RuleEditor(Composite parent, EventProcessingPolicyRule rule, EventProcessingPolicyEditor editor)
   {
      super(parent, SWT.NONE);
      this.rule = rule;
      this.ruleNumber = rule.getRuleNumber();
      this.editor = editor;

      session = Registry.getSession();

      setBackground(ThemeEngine.getBackgroundColor("RuleEditor.Border.Rule"));

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginHeight = 1;
      layout.marginWidth = 0;
      layout.horizontalSpacing = 1;
      layout.verticalSpacing = 1;
      setLayout(layout);

      ruleMouseListener = new MouseListener() {
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 1)
            {
               processRuleMouseEvent(e);
            }
            else if (e.button == 3 && !selected)
            {
               RuleEditor.this.editor.setSelection(RuleEditor.this);
            }
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
            // Update selection only on button up event if mouse was pressed on already selected rule
            if ((e.button == 1) && ((e.stateMask & (SWT.MOD1 | SWT.SHIFT)) == 0) && selected)
               RuleEditor.this.editor.setSelection(RuleEditor.this);
         }
      };

      createLeftPanel();
      createHeader();
      createMainArea();

      createPopupMenu(new Control[] { leftPanel, ruleNumberLabel, header, headerLabel });

      condition = new Card(mainArea, i18n.tr("Filter")) {
         @Override
         protected Control createClientArea(Composite parent)
         {
            setTitleBackground(ThemeEngine.getBackgroundColor("RuleEditor.Border.Condition"));
            setTitleColor(ThemeEngine.getForegroundColor("RuleEditor.Title"));
            return createConditionControl(parent, RuleEditor.this.rule);
         }
      };
      configureLayout(condition);
      final Action editRuleCondition = new Action() {
         @Override
         public void run()
         {
            editRule("org.netxms.ui.eclipse.epp.propertypages.RuleCondition#0");
         }
      };
      condition.addButton(new DashboardElementButton(i18n.tr("Edit condition"), SharedIcons.IMG_EDIT, editRuleCondition));
      condition.setDoubleClickAction(editRuleCondition);

      action = new Card(mainArea, i18n.tr("Action")) {
         @Override
         protected Control createClientArea(Composite parent)
         {
            setTitleBackground(ThemeEngine.getBackgroundColor("RuleEditor.Border.Action"));
            setTitleColor(ThemeEngine.getForegroundColor("RuleEditor.Title"));
            return createActionControl(parent, RuleEditor.this.rule);
         }
      };
      configureLayout(action);
      final Action editRuleAction = new Action() {
         @Override
         public void run()
         {
            editRule("org.netxms.ui.eclipse.epp.propertypages.RuleAction#1");
         }
      };
      action.addButton(new DashboardElementButton(i18n.tr("Edit actions"), SharedIcons.IMG_EDIT, editRuleAction));
      action.setDoubleClickAction(editRuleAction);

      dragEnable();
      dropEnable();
   }

   /**
    * Enables drag functionality of the rule
    */
   private void dragEnable()
   {
      // enable each label to be a drag target
      DragSource source = new DragSource(headerLabel, DND.DROP_MOVE);
      source.setTransfer(new Transfer[] { LocalSelectionTransfer.getTransfer() });
      // add a drop listener
      source.addDragListener(new RuleDragSourceListener(this));
   }

   /**
    * Enables drop functionality of the rule
    */
   private void dropEnable()
   {
      // enable each label to be a drop target
      DropTarget target = new DropTarget(this, DND.DROP_MOVE);
      target.setTransfer(new Transfer[] { LocalSelectionTransfer.getTransfer() });
      // add a drop listener
      target.addDropListener(new RuleDropTargetListener());
   }

   /**
    * Create main area which contains condition and action elements
    */
   private void createMainArea()
   {
      mainArea = new Composite(this, SWT.NONE);
      mainArea.setBackground(ThemeEngine.getBackgroundColor("Dashboard"));

      GridLayout layout = new GridLayout();
      layout.numColumns = verticalLayout ? 1 : 2;
      layout.marginHeight = 4;
      layout.marginWidth = 4;
      layout.makeColumnsEqualWidth = true;
      mainArea.setLayout(layout);

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.exclude = collapsed;
      mainArea.setLayoutData(gd);
   }

   /**
    * Create panel with rule number and collapse/expand button on the left
    */
   private void createLeftPanel()
   {
      leftPanel = new Composite(this, SWT.NONE);
      leftPanel.setBackground(ThemeEngine.getBackgroundColor((rule.isDisabled() || rule.isFilterEmpty()) ? "RuleEditor.Title.Disabled" : "RuleEditor.Title.Normal"));
      leftPanel.addMouseListener(ruleMouseListener);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      leftPanel.setLayout(layout);

      GridData gd = new GridData();
      gd.verticalSpan = collapsed ? 1 : 2;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessVerticalSpace = true;
      leftPanel.setLayoutData(gd);

      ruleNumberLabel = new Label(leftPanel, SWT.NONE);
      ruleNumberLabel.setText(Integer.toString(ruleNumber));
      ruleNumberLabel.setFont(editor.getBoldFont());
      ruleNumberLabel.setBackground(leftPanel.getBackground());
      ruleNumberLabel.setForeground(ThemeEngine.getForegroundColor("RuleEditor.Title"));
      ruleNumberLabel.setAlignment(SWT.CENTER);
      ruleNumberLabel.addMouseListener(ruleMouseListener);

      gd = new GridData();
      gd.horizontalAlignment = SWT.CENTER;
      gd.verticalAlignment = SWT.CENTER;
      gd.grabExcessVerticalSpace = true;
      gd.widthHint = 30;
      ruleNumberLabel.setLayoutData(gd);
   }

   /**
    * Create popup menu for given controls
    * 
    * @param controls
    */
   private void createPopupMenu(final Control[] controls)
   {
      MenuManager menuMgr = new MenuManager();
      menuMgr.setRemoveAllWhenShown(true);
      menuMgr.addMenuListener((m) -> editor.fillRuleContextMenu(m));
      for(Control c : controls)
      {
         Menu menu = menuMgr.createContextMenu(c);
         c.setMenu(menu);
      }
   }

   /**
    * Create rule header
    */
   private void createHeader()
   {
      final MouseListener headerMouseListener = new MouseListener() {
         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            if (e.button == 1)
               setCollapsed(!isCollapsed(), true);
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
            ruleMouseListener.mouseDown(e);
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
            ruleMouseListener.mouseUp(e);
         }
      };

      header = new Composite(this, SWT.NONE);
      header.setBackground(ThemeEngine.getBackgroundColor((rule.isDisabled() || rule.isFilterEmpty()) ? "RuleEditor.Title.Disabled" : "RuleEditor.Title.Normal"));
      header.addMouseListener(headerMouseListener);

      GridLayout layout = new GridLayout();
      layout.numColumns = 3;
      header.setLayout(layout);

      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      header.setLayoutData(gd);

      headerLabel = new Label(header, SWT.NONE);
      if (rule.isDisabled())
         headerLabel.setText(rule.getComments() + i18n.tr(" (disabled)"));
      else if (rule.isFilterEmpty())
         headerLabel.setText(rule.getComments() + i18n.tr(" (empty filter)"));
      else
         headerLabel.setText(rule.getComments());
      headerLabel.setBackground(header.getBackground());
      headerLabel.setForeground(ThemeEngine.getForegroundColor("RuleEditor.Title"));
      headerLabel.setFont(editor.getNormalFont());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      headerLabel.setLayoutData(gd);
      headerLabel.addMouseListener(headerMouseListener);

      editButton = new Label(header, SWT.NONE);
      editButton.setBackground(header.getBackground());
      editButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
      editButton.setImage(SharedIcons.IMG_EDIT);
      editButton.setToolTipText(i18n.tr("Edit rule"));
      editButton.addMouseListener(new MouseListener() {
         private boolean doAction = false;

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            if (e.button == 1)
               doAction = false;
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 1)
               doAction = true;
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
            if ((e.button == 1) && doAction)
               editRule("Comments");
         }
      });

      expandButton = new Label(header, SWT.NONE);
      expandButton.setBackground(header.getBackground());
      expandButton.setCursor(getDisplay().getSystemCursor(SWT.CURSOR_HAND));
      expandButton.setImage(collapsed ? SharedIcons.IMG_EXPAND : SharedIcons.IMG_COLLAPSE);
      expandButton.setToolTipText(collapsed ? i18n.tr("Expand rule") : i18n.tr("Collapse rule"));
      expandButton.addMouseListener(new MouseListener() {
         private boolean doAction = false;

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            if (e.button == 1)
               doAction = false;
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
            if (e.button == 1)
               doAction = true;
         }

         @Override
         public void mouseUp(MouseEvent e)
         {
            if ((e.button == 1) && doAction)
               setCollapsed(!isCollapsed(), true);
         }
      });
   }

   /**
    * Configure layout for child element
    * 
    * @param ctrl child control
    */
   private void configureLayout(Control ctrl)
   {
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      ctrl.setLayoutData(gd);
   }

   /**
    * Create mouse listener for elements
    * 
    * @param pageId property page ID to be opened on double click
    * @return
    */
   private MouseListener createMouseListener(final String pageId)
   {
      return new MouseListener() {
         @Override
         public void mouseUp(MouseEvent e)
         {
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
         }

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
            editRule(pageId);
         }
      };
   }

   /**
    * Create condition summary control
    * 
    * @param parent
    * @param rule
    * @return
    */
   private Control createConditionControl(Composite parent, final EventProcessingPolicyRule rule)
   {
      Composite clientArea = new Composite(parent, SWT.NONE);

      clientArea.setBackground(ThemeEngine.getBackgroundColor("RuleEditor"));
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = 0;
      clientArea.setLayout(layout);

      if (rule.isFilterEmpty())
      {
         createLabel(clientArea, 0, true, i18n.tr("SKIP"), null);
         return clientArea;
      }

      boolean needAnd = false;
      if (((rule.getSources().size() > 0 || rule.getSourceExclusions().size() > 0) && rule.isSourceInverted())
            || ((rule.getSources().size() == 0) && (rule.getSourceExclusions().size() == 0) && (rule.getEvents().size() > 0) && rule.isEventsInverted()))
         createLabel(clientArea, 0, true, i18n.tr("IF NOT"), null);
      else
         createLabel(clientArea, 0, true, i18n.tr("IF"), null);

      /* source */
      if (rule.getSources().size() > 0 || rule.getSourceExclusions().size() > 0)
      {
         final MouseListener listener = createMouseListener("SourceObjects");
         addConditionGroupLabel(clientArea, i18n.tr("source object is one of the following:"), needAnd, rule.isSourceInverted(), listener);

         List<AbstractObject> sortedObjects = session.findMultipleObjects(rule.getSources(), true);
         sortedObjects.sort(new Comparator<AbstractObject>() {
            @Override
            public int compare(AbstractObject o1, AbstractObject o2)
            {
               return o1.getObjectName().compareToIgnoreCase(o2.getObjectName());
            }
         });
         for(AbstractObject object : sortedObjects)
         {
            CLabel clabel = createCLabel(clientArea, 2, false);
            clabel.addMouseListener(listener);
            clabel.setText(object.getObjectName());
            clabel.setImage(editor.getObjectLabelProvider().getImage(object));
         }
         
         if (rule.getSourceExclusions().size() > 0)
            addConditionGroupLabel(clientArea, "except:", needAnd, rule.isSourceInverted(), listener);
         sortedObjects = session.findMultipleObjects(rule.getSourceExclusions(), true);
         sortedObjects.sort(new Comparator<AbstractObject>() {
            @Override
            public int compare(AbstractObject o1, AbstractObject o2)
            {
               return o1.getObjectName().compareToIgnoreCase(o2.getObjectName());
            }
         });
         for(AbstractObject object : sortedObjects)
         {
            CLabel clabel = createCLabel(clientArea, 2, false);
            clabel.addMouseListener(listener);
            clabel.setText(object.getObjectName());
            clabel.setImage(editor.getObjectLabelProvider().getImage(object));
         }
         needAnd = true;
      }

      /* events */
      if (rule.getEvents().size() > 0)
      {
         final MouseListener listener = createMouseListener("Events");
         addConditionGroupLabel(clientArea,
               rule.isCorrelatedEventProcessingAllowed() ? i18n.tr("event code is one of the following (including correlated events):") : i18n.tr("event code is one of the following:"), needAnd,
               rule.isEventsInverted(), listener);

         List<EventTemplate> sortedEvents = new ArrayList<EventTemplate>(rule.getEvents().size());
         for(Integer code : rule.getEvents())
         {
            EventTemplate event = session.findEventTemplateByCode(code);
            if (event == null)
            {
               event = new EventTemplate(code);
               event.setSeverity(Severity.UNKNOWN);
               event.setName("[" + code.toString() + "]");
            }
            sortedEvents.add(event);
         }
         Collections.sort(sortedEvents, new Comparator<EventTemplate>() {
            @Override
            public int compare(EventTemplate t1, EventTemplate t2)
            {
               return t1.getName().compareToIgnoreCase(t2.getName());
            }
         });

         for(EventTemplate e : sortedEvents)
         {
            CLabel clabel = createCLabel(clientArea, 2, false);
            clabel.addMouseListener(listener);
            clabel.setText(e.getName());
            clabel.setImage(StatusDisplayInfo.getStatusImage(e.getSeverity()));
         }
         needAnd = true;
      }

      /* time filter */
      if (rule.getTimeFrames().size() != 0)
      {
         final MouseListener listener = createMouseListener("TimeFilter");
         addConditionGroupLabel(clientArea, "current time is within:", needAnd, rule.isTimeFramesInverted(), listener);

         final DateFormat dfTime = DateFormatFactory.getShortTimeFormat();    
         for(TimeFrame tf : rule.getTimeFrames())
         {
            CLabel clabel = createCLabel(clientArea, 2, false);
            clabel.addMouseListener(listener);
            clabel.setText(tf.getFormattedDateString(dfTime, Locale.getDefault()));
         }
         needAnd = true;
      }

      /* severity */
      if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_ANY) != EventProcessingPolicyRule.SEVERITY_ANY)
      {
         final MouseListener listener = createMouseListener("SeverityFilter");
         addConditionGroupLabel(clientArea, i18n.tr("event severity is one of the following:"), needAnd, false, listener);

         if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_NORMAL) != 0)
            addSeverityLabel(clientArea, Severity.NORMAL, listener);

         if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_WARNING) != 0)
            addSeverityLabel(clientArea, Severity.WARNING, listener);

         if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_MINOR) != 0)
            addSeverityLabel(clientArea, Severity.MINOR, listener);

         if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_MAJOR) != 0)
            addSeverityLabel(clientArea, Severity.MAJOR, listener);

         if ((rule.getFlags() & EventProcessingPolicyRule.SEVERITY_CRITICAL) != 0)
            addSeverityLabel(clientArea, Severity.CRITICAL, listener);

         needAnd = true;
      }

      /* script */
      if ((rule.getFilterScript() != null) && !rule.getFilterScript().isEmpty())
      {
         final MouseListener listener = createMouseListener("FilteringScript");
         addConditionGroupLabel(clientArea, i18n.tr("the following script returns true:"), needAnd, false, listener);

         ScriptEditor scriptEditor = new ScriptEditor(clientArea, SWT.BORDER, SWT.NONE, false);
         GridData gd = new GridData();
         gd.horizontalIndent = INDENT * 2;
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         scriptEditor.setLayoutData(gd);
         scriptEditor.setText(rule.getFilterScript());
         scriptEditor.getTextWidget().setEditable(false);
         scriptEditor.getTextWidget().addMouseListener(listener);
      }

      return clientArea;
   }

   /**
    * Create label with given text and indent
    * 
    * @param parent
    * @param indent
    * @param bold
    * @param text
    * @return
    */
   private Label createLabel(Composite parent, int indent, boolean bold, String text, MouseListener mouseListener)
   {
      Label label = new Label(parent, SWT.NONE);
      label.setBackground(ThemeEngine.getBackgroundColor("RuleEditor"));
      label.setFont(bold ? editor.getBoldFont() : editor.getNormalFont());
      label.setText(text);

      GridData gd = new GridData();
      gd.horizontalIndent = INDENT * indent;
      label.setLayoutData(gd);

      if (mouseListener != null)
         label.addMouseListener(mouseListener);

      return label;
   }

   /**
    * Create CLable with given indent
    * 
    * @param parent
    * @param indent
    * @param bold
    * @return
    */
   private CLabel createCLabel(Composite parent, int indent, boolean bold)
   {
      CLabel label = new CLabel(parent, SWT.NONE);
      label.setBackground(ThemeEngine.getBackgroundColor("RuleEditor"));
      label.setFont(bold ? editor.getBoldFont() : editor.getNormalFont());

      GridData gd = new GridData();
      gd.horizontalIndent = INDENT * indent;
      label.setLayoutData(gd);

      return label;
   }

   /**
    * Add condition group opening text
    * 
    * @param parent parent composite
    * @param title group's title
    * @param needAnd true if AND clause have to be added
    * @param inverted true if condition is inverted
    * @param mouseListener mouse listener to attach
    */
   private void addConditionGroupLabel(Composite parent, String title, boolean needAnd, boolean inverted, MouseListener mouseListener)
   {
      if (needAnd)
         createLabel(parent, 0, true, inverted ? i18n.tr("AND NOT") : i18n.tr("AND"), null);
      createLabel(parent, 1, false, title, mouseListener);
   }

   /**
    * Add severity label
    * 
    * @param parent parent composite
    * @param severity severity code
    */
   private void addSeverityLabel(Composite parent, Severity severity, MouseListener mouseListener)
   {
      CLabel clabel = createCLabel(parent, 2, false);
      clabel.setText(StatusDisplayInfo.getStatusText(severity));
      clabel.setImage(StatusDisplayInfo.getStatusImage(severity));
      clabel.addMouseListener(mouseListener);
   }

   /**
    * Create action summary control
    * 
    * @param parent
    * @param rule
    * @return
    */
   private Control createActionControl(Composite parent, final EventProcessingPolicyRule rule)
   {
      Composite clientArea = new Composite(parent, SWT.NONE);

      clientArea.setBackground(ThemeEngine.getBackgroundColor("RuleEditor"));
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = 0;
      clientArea.setLayout(layout);

      /* alarm */
      if ((rule.getFlags() & EventProcessingPolicyRule.GENERATE_ALARM) != 0)
      {
         final MouseListener listener = createMouseListener("Alarm");
         if (rule.getAlarmSeverity().compareTo(Severity.TERMINATE) < 0)
         {
            addActionGroupLabel(clientArea, i18n.tr("Generate alarm"), editor.getImageAlarm(), listener);

            CLabel clabel = createCLabel(clientArea, 1, false);
            clabel.setImage(StatusDisplayInfo.getStatusImage(rule.getAlarmSeverity()));
            clabel.setText(rule.getAlarmMessage());
            clabel.addMouseListener(listener);

            if ((rule.getAlarmKey() != null) && !rule.getAlarmKey().isEmpty())
            {
               createLabel(clientArea, 1, false, String.format(i18n.tr("with key \"%s\""), rule.getAlarmKey()), listener);
            }
            if ((rule.getRcaScriptName() != null) && !rule.getRcaScriptName().isEmpty())
            {
               createLabel(clientArea, 1, false, String.format(i18n.tr("using root cause analysis script \"%s\""), rule.getRcaScriptName()), listener);
            }
         }
         else if (rule.getAlarmSeverity() == Severity.TERMINATE)
         {
            addActionGroupLabel(clientArea, i18n.tr("Terminate alarms"), editor.getImageTerminate(), listener);
            createLabel(clientArea, 1, false, String.format(i18n.tr("with key \"%s\""), rule.getAlarmKey()), listener);
            if ((rule.getFlags() & EventProcessingPolicyRule.TERMINATE_BY_REGEXP) != 0)
               createLabel(clientArea, 1, false, i18n.tr("(use regular expression for alarm termination)"), listener);
         }
         else if (rule.getAlarmSeverity() == Severity.RESOLVE)
         {
            addActionGroupLabel(clientArea, i18n.tr("Resolve alarms"), editor.getImageTerminate(), listener);
            createLabel(clientArea, 1, false, String.format(i18n.tr("with key \"%s\""), rule.getAlarmKey()), listener);
            if ((rule.getFlags() & EventProcessingPolicyRule.TERMINATE_BY_REGEXP) != 0)
               createLabel(clientArea, 1, false, i18n.tr("(use regular expression for alarm resolve)"), listener);
         }
         
         if (rule.getAlarmCategories().size() > 0)
         {
            createLabel(clientArea, 1, false, rule.getAlarmCategories().size() == 1 ? i18n.tr("with category: ") : i18n.tr("with categories: "), listener);
            for(Long id :rule.getAlarmCategories())
            {
               AlarmCategory category = session.findAlarmCategoryById(id);
               if (category != null)
               {
                  createLabel(clientArea, 2, false, category.getName(), listener);
               }
            }
         }

         if ((rule.getFlags() & EventProcessingPolicyRule.CREATE_TICKET) != 0)
         {
            createLabel(clientArea, 1, false, i18n.tr("and create helpdesk ticket"), listener);
         }

         if ((rule.getFlags() & EventProcessingPolicyRule.REQUEST_AI_COMMENT) != 0)
         {
            createLabel(clientArea, 1, false, i18n.tr("and add AI assistant's comment"), listener);
         }
      }

      /* downtime */
      if ((rule.getFlags() & (EventProcessingPolicyRule.START_DOWNTIME | EventProcessingPolicyRule.END_DOWNTIME)) != 0)
      {
         final MouseListener listener = createMouseListener("Downtime");
         if ((rule.getFlags() & EventProcessingPolicyRule.START_DOWNTIME) != 0)
         {
            addActionGroupLabel(clientArea, i18n.tr("Start downtime with tag \"{0}\"", rule.getDowntimeTag()), editor.getImageStartDowntime(), listener);
         }
         else
         {
            addActionGroupLabel(clientArea, i18n.tr("End downtime with tag \"{0}\"", rule.getDowntimeTag()), editor.getImageEndDowntime(), listener);
         }
      }

      /* persistent storage */
      if (!rule.getPStorageSet().isEmpty() || !rule.getPStorageDelete().isEmpty())
      {
         final MouseListener listener = createMouseListener("PersistentStorage");
         addActionGroupLabel(clientArea, i18n.tr("Update persistent storage entries"), editor.getImagePersistentStorage(), listener);

         if (!rule.getPStorageSet().isEmpty())
         {
            createLabel(clientArea, 1, false, i18n.tr("Set persistent storage values:"), listener);
            for(Entry<String, String> e : rule.getPStorageSet().entrySet())
            {
               createLabel(clientArea, 2, false, e.getKey() + " = \"" + e.getValue() + "\"", listener);
            }
         }

         if (!rule.getPStorageDelete().isEmpty())
         {
            createLabel(clientArea, 1, false, i18n.tr("Delete persistent storage values:"), listener);
            List<String> pStorageList = rule.getPStorageDelete();
            for(int i = 0; i < pStorageList.size(); i++)
            {
               createLabel(clientArea, 2, false, pStorageList.get(i), listener);
            }
         }
      }

      /* custom attributes */
      if (rule.getCustomAttributeStorageSet().size() != 0 || rule.getCustomAttributeStorageDelete().size() != 0)
      {
         final MouseListener listener = createMouseListener("CustomAttributes"); //$NON-NLS-1$
         addActionGroupLabel(clientArea, i18n.tr("Update custom attribute entries"), editor.getImagePersistentStorage(), listener);

         if(rule.getCustomAttributeStorageSet().size() != 0)
         {
            createLabel(clientArea, 1, false, i18n.tr("Set custom attribute values"), listener); //$NON-NLS-1$ //$NON-NLS-2$
            for(Entry<String, String> e : rule.getCustomAttributeStorageSet().entrySet())
            {
               createLabel(clientArea, 2, false, e.getKey() + " = \"" + e.getValue() + "\"", listener); //$NON-NLS-1$ //$NON-NLS-2$
            }
         }

         if(rule.getCustomAttributeStorageDelete().size() != 0)
         {
            createLabel(clientArea, 1, false, i18n.tr("Delete custom attribute values"), listener);
            List<String> customAttributeList = rule.getCustomAttributeStorageDelete();
            for(int i = 0; i < customAttributeList.size(); i++)
            {
               createLabel(clientArea, 2, false, customAttributeList.get(i), listener);
            }
         }
         
      }

      /* actions */
      if (!rule.getActions().isEmpty())
      {
         final MouseListener listener = createMouseListener("ServerActions");
         addActionGroupLabel(clientArea, i18n.tr("Execute the following predefined actions:"), editor.getImageExecute(), listener);
         for(ActionExecutionConfiguration c : rule.getActions())
         {
            CLabel clabel = createCLabel(clientArea, 1, false);
            clabel.addMouseListener(listener);
            ServerAction action = editor.findActionById(c.getActionId());
            if (action != null)
            {
               clabel.setText(action.getName() + (c.isActive() ? "" : i18n.tr(" (Inactive)")));
               clabel.setImage(editor.getActionLabelProvider().getImage(action));
            }
            else
            {
               clabel.setText("[" + Long.toString(c.getActionId()) + "]"+ (c.isActive() ? "" : i18n.tr(" (Inactive)")));
            }
            if (!c.getTimerDelay().isEmpty())
            {
               clabel = createCLabel(clientArea, 2, false);
               clabel.addMouseListener(listener);
               if (c.getTimerKey().isEmpty())
               {
                  clabel.setText(String.format(i18n.tr("Delayed by %s seconds"), c.getTimerDelay()));
               }
               else
               {
                  clabel.setText(String.format(i18n.tr("Delayed by %s seconds with timer key set to \"%s\""), c.getTimerDelay(), c.getTimerKey()));
               }
            }
            if (!c.getBlockingTimerKey().isEmpty())
            {
               clabel = createCLabel(clientArea, 2, false);
               clabel.addMouseListener(listener);
               clabel.setText(String.format(i18n.tr("Do not run if timer with key \"%s\" is active"), c.getBlockingTimerKey()));
            }
            if (!c.getSnoozeTime().isEmpty())
            {
               clabel = createCLabel(clientArea, 2, false);
               clabel.addMouseListener(listener);
               clabel.setText(String.format(i18n.tr("Set snooze timer for %s seconds after execution with key %s"), c.getSnoozeTime(), c.getBlockingTimerKey()));
            }
         }
      }

      /* script */
      if ((rule.getActionScript() != null) && !rule.getActionScript().isEmpty())
      {
         final MouseListener listener = createMouseListener("ActionScript"); //$NON-NLS-1$
         addActionGroupLabel(clientArea, i18n.tr("Execute script"), editor.getImageExecute(), listener);

         ScriptEditor scriptEditor = new ScriptEditor(clientArea, SWT.BORDER, SWT.NONE, false);
         GridData gd = new GridData();
         gd.horizontalIndent = INDENT * 2;
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         scriptEditor.setLayoutData(gd);
         scriptEditor.setText(rule.getActionScript());
         scriptEditor.getTextWidget().setEditable(false);
         scriptEditor.getTextWidget().addMouseListener(listener);
      }

      /* timer cancellations */
      if (!rule.getTimerCancellations().isEmpty())
      {
         final MouseListener listener = createMouseListener("TimerCancellations");
         addActionGroupLabel(clientArea, i18n.tr("Cancel the following timers:"), editor.getImageCancelTimer(), listener);
         for(String tc : rule.getTimerCancellations())
         {
            CLabel clabel = createCLabel(clientArea, 1, false);
            clabel.addMouseListener(listener);
            clabel.setText(tc);
         }
      }

      if (!rule.getAiAgentInstructions().isBlank())
      {
         final MouseListener listener = createMouseListener("AIAgentInstructions");
         addActionGroupLabel(clientArea, i18n.tr("Instruct AI agent:"), editor.getImageExecute(), listener);

         Text aiInstructions = new Text(clientArea, SWT.MULTI | SWT.WRAP);
         GridData gd = new GridData();
         gd.horizontalIndent = INDENT * 2;
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         aiInstructions.setLayoutData(gd);
         aiInstructions.setText(rule.getAiAgentInstructions());
         aiInstructions.setEditable(false);
         aiInstructions.addMouseListener(listener);
      }

      /* flags */
      if ((rule.getFlags() & EventProcessingPolicyRule.STOP_PROCESSING) != 0)
      {
         final MouseListener listener = createMouseListener("Action");
         addActionGroupLabel(clientArea, i18n.tr("Stop event processing"), editor.getImageStop(), listener);
      }
      
      return clientArea;
   }

   /**
    * Add condition group opening text
    * 
    * @param parent parent composite
    * @param title group's title
    * @param needAnd true if AND clause have to be added
    */
   private void addActionGroupLabel(Composite parent, String title, Image image, MouseListener mouseListener)
   {
      CLabel label = createCLabel(parent, 0, true);
      label.setImage(image);
      label.setText(title);
      label.addMouseListener(mouseListener);
   }

   /**
    * @return the ruleNumber
    */
   public int getRuleNumber()
   {
      return ruleNumber;
   }

   /**
    * @param ruleNumber the ruleNumber to set
    */
   public void setRuleNumber(int ruleNumber)
   {
      this.ruleNumber = ruleNumber;
      rule.setRuleNumber(ruleNumber);
      if (!isDisposed())
      {
         ruleNumberLabel.setText(Integer.toString(ruleNumber));
         leftPanel.layout();
      }
   }

   /**
    * @param verticalLayout the verticalLayout to set
    */
   public void setVerticalLayout(boolean verticalLayout, boolean doLayout)
   {
      this.verticalLayout = verticalLayout;
      GridLayout layout = (GridLayout)mainArea.getLayout();
      layout.numColumns = verticalLayout ? 1 : 2;
      if (doLayout)
         layout();
   }

   /**
    * Check if widget is in collapsed state.
    * 
    * @return true if widget is in collapsed state
    */
   public boolean isCollapsed()
   {
      return collapsed;
   }

   /**
    * Set widget's to collapsed or expanded state
    * 
    * @param collapsed true to collapse widget, false to expand
    * @param doLayout if set to true, view layout will be updated
    */
   public void setCollapsed(boolean collapsed, boolean doLayout)
   {
      this.collapsed = collapsed;
      expandButton.setImage(collapsed ? SharedIcons.IMG_EXPAND : SharedIcons.IMG_COLLAPSE);
      expandButton.setToolTipText(collapsed ? i18n.tr("Expand rule") : i18n.tr("Collapse rule"));
      mainArea.setVisible(!collapsed);
      ((GridData)mainArea.getLayoutData()).exclude = collapsed;
      ((GridData)leftPanel.getLayoutData()).verticalSpan = collapsed ? 1 : 2;
      if (doLayout)
         editor.updateEditorAreaLayout();
   }

   /**
    * Edit rule
    */
   private void editRule(String pageId)
   {
      modified = false;

      PreferenceManager pm = new PreferenceManager();
      pm.addToRoot(new PreferenceNode("Condition", new RuleCondition(this)));
      pm.addTo("Condition", new PreferenceNode("Events", new RuleEvents(this)));
      pm.addTo("Condition", new PreferenceNode("SourceObjects", new RuleSourceObjects(this)));
      pm.addTo("Condition", new PreferenceNode("TimeFilter", new RuleTimeFilter(this)));
      pm.addTo("Condition", new PreferenceNode("SeverityFilter", new RuleSeverityFilter(this)));
      pm.addTo("Condition", new PreferenceNode("FilteringScript", new RuleFilteringScript(this)));
      pm.addToRoot(new PreferenceNode("Action", new RuleAction(this)));
      pm.addTo("Action", new PreferenceNode("Alarm", new RuleAlarm(this)));
      pm.addTo("Action", new PreferenceNode("Downtime", new RuleDowntimeControl(this)));
      pm.addTo("Action", new PreferenceNode("PersistentStorage", new RulePersistentStorage(this)));
      pm.addTo("Action", new PreferenceNode("CustomAttributes", new RuleCustomAttribute(this)));
      pm.addTo("Action", new PreferenceNode("ServerActions", new RuleServerActions(this)));
      pm.addTo("Action", new PreferenceNode("ActionScript", new RuleActionScript(this)));
      pm.addTo("Action", new PreferenceNode("TimerCancellations", new RuleTimerCancellations(this)));
      pm.addTo("Action", new PreferenceNode("AIAgentInstructions", new RuleAiAgentInstructions(this)));
      pm.addToRoot(new PreferenceNode("Comments", new RuleComments(this)));

      PropertyDialog dlg = new PropertyDialog(editor.getWindow().getShell(), pm, String.format(i18n.tr("Edit Rule %d"), ruleNumber)) {
         @Override
         protected TreeViewer createTreeViewer(Composite parent)
         {
            TreeViewer viewer = super.createTreeViewer(parent);
            viewer.expandAll();
            viewer.setAutoExpandLevel(TreeViewer.ALL_LEVELS);
            return viewer;
         }
      };
      dlg.setSelectedNode(pageId);
      dlg.open();
      if (modified)
      {
         if (rule.isDisabled())
            headerLabel.setText(rule.getComments() + i18n.tr(" (disabled)"));
         else if (rule.isFilterEmpty())
            headerLabel.setText(rule.getComments() + i18n.tr(" (empty filter)"));
         else
            headerLabel.setText(rule.getComments());

         updateBackground();

         condition.replaceClientArea();
         action.replaceClientArea();
         editor.updateEditorAreaLayout();
         editor.setModified(true);
      }
   }

   /**
    * @return the rule
    */
   public EventProcessingPolicyRule getRule()
   {
      return rule;
   }

   /**
    * @return the modified
    */
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
   }

   /**
    * @return the editor
    */
   public EventProcessingPolicyEditor getEditorView()
   {
      return editor;
   }

   /**
    * Process mouse click on rule number
    * 
    * @param e mouse event
    */
   private void processRuleMouseEvent(MouseEvent e)
   {
      boolean ctrlPressed = (e.stateMask & SWT.MOD1) != 0;
      boolean shiftPressed = (e.stateMask & SWT.SHIFT) != 0;

      if (ctrlPressed)
      {
         if (selected)
            editor.removeFromSelection(this);
         else
            editor.addToSelection(this, false);
      }
      else if (shiftPressed)
      {
         editor.addToSelection(this, true);
      }
      else if (!selected)
      {
         editor.setSelection(this);
      }
   }

   /**
    * @return the selected
    */
   public boolean isSelected()
   {
      return selected;
   }

   /**
    * Set selection status
    * 
    * @param selected the selected to set
    */
   public void setSelected(boolean selected)
   {
      this.selected = selected;
      final Color color = ThemeEngine
            .getBackgroundColor(selected ? "RuleEditor.Title.Selected" : ((rule.isDisabled() || rule.isFilterEmpty()) ? "RuleEditor.Title.Disabled" : "RuleEditor.Title.Normal"));

      leftPanel.setBackground(color);
      for(Control c : leftPanel.getChildren())
         c.setBackground(color);
      leftPanel.redraw();

      header.setBackground(color);
      headerLabel.setBackground(color);
      expandButton.setBackground(color);
      editButton.setBackground(color);
      header.redraw();
   }

   /**
    * Enable or disable rule
    * 
    * @param enabled true to enable rule
    */
   public void enableRule(boolean enabled)
   {
      if (enabled == !rule.isDisabled())
         return;

      if (enabled)
      {
         rule.setFlags(rule.getFlags() & ~EventProcessingPolicyRule.DISABLED);
         if (rule.isFilterEmpty())
            headerLabel.setText(rule.getComments() + i18n.tr(" (empty filter)"));
         else
            headerLabel.setText(rule.getComments());
      }
      else
      {
         rule.setFlags(rule.getFlags() | EventProcessingPolicyRule.DISABLED);
         headerLabel.setText(rule.getComments() + i18n.tr(" (disabled)"));
      }

      modified = true;
      updateBackground();
      editor.setModified(true);
   }

   /**
    * Update background color after rule status change
    */
   private void updateBackground()
   {
      final Color color = ThemeEngine.getBackgroundColor(
            selected ? "RuleEditor.Title.Selected" : ((rule.isDisabled() || rule.isFilterEmpty()) ? "RuleEditor.Title.Disabled" : "RuleEditor.Title.Normal"));
      leftPanel.setBackground(color);
      for(Control c : leftPanel.getChildren())
         c.setBackground(color);
      header.setBackground(color);
      headerLabel.setBackground(color);
      expandButton.setBackground(color);
      editButton.setBackground(color);
   }

   /**
    * DropTargetListener for Rule editor
    */
   private class RuleDropTargetListener implements DropTargetListener
   {
      /**
       * @see org.eclipse.swt.dnd.DropTargetListener#dragEnter(org.eclipse.swt.dnd.DropTargetEvent)
       */
      @Override
      public void dragEnter(DropTargetEvent event)
      {
      }

      /**
       * @see org.eclipse.swt.dnd.DropTargetListener#dragOver(org.eclipse.swt.dnd.DropTargetEvent)
       */
      @Override
      public void dragOver(DropTargetEvent event)
      {
      }

      /**
       * @see org.eclipse.swt.dnd.DropTargetListener#dragLeave(org.eclipse.swt.dnd.DropTargetEvent)
       */
      @Override
      public void dragLeave(DropTargetEvent event)
      {
      }

      /**
       * @see org.eclipse.swt.dnd.DropTargetListener#dropAccept(org.eclipse.swt.dnd.DropTargetEvent)
       */
      @Override
      public void dropAccept(DropTargetEvent event)
      {
      }

      /**
       * @see org.eclipse.swt.dnd.DropTargetListener#dragOperationChanged(org.eclipse.swt.dnd.DropTargetEvent)
       */
      @Override
      public void dragOperationChanged(DropTargetEvent event)
      {
      }

      /**
       * @see org.eclipse.swt.dnd.DropTargetListener#drop(org.eclipse.swt.dnd.DropTargetEvent)
       */
      @Override
      public void drop(DropTargetEvent event)
      {
         if (!(event.data instanceof IStructuredSelection))
            return;
         
         Object object = ((IStructuredSelection)event.data).getFirstElement();
         if (!(object instanceof RuleEditor))
            return;
         
         RuleEditor.this.editor.moveSelection(RuleEditor.this);
      }
   }

   /**
    * DragSourceListener for Rule editor
    */
   private static class RuleDragSourceListener implements DragSourceListener
   {
      private RuleEditor editor;

      public RuleDragSourceListener(RuleEditor editor)
      {
         this.editor = editor;
      }

      /**
       * @see org.eclipse.swt.dnd.DragSourceListener#dragStart(org.eclipse.swt.dnd.DragSourceEvent)
       */
      @Override
      public void dragStart(DragSourceEvent event)
      {
         LocalSelectionTransfer.getTransfer().setSelection(new StructuredSelection(editor));
      }

      /**
       * @see org.eclipse.swt.dnd.DragSourceListener#dragFinished(org.eclipse.swt.dnd.DragSourceEvent)
       */
      @Override
      public void dragFinished(DragSourceEvent event)
      {
      }

      /**
       * @see org.eclipse.swt.dnd.DragSourceListener#dragSetData(org.eclipse.swt.dnd.DragSourceEvent)
       */
      @Override
      public void dragSetData(DragSourceEvent event)
      {
         event.data = LocalSelectionTransfer.getTransfer().getSelection();
      }
   }
}
