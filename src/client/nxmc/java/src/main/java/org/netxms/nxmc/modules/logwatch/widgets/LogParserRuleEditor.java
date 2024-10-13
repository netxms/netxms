/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.logwatch.widgets;

import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Combo;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Display;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Spinner;
import org.netxms.client.events.EventTemplate;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.widgets.Card;
import org.netxms.nxmc.base.widgets.DashboardComposite;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.LabeledSpinner;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.base.widgets.helpers.SelectorConfigurator;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.events.widgets.EventSelector;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserContext;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserEvent;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserMatch;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserMetric;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserRule;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserType;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.WidgetHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Editor widget for single log parser rule
 */
public class LogParserRuleEditor extends DashboardComposite
{
   private static final Color CONDITION_BORDER_COLOR = new Color(Display.getCurrent(), 198, 214, 172);
   private static final Color ACTION_BORDER_COLOR = new Color(Display.getCurrent(), 186, 176, 201);
   private static final Color TITLE_COLOR = new Color(Display.getCurrent(), 0, 0, 0);

   private final I18n i18n = LocalizationHelper.getI18n(LogParserRuleEditor.class);

   private LogParserRule rule;
   private LogParserEditor editor;
   private LabeledText name;
   private LabeledText regexp;
   private Button checkboxInvert;
   private Button checkboxReset;
   private LabeledSpinner repeatCount;
   private Spinner timeRange;
   private Combo timeUnits;
   private LabeledText severity;
   private LabeledText facility;
   private LabeledText tag;
   private LabeledText logName;
   private LabeledText activeContext;
   private LabeledText description;
   private LabeledText agentAction;
   private EventSelector event;
   private LabeledText eventTag;
   private LabeledText context;
   private Combo contextAction;
   private Combo contextResetMode;
   private Button checkboxBreak;
   private Button checkboxDoNotSaveToDB;
   private ImageHyperlink addMetric;
   private Composite pushDataArea;

   /**
    * @param parent
    * @param style
    */
   public LogParserRuleEditor(Composite parent, LogParserRule rule, final LogParserEditor editor)
   {
      super(parent, SWT.NONE);

      this.rule = rule;
      this.editor = editor;

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      setLayout(layout);

      Label separator = new Label(this, SWT.HORIZONTAL | SWT.SEPARATOR);
      GridData gd = new GridData();
      gd.horizontalSpan = 2;
      gd.horizontalAlignment = SWT.FILL;
      separator.setLayoutData(gd);

      final Composite controlBar = new Composite(this, SWT.NONE);
      controlBar.setLayout(new RowLayout(SWT.HORIZONTAL));
      controlBar.setBackground(parent.getBackground());
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.horizontalAlignment = SWT.RIGHT;
      controlBar.setLayoutData(gd);
      fillControlBar(controlBar);

      name = new LabeledText(this, SWT.NONE);
      name.setLabel("Name");
      name.setBackground(parent.getBackground());
      gd = new GridData();
      gd.horizontalSpan = 2;
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      name.setLayoutData(gd);
      name.setText((rule.getName() != null) ? rule.getName() : "");
      name.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            editor.fireModifyListeners();
         }
      });

      final Card condition = new Card(this, i18n.tr("Condition")) {
         @Override
         protected Control createClientArea(Composite parent)
         {
            setTitleBackground(CONDITION_BORDER_COLOR);
            setTitleColor(TITLE_COLOR);
            return createMatchingArea(parent);
         }
      };
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      condition.setLayoutData(gd);

      final Card action = new Card(this, i18n.tr("Action")) {
         @Override
         protected Control createClientArea(Composite parent)
         {
            setTitleBackground(ACTION_BORDER_COLOR);
            setTitleColor(TITLE_COLOR);
            return createActionArea(parent);
         }
      };
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      action.setLayoutData(gd);
   }

   /**
    * Fill control bar
    * 
    * @param parent
    */
   private void fillControlBar(Composite parent)
   {
      ImageHyperlink link = new ImageHyperlink(parent, SWT.NONE);
      link.setImage(SharedIcons.IMG_UP);
      link.setToolTipText(i18n.tr("Move up"));
      link.setBackground(parent.getBackground());
      link.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            editor.moveRuleUp(rule);
         }
      });

      link = new ImageHyperlink(parent, SWT.NONE);
      link.setImage(SharedIcons.IMG_DOWN);
      link.setToolTipText(i18n.tr("Move down"));
      link.setBackground(parent.getBackground());
      link.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            editor.moveRuleDown(rule);
         }
      });

      link = new ImageHyperlink(parent, SWT.NONE);
      link.setImage(SharedIcons.IMG_DELETE_OBJECT);
      link.setToolTipText(i18n.tr("Delete"));
      link.setBackground(parent.getBackground());
      link.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            editor.deleteRule(rule);
         }
      });
   }

   /**
    * Create matching area
    */
   private Composite createMatchingArea(Composite parent)
   {
      Composite area = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      area.setLayout(layout);

      final ModifyListener listener = new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            editor.fireModifyListeners();
         }
      };

      Composite matcher = new Composite(area, SWT.NONE);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      matcher.setLayoutData(gd);

      layout = new GridLayout();
      layout.numColumns = 2;
      layout.marginWidth = 0;
      matcher.setLayout(layout);

      regexp = new LabeledText(matcher, SWT.NONE);
      regexp.setLabel(i18n.tr("Matching regular expression"));
      regexp.setText(rule.getMatch().getMatch());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      regexp.setLayoutData(gd);
      regexp.getTextControl().addModifyListener(listener);

      checkboxInvert = new Button(matcher, SWT.CHECK);
      checkboxInvert.setText(i18n.tr("Invert"));
      checkboxInvert.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editor.fireModifyListeners();
         }
      });
      checkboxInvert.setSelection(rule.getMatch().getInvert());

      if (editor.getParserType() == LogParserType.POLICY)
      {
         Composite matcherRepeatConf = new Composite(matcher, SWT.NONE);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = 2;
         matcherRepeatConf.setLayoutData(gd);

         layout = new GridLayout();
         layout.numColumns = 3;
         layout.marginWidth = 0;
         matcherRepeatConf.setLayout(layout);

         repeatCount = new LabeledSpinner(matcherRepeatConf, SWT.NONE);
         repeatCount.setLabel(i18n.tr("Repeat count"));
         repeatCount.getSpinnerControl().setMinimum(0);
         repeatCount.setSelection(rule.getMatch().getRepeatCount());
         repeatCount.getSpinnerControl().addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e)
            {
               editor.fireModifyListeners();
            }
         });

         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         repeatCount.setLayoutData(gd);

         Composite timeBackGroup = new Composite(matcherRepeatConf, SWT.NONE);
         layout = new GridLayout();
         layout.marginWidth = 0;
         layout.marginHeight = 0;
         layout.horizontalSpacing = WidgetHelper.OUTER_SPACING;
         layout.numColumns = 2;
         timeBackGroup.setLayout(layout);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         timeBackGroup.setLayoutData(gd);

         timeRange = WidgetHelper.createLabeledSpinner(timeBackGroup, SWT.BORDER, i18n.tr("Repeat interval"), 1, 10000, WidgetHelper.DEFAULT_LAYOUT_DATA);
         timeRange.setSelection(rule.getMatch().getTimeRagne());
         timeRange.addModifyListener(listener);

         timeUnits = WidgetHelper.createLabeledCombo(timeBackGroup, SWT.READ_ONLY, "", WidgetHelper.DEFAULT_LAYOUT_DATA);
         timeUnits.add("Seconds");
         timeUnits.add("Minutes");
         timeUnits.add("Hours");
         timeUnits.select(rule.getMatch().getTimeUnit());
         timeUnits.addModifyListener(listener);

         checkboxReset = new Button(matcherRepeatConf, SWT.CHECK);
         checkboxReset.setText("Reset repeat count");
         checkboxReset.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               editor.fireModifyListeners();
            }
         });
         checkboxReset.setSelection(rule.getMatch().getReset());
      }

      severity = new LabeledText(area, SWT.NONE);
      severity.setLabel(editor.getParserType() == LogParserType.SYSLOG ? i18n.tr("Severity") : i18n.tr("Level"));
      severity.setText(rule.getSeverityOrLevel(editor.getParserType() == LogParserType.SYSLOG));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      severity.setLayoutData(gd);
      severity.getTextControl().addModifyListener(listener);

      facility = new LabeledText(area, SWT.NONE);
      facility.setLabel(editor.getParserType() == LogParserType.SYSLOG ? i18n.tr("Facility") : i18n.tr("Event ID or ID range"));
      facility.setText(rule.getFacilityOrId(editor.getParserType() == LogParserType.SYSLOG));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      facility.setLayoutData(gd);
      facility.getTextControl().addModifyListener(listener);

      tag = new LabeledText(area, SWT.NONE);
      tag.setLabel(editor.getParserType() == LogParserType.SYSLOG ? i18n.tr("Tag") : i18n.tr("Source"));
      tag.setText(rule.getTagOrSource(editor.getParserType() == LogParserType.SYSLOG));
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      tag.setLayoutData(gd);
      tag.getTextControl().addModifyListener(listener);

      if (editor.getParserType() == LogParserType.WIN_EVENT)
      {
         logName = new LabeledText(area, SWT.NONE);
         logName.setLabel("Log name");
         logName.setText(rule.getLogName());
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         logName.setLayoutData(gd);
         logName.getTextControl().addModifyListener(listener);
      }

      if (editor.getParserType() == LogParserType.POLICY)
      {
         activeContext = new LabeledText(area, SWT.NONE);
         activeContext.setLabel(i18n.tr("Active context"));
         activeContext.setText((rule.getContext() != null) ? rule.getContext() : "");
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         activeContext.setLayoutData(gd);
         activeContext.getTextControl().addModifyListener(listener);
      }

      description = new LabeledText(area, SWT.NONE);
      description.setLabel(i18n.tr("Description"));
      description.setText((rule.getDescription() != null) ? rule.getDescription() : "");
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      description.setLayoutData(gd);
      description.getTextControl().addModifyListener(listener);

      updateWindowsEventLogFields();
      return area;
   }

   /**
    * Enables or disables Windows syslog fields depending on filename field
    */
   public void updateWindowsEventLogFields()
   {
      if (editor.getParserType() == LogParserType.SYSLOG)
         return;

      if (editor.isWindowsEventLogParser() || editor.getParserType() == LogParserType.WIN_EVENT)
      {
         severity.setEnabled(true);
         facility.setEnabled(true);
         tag.setEnabled(true);
      }
      else
      {
         severity.setEnabled(false);
         facility.setEnabled(false);
         tag.setEnabled(false);
      }
   }

   /**
    * Create action area
    */
   private Composite createActionArea(Composite parent)
   {
      Composite area = new Composite(parent, SWT.NONE);

      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      area.setLayout(layout);

      int eventCode = 0;
      if (rule.getEvent() != null)
      {
         try
         {
            eventCode = Integer.parseInt(rule.getEvent().getEvent());
         }
         catch(NumberFormatException e)
         {
            EventTemplate tmpl = Registry.getSession().findEventTemplateByName(rule.getEvent().getEvent());
            if (tmpl != null)
               eventCode = tmpl.getCode();
         }
      }

      event = new EventSelector(area, SWT.NONE, new SelectorConfigurator().setUseHyperlink(true).setShowClearButton(true));
      event.setLabel(i18n.tr("Generate event"));
      event.setEventCode(eventCode);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      event.setLayoutData(gd);
      event.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            editor.fireModifyListeners();
            eventTag.setEnabled(event.getEventCode() != 0);
         }
      });

      String eventTagText = "";
      if (rule.getEvent() != null)
      {
         eventTagText = rule.getEvent().getEventTag() != null ? rule.getEvent().getEventTag() : "";
      }

      eventTag = new LabeledText(area, SWT.NONE);
      eventTag.setLabel(i18n.tr("Event tag"));
      eventTag.setText(eventTagText);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      eventTag.setLayoutData(gd);
      eventTag.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            editor.fireModifyListeners();
         }
      });
      eventTag.setEnabled(event.getEventCode() != 0);

      if (editor.getParserType() == LogParserType.POLICY)
      {
         final LogParserContext contextDefinition = rule.getContextDefinition();

         context = new LabeledText(area, SWT.NONE);
         context.setLabel(i18n.tr("Change context"));
         context.setText((contextDefinition != null) ? contextDefinition.getData() : ""); //$NON-NLS-1$
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = 2;
         context.setLayoutData(gd);
         context.getTextControl().addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e)
            {
               boolean contextSet = !context.getText().trim().isEmpty();
               contextAction.setEnabled(contextSet);
               contextResetMode.setEnabled(contextSet && (contextAction.getSelectionIndex() == 0));
               editor.fireModifyListeners();
            }
         });

         final Composite contextOptions = new Composite(area, SWT.NONE);
         layout = new GridLayout();
         layout.marginHeight = 0;
         layout.marginWidth = 0;
         layout.numColumns = 2;
         layout.makeColumnsEqualWidth = true;
         contextOptions.setLayout(layout);
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = 2;
         contextOptions.setLayoutData(gd);

         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         contextAction = WidgetHelper.createLabeledCombo(contextOptions, SWT.READ_ONLY, i18n.tr("Context action"), gd);
         contextAction.add(i18n.tr("Activate"));
         contextAction.add(i18n.tr("Clear"));
         if (contextDefinition != null)
         {
            contextAction.select(contextDefinition.getAction());
         }
         else
         {
            contextAction.select(0);
            contextAction.setEnabled(false);
         }
         contextAction.addSelectionListener(new SelectionListener() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               contextResetMode.setEnabled(contextAction.getSelectionIndex() == 0);
               editor.fireModifyListeners();
            }

            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               widgetSelected(e);
            }
         });

         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         contextResetMode = WidgetHelper.createLabeledCombo(contextOptions, SWT.READ_ONLY, i18n.tr("Context reset mode"), gd);
         contextResetMode.add(i18n.tr("Automatic"));
         contextResetMode.add(i18n.tr("Manual"));
         if (contextDefinition != null)
         {
            contextResetMode.select(contextDefinition.getReset());
            contextResetMode.setEnabled(contextDefinition.getAction() == 0);
         }
         else
         {
            contextResetMode.select(0);
            contextResetMode.setEnabled(false);
         }
         contextResetMode.addSelectionListener(new SelectionListener() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               editor.fireModifyListeners();
            }

            @Override
            public void widgetDefaultSelected(SelectionEvent e)
            {
               widgetSelected(e);
            }
         });
      }

      if (editor.getParserType() == LogParserType.POLICY)
      {
         agentAction = new LabeledText(area, SWT.NONE);
         agentAction.setLabel(i18n.tr("Execute action"));
         agentAction.setText((rule.getAgentAction() != null) ? rule.getAgentAction().getAction() : ""); //$NON-NLS-1$
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         gd.horizontalSpan = 2;
         agentAction.setLayoutData(gd);
         agentAction.getTextControl().addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e)
            {
               editor.fireModifyListeners();
            }
         });

         pushDataArea = new Composite(area, SWT.NONE);
         GridLayout pushDataAreaLayout = new GridLayout();
         pushDataAreaLayout.marginHeight = 0;
         pushDataAreaLayout.marginWidth = 0;
         pushDataAreaLayout.makeColumnsEqualWidth = false;
         pushDataArea.setLayout(pushDataAreaLayout);
         pushDataArea.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false, 2, 1));

         for(LogParserMetric metric : rule.getMetrics())
            createMetricEditor(metric);

         addMetric = new ImageHyperlink(pushDataArea, SWT.NONE);
         addMetric.setText(i18n.tr("Add metric"));
         addMetric.setImage(SharedIcons.IMG_ADD_OBJECT);
         addMetric.addHyperlinkListener(new HyperlinkAdapter() {
            @Override
            public void linkActivated(HyperlinkEvent e)
            {
               addMetric();
            }            
         });
      }

      checkboxBreak = new Button(area, SWT.CHECK);
      checkboxBreak.setText(i18n.tr("Break"));
      checkboxBreak.setSelection(rule.isBreakProcessing());
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      checkboxBreak.setLayoutData(gd);
      checkboxBreak.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            editor.fireModifyListeners();
         }
      });

      if (editor.getParserType() != LogParserType.POLICY)
      {
         checkboxDoNotSaveToDB = new Button(area, SWT.CHECK);
         checkboxDoNotSaveToDB.setText(i18n.tr("Do not save to database"));
         checkboxDoNotSaveToDB.setSelection(rule.isDoNotSaveToDatabase());
         checkboxDoNotSaveToDB.addSelectionListener(new SelectionAdapter() {
            @Override
            public void widgetSelected(SelectionEvent e)
            {
               editor.fireModifyListeners();
            }
         });
      }

      return area;
   }

   /**
    * Create metric editor
    * 
    * @param metric metric to create editor for
    */
   private LogParserMetricEditor createMetricEditor(LogParserMetric metric)
   {
      LogParserMetricEditor editor = new LogParserMetricEditor(pushDataArea, metric, this);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      editor.setLayoutData(gd);
      metric.setEditor(editor);
      return editor;
   }

   /**
    * Add metric link action implementation
    */
   protected void addMetric()
   {
      LogParserMetric metric = new LogParserMetric();
      LogParserMetricEditor metricEditor = createMetricEditor(metric);
      metricEditor.moveAbove(addMetric);
      rule.getMetrics().add(metric);
      editor.updateScroller(); 
      fireModifyListeners();
      
   }

   /**
    * Delete metric link action implementation
    * 
    * @param metric to delete
    */
   public void deleteMetric(LogParserMetric metric)
   {
      rule.getMetrics().remove(metric);
      metric.getEditor().dispose();
      editor.updateScroller(); 
      getParent().layout(true, true);
      fireModifyListeners();
   }

   /**
    * Save data from controls into parser rule
    */
   public void save()
   {
      rule.setName(name.getText().trim());
      if (editor.getParserType() == LogParserType.POLICY)
      {
         rule.setMatch(new LogParserMatch(regexp.getText(), checkboxInvert.getSelection(), intOrNull(repeatCount.getText()),
               Integer.parseInt(timeRange.getText()) * (int)Math.pow(60, timeUnits.getSelectionIndex()),
               checkboxReset.getSelection()));
      }
      else
      {
         rule.setMatch(new LogParserMatch(regexp.getText(), checkboxInvert.getSelection(), null, 0, false));         
      }
      rule.setEventId(facility.getText());
      rule.setSeverityOrLevel(intOrNull(severity.getText()));
      rule.setTagOrSource(tag.getText());

      if (editor.getParserType() == LogParserType.WIN_EVENT)
         rule.setLogName(logName.getText());
      else
         rule.setLogName("");

      if (editor.getParserType() == LogParserType.POLICY)
      {
         rule.setContext(activeContext.getText().trim().isEmpty() ? null : activeContext.getText());
      }
      else
      {
         rule.setContext(null);
      }
      rule.setBreakProcessing(checkboxBreak.getSelection());
      rule.setDescription(description.getText());
      if (event.getEventCode() != 0)
      {
         rule.setEvent(new LogParserEvent(event.getEventName() != null ? event.getEventName() : Long.toString(event.getEventCode()),
               null, eventTag.getText().isEmpty() ? null : eventTag.getText()));
      }
      else
      {
         rule.setEvent(null);
      }

      if ((editor.getParserType() != LogParserType.POLICY) || context.getText().trim().isEmpty())
      {
         rule.setContextDefinition(null);
      }
      else
      {
         LogParserContext ctx = new LogParserContext();
         ctx.setData(context.getText());
         ctx.setAction(contextAction.getSelectionIndex());
         ctx.setReset(contextResetMode.getSelectionIndex());
         rule.setContextDefinition(ctx);
      }

      if (editor.getParserType() == LogParserType.POLICY)
      {
         rule.setAgentAction(agentAction.getText().trim());
      }
      else
         rule.setAgentAction("");
      
      if (editor.getParserType() == LogParserType.POLICY)
      {
         for(LogParserMetric metric : rule.getMetrics())
            metric.getEditor().save();   
      }
      else
         rule.setMetrics(null);

      if (editor.getParserType() != LogParserType.POLICY)
         rule.setDoNotSaveToDatabase(checkboxDoNotSaveToDB.getSelection());
      else
         rule.setDoNotSaveToDatabase(false);
   }

   /**
    * Return integer object created from input string or null if input text string is empty or cannot be parsed
    * 
    * @param text
    * @return
    */
   private Integer intOrNull(String text)
   {
      if (text.trim().isEmpty())
         return null;
      try
      {
         return Integer.parseInt(text);
      }
      catch(NumberFormatException e)
      {
         return null;
      }
   }

   /**
    * @return
    */
   public LogParserType getParserType()
   {
      return editor.getParserType();
   }

   public void fireModifyListeners()
   {
      editor.fireModifyListeners();
   }

   /**
    * @see org.eclipse.swt.widgets.Widget#dispose()
    */
   @Override
   public void dispose()
   {
      if (rule.getMetrics() != null)
         for(LogParserMetric metric : rule.getMetrics())
            metric.getEditor().dispose(); 
      super.dispose();
   }
   
   
}
