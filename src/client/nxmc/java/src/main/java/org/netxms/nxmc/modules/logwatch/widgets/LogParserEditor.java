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
package org.netxms.nxmc.modules.logwatch.widgets;

import java.util.Collections;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.Viewer;
import org.eclipse.jface.viewers.ViewerComparator;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.Text;
import org.netxms.nxmc.base.widgets.ImageHyperlink;
import org.netxms.nxmc.base.widgets.LabeledText;
import org.netxms.nxmc.base.widgets.Section;
import org.netxms.nxmc.base.widgets.SortableTableViewer;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.base.widgets.helpers.ExpansionEvent;
import org.netxms.nxmc.base.widgets.helpers.ExpansionListener;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logwatch.dialogs.LogMacroEditDialog;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParser;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserFile;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserModifyListener;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserRule;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserType;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.MacroListLabelProvider;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.tools.MessageDialogHelper;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;

/**
 * Log parser editor
 */
public class LogParserEditor extends Composite
{
   private static final Logger logger = LoggerFactory.getLogger(LogParserEditor.class);

   private static final int TAB_NONE = 0;
   private static final int TAB_BUILDER = 1;
   private static final int TAB_XML = 2;

   private static final int DEFAULT_FILE_CHECK_INTERVAL = 10000;

   private final I18n i18n = LocalizationHelper.getI18n(LogParserEditor.class);

   private CTabFolder tabFolder;
   private int currentTab = TAB_NONE;
   private Text xmlEditor;
   private Set<LogParserModifyListener> listeners = new HashSet<LogParserModifyListener>();
   private boolean enableModifyListeners = true;
   private LogParser parser = new LogParser();
   private ScrolledComposite scroller;
   private Composite visualEditorArea;
   private Composite rulesArea;
   private Composite fileArea;
   private ImageHyperlink addRuleLink;
   private ImageHyperlink addFileLink;
   private SortableTableViewer macroList;
   private LogParserType type;

   /* General section */
   private LabeledText textName;
   private Spinner spinnerFileCheckInterval;
   private Button checkProcessAll;

   /**
    * @param parent
    * @param style
    */
   public LogParserEditor(Composite parent, int style, LogParserType type)
   {
      super(parent, style);

      setLayout(new FillLayout());

      this.type = type;

      tabFolder = new CTabFolder(this, SWT.BOTTOM | SWT.FLAT | SWT.MULTI);
      tabFolder.setUnselectedImageVisible(true);
      WidgetHelper.disableTabFolderSelectionBar(tabFolder);
      tabFolder.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            String xml;
            switch(currentTab)
            {
               case TAB_BUILDER:
                  xml = (parser != null) ? buildParserXml() : "";
                  break;
               case TAB_XML:
                  xml = xmlEditor.getText();
                  break;
               default:
                  xml = null;
                  break;
            }
            CTabItem tab = tabFolder.getSelection();
            currentTab = (tab != null) ? (Integer)tab.getData() : TAB_NONE;
            if (xml != null)
            {
               switch(currentTab)
               {
                  case TAB_BUILDER:
                     updateBuilderFromXml(xmlEditor.getText());
                     break;
                  case TAB_XML:
                     if (parser != null)
                        xmlEditor.setText(buildParserXml());
                     break;
                  default:
                     break;
               }
            }
         }
      });

      createVisualEditor();
      createTextEditor();

      currentTab = TAB_BUILDER;
      tabFolder.setSelection(0);
   }

   /**
    * Select XML editor
    */
   private void selectXmlEditor()
   {
      CTabItem tab = tabFolder.getSelection();
      if (tab != null && (Integer)tab.getData() == TAB_XML)
         return;

      for(CTabItem t : tabFolder.getItems())
      {
         if ((Integer)t.getData() == TAB_XML)
         {
            tabFolder.setSelection(t);
            currentTab = TAB_XML;
            break;
         }
      }
   }

   /**
    * Create policy visual editor
    */
   private void createVisualEditor()
   {
      scroller = new ScrolledComposite(tabFolder, SWT.V_SCROLL);
      scroller.setExpandHorizontal(true);
      scroller.setExpandVertical(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            updateScroller();
         }
      });

      final CTabItem tabItem = new CTabItem(tabFolder, SWT.NONE);
      tabItem.setText(i18n.tr("Editor"));
      tabItem.setImage(SharedIcons.IMG_EDIT);
      tabItem.setControl(scroller);
      tabItem.setData(TAB_BUILDER);

      visualEditorArea = new Composite(scroller, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      layout.makeColumnsEqualWidth = true;
      layout.horizontalSpacing = 8;
      layout.verticalSpacing = 8;
      layout.marginWidth = 8;
      layout.marginHeight = 8;
      visualEditorArea.setLayout(layout);
      visualEditorArea.setBackground(getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
      scroller.setContent(visualEditorArea);

      /* General section */
      Section section = new Section(visualEditorArea, i18n.tr("General"), true);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      gd.widthHint = 400;
      section.setLayoutData(gd);
      section.addExpansionListener(new ExpansionListener() {
         @Override
         public void expansionStateChanged(ExpansionEvent e)
         {
            ((GridData)((Control)e.widget).getLayoutData()).verticalAlignment = e.getState() ? SWT.FILL : SWT.TOP;
         }
      });

      createGeneralArea(section.getClient());

      /* Macros section */
      section = new Section(visualEditorArea, i18n.tr("Macros"), true);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.verticalAlignment = SWT.FILL;
      section.setLayoutData(gd);
      section.addExpansionListener(new ExpansionListener() {
         @Override
         public void expansionStateChanged(ExpansionEvent e)
         {
            ((GridData)((Control)e.widget).getLayoutData()).verticalAlignment = e.getState() ? SWT.FILL : SWT.TOP;
         }
      });

      createMacroSection(section.getClient());

      /* Rules section */
      section = new Section(visualEditorArea, i18n.tr("Rules"), true);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalSpan = 2;
      section.setLayoutData(gd);

      rulesArea = section.getClient();
      GridLayout rulesAreaLayout = new GridLayout();
      rulesAreaLayout.marginHeight = 0;
      rulesAreaLayout.marginWidth = 0;
      rulesAreaLayout.verticalSpacing = 1;
      rulesArea.setLayout(rulesAreaLayout);

      rulesArea.setBackground(visualEditorArea.getBackground());

      addRuleLink = new ImageHyperlink(rulesArea, SWT.NONE);
      addRuleLink.setText(i18n.tr("Add rule"));
      addRuleLink.setImage(SharedIcons.IMG_ADD_OBJECT);
      addRuleLink.setBackground(rulesArea.getBackground());
      addRuleLink.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addRule();
         }
      });
   }

   /**
    * Recalculate scroller content size and update scroller
    */
   void updateScroller()
   {
      visualEditorArea.layout(true, true);
      visualEditorArea.requestLayout();
      scroller.setMinSize(visualEditorArea.computeSize(scroller.getSize().x, SWT.DEFAULT));
   }

   /**
    * @param generalArea
    */
   private void createGeneralArea(Composite generalArea)
   {
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      generalArea.setLayout(layout);

      textName = new LabeledText(generalArea, SWT.NONE);
      textName.setLabel(i18n.tr("Parser name"));
      textName.setText((parser.getName() != null) ? parser.getName() : ""); //$NON-NLS-1$
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      gd.horizontalSpan = (type == LogParserType.POLICY) ? 1 : 2;
      gd.verticalAlignment = SWT.BOTTOM;
      textName.setLayoutData(gd);
      textName.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            fireModifyListeners();
         }
      });

      if (type == LogParserType.POLICY)
      {
         gd = new GridData();
         gd.horizontalAlignment = SWT.FILL;
         gd.verticalAlignment = SWT.BOTTOM;
         spinnerFileCheckInterval = WidgetHelper.createLabeledSpinner(generalArea, SWT.BORDER, i18n.tr("File check interval(ms)"), 1000, 60000, gd);
         spinnerFileCheckInterval.addModifyListener(new ModifyListener() {
            @Override
            public void modifyText(ModifyEvent e)
            {
               fireModifyListeners();
            }
         });
         spinnerFileCheckInterval.setSelection(parser.getFileCheckInterval() != null ? parser.getFileCheckInterval() : DEFAULT_FILE_CHECK_INTERVAL);
      }

      checkProcessAll = new Button(generalArea, SWT.CHECK);
      checkProcessAll.setText(i18n.tr("Always process all rules"));
      checkProcessAll.setSelection(parser.getProcessALL());
      checkProcessAll.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            fireModifyListeners();
         }
      });

      if (type == LogParserType.POLICY)
      {
         fileArea = new Composite(generalArea, SWT.NONE);

         layout = new GridLayout();
         layout.marginHeight = 0;
         layout.marginWidth = 0;
         fileArea.setLayout(layout);

         gd = new GridData();
         gd.horizontalSpan = 3;
         gd.horizontalAlignment = SWT.FILL;
         gd.grabExcessHorizontalSpace = true;
         fileArea.setLayoutData(gd);

         // add link to add file editor
         addFileLink = new ImageHyperlink(fileArea, SWT.NONE);
         addFileLink.setText(i18n.tr("Add file"));
         addFileLink.setImage(SharedIcons.IMG_ADD_OBJECT);
         addFileLink.addHyperlinkListener(new HyperlinkAdapter() {
            @Override
            public void linkActivated(HyperlinkEvent e)
            {
               addFile();
            }
         });
      }
   }

   /**
    * Create text editor for direct XML edit
    */
   private void createTextEditor()
   {
      xmlEditor = new Text(tabFolder, SWT.MULTI | SWT.H_SCROLL | SWT.V_SCROLL);
      xmlEditor.setFont(JFaceResources.getTextFont());

      final CTabItem tabItem = new CTabItem(tabFolder, SWT.NONE);
      tabItem.setText("XML");
      tabItem.setImage(SharedIcons.IMG_XML);
      tabItem.setControl(xmlEditor);
      tabItem.setData(TAB_XML);

      xmlEditor.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            fireModifyListeners();
         }
      });
   }

   /**
    * @param macroArea
    */
   private void createMacroSection(Composite macroArea)
   {
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      macroArea.setLayout(layout);

      final String[] columnNames = { i18n.tr("Name"), i18n.tr("Value") };
      final int[] columnWidths = { 100, 200 };

      macroList = new SortableTableViewer(macroArea, columnNames, columnWidths, 0, SWT.UP, SWT.BORDER);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      gd.grabExcessVerticalSpace = true;
      gd.verticalSpan = 3;
      gd.heightHint = 200;
      final Table table = macroList.getTable();
      table.setLayoutData(gd);

      macroList.setLabelProvider(new MacroListLabelProvider());
      macroList.setContentProvider(new ArrayContentProvider());
      macroList.setComparator(new ViewerComparator() {
         @SuppressWarnings("unchecked")
         @Override
         public int compare(Viewer viewer, Object e1, Object e2)
         {
            Entry<String, String> d1 = (Entry<String, String>)e1;
            Entry<String, String> d2 = (Entry<String, String>)e2;

            int result;
            switch((Integer)((SortableTableViewer)viewer).getTable().getSortColumn().getData("ID")) //$NON-NLS-1$
            {
               case 0:
                  result = (d1.getKey()).compareTo(d2.getKey());
                  break;
               case 1:
                  result = (d1.getValue()).compareTo(d2.getValue());
                  break;
               default:
                  result = 0;
                  break;
            }
            return (((SortableTableViewer)viewer).getTable().getSortDirection() == SWT.UP) ? result : -result;
         }
      });

      ImageHyperlink link = new ImageHyperlink(macroArea, SWT.NONE);
      link.setImage(SharedIcons.IMG_ADD_OBJECT);
      link.setText(i18n.tr("Add"));
      link.setBackground(macroArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      link.setLayoutData(gd);
      link.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            addMacro();
         }
      });

      link = new ImageHyperlink(macroArea, SWT.NONE);
      link.setImage(SharedIcons.IMG_EDIT);
      link.setText(i18n.tr("Edit"));
      link.setBackground(macroArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      link.setLayoutData(gd);
      link.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            editMacro();
         }
      });

      link = new ImageHyperlink(macroArea, SWT.NONE);
      link.setImage(SharedIcons.IMG_DELETE_OBJECT);
      link.setText(i18n.tr("Delete"));
      link.setBackground(macroArea.getBackground());
      gd = new GridData();
      gd.verticalAlignment = SWT.TOP;
      link.setLayoutData(gd);
      link.addHyperlinkListener(new HyperlinkAdapter() {
         @Override
         public void linkActivated(HyperlinkEvent e)
         {
            deleteMacro();
         }
      });
   }

   /**
    * @param modifyListener
    */
   public void addModifyListener(LogParserModifyListener modifyListener)
   {
      listeners.add(modifyListener);
   }

   /**
    * @param modifyListener
    */
   public void removeModifyListener(LogParserModifyListener modifyListener)
   {
      listeners.remove(modifyListener);
   }

   /**
    * Execute all registered modify listeners
    */
   public void fireModifyListeners()
   {
      if (enableModifyListeners)
         for(LogParserModifyListener l : listeners)
            l.modifyParser();
   }

   /**
    * Get parser XML
    * 
    * @return
    */
   public String getParserXml()
   {
      switch(currentTab)
      {
         case TAB_BUILDER:
            return buildParserXml();
         case TAB_XML:
            return xmlEditor.getText();
         default:
            return "<parser></parser>"; //$NON-NLS-1$
      }
   }

   /**
    * Build parser XML from current builder state
    * 
    * @return
    */
   private String buildParserXml()
   {
      if (type == LogParserType.POLICY)
      {
         for(LogParserFile file : parser.getFiles())
            file.getEditor().save();
         parser.setFileCheckInterval(spinnerFileCheckInterval.getSelection());
      }
      parser.setName(textName.getText());
      parser.setProcessALL(checkProcessAll.getSelection());

      for(LogParserRule rule : parser.getRules())
         rule.getEditor().save();

      try
      {
         return parser.createXml();
      }
      catch(Exception e)
      {
         logger.error("Unexpected error creating parser XML", e);
         return "<parser>\n</parser>"; //$NON-NLS-1$
      }
   }

   /**
    * Set parser XML
    * 
    * @param content
    */
   public void setParserXml(String xml)
   {
      enableModifyListeners = false;
      xmlEditor.setText(xml);
      updateBuilderFromXml(xml);
      enableModifyListeners = true;
   }

   /**
    * Update parser builder from XML
    * 
    * @param xml
    */
   private void updateBuilderFromXml(String xml)
   {
      if (parser != null)
      {
         for(LogParserFile file : parser.getFiles())
            file.getEditor().dispose();

         for(LogParserRule rule : parser.getRules())
            rule.getEditor().dispose();
      }

      try
      {
         parser = LogParser.createFromXml(xml);
      }
      catch(Exception e)
      {
         logger.error("Error creating log parser object from XML", e);
         MessageDialogHelper.openError(getShell(), i18n.tr("Error"), i18n.tr("Log parser definition is invalid"));
         parser = null;
         selectXmlEditor();
         return;
      }
      parser.setSyslogParser(type);

      /* general */
      if (type == LogParserType.POLICY)
      {
         for(LogParserFile file : parser.getFiles())
            createFileEditor(file).moveAbove(addFileLink);
         spinnerFileCheckInterval.setSelection(parser.getFileCheckInterval() != null ? parser.getFileCheckInterval() : DEFAULT_FILE_CHECK_INTERVAL);
      }
      textName.setText(parser.getName());
      checkProcessAll.setSelection(parser.getProcessALL());

      /* rules */
      for(LogParserRule rule : parser.getRules())
         createRuleEditor(rule).moveAbove(addRuleLink);

      /* macros */
      macroList.setInput(parser.getMacros().entrySet().toArray());

      updateScroller();
   }

   /**
    * @param rule
    */
   private LogParserRuleEditor createRuleEditor(LogParserRule rule)
   {
      LogParserRuleEditor editor = new LogParserRuleEditor(rulesArea, rule, this);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      editor.setLayoutData(gd);
      rule.setEditor(editor);
      return editor;
   }

   /**
    * @param addColumnLink
    */
   private void addRule()
   {
      LogParserRule rule = new LogParserRule();
      LogParserRuleEditor editor = createRuleEditor(rule);
      editor.moveAbove(addRuleLink);
      parser.getRules().add(rule);
      updateScroller();
      fireModifyListeners();
   }

   /**
    * @param file
    */
   private LogParserFileEditor createFileEditor(LogParserFile file)
   {
      LogParserFileEditor editor = new LogParserFileEditor(fileArea, file, this);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.grabExcessHorizontalSpace = true;
      editor.setLayoutData(gd);
      file.setEditor(editor);
      return editor;
   }

   /**
    * Adds file entry
    */
   private void addFile()
   {
      LogParserFile file = new LogParserFile();
      LogParserFileEditor editor = createFileEditor(file);
      editor.moveAbove(addFileLink);
      parser.getFiles().add(file);
      updateScroller();
      fireModifyListeners();
   }

   /**
    * Add new macro
    */
   private void addMacro()
   {
      LogMacroEditDialog dlg = new LogMacroEditDialog(getShell(), null, null);
      if (dlg.open() == Window.OK)
      {
         parser.getMacros().put(dlg.getName(), dlg.getValue());
         macroList.setInput(parser.getMacros().entrySet().toArray());
         fireModifyListeners();
      }
   }

   /**
    * Edit selected macro
    */
   @SuppressWarnings("unchecked")
   private void editMacro()
   {
      IStructuredSelection selection = macroList.getStructuredSelection();
      if (selection.size() != 1)
         return;

      Entry<String, String> e = (Entry<String, String>)selection.getFirstElement();
      LogMacroEditDialog dlg = new LogMacroEditDialog(getShell(), e.getKey(), e.getValue());
      if (dlg.open() == Window.OK)
      {
         parser.getMacros().put(dlg.getName(), dlg.getValue());
         macroList.setInput(parser.getMacros().entrySet().toArray());
         fireModifyListeners();
      }
   }

   /**
    * Delete selected macro
    */
   @SuppressWarnings("unchecked")
   private void deleteMacro()
   {
      IStructuredSelection selection = macroList.getStructuredSelection();
      if (selection.size() == 0)
         return;

      Map<String, String> macros = parser.getMacros();
      for(Object o : selection.toList())
      {
         macros.remove(((Entry<String, String>)o).getKey());
      }
      macroList.setInput(macros.entrySet().toArray());
      fireModifyListeners();
   }

   /**
    * Delete rule
    * 
    * @param rule
    */
   public void deleteRule(LogParserRule rule)
   {
      parser.getRules().remove(rule);
      rule.getEditor().dispose();
      updateScroller();
      getParent().layout(true, true);
      fireModifyListeners();
   }

   /**
    * Delete file
    * 
    * @param file
    */
   public void deleteFile(LogParserFile file)
   {
      parser.getFiles().remove(file);
      file.getEditor().dispose();
      updateScroller();
      getParent().layout(true, true);
      fireModifyListeners();
   }

   /**
    * Move given rule up
    * 
    * @param rule
    */
   public void moveRuleUp(LogParserRule rule)
   {
      int index = parser.getRules().indexOf(rule);
      if (index < 1)
         return;

      rule.getEditor().moveAbove(parser.getRules().get(index - 1).getEditor());
      Collections.swap(parser.getRules(), index - 1, index);
      updateScroller();
      getParent().layout(true, true);
      fireModifyListeners();
   }

   /**
    * Move given rule down
    * 
    * @param rule
    */
   public void moveRuleDown(LogParserRule rule)
   {
      int index = parser.getRules().indexOf(rule);
      if ((index < 0) || (index >= parser.getRules().size() - 1))
         return;

      rule.getEditor().moveBelow(parser.getRules().get(index + 1).getEditor());
      Collections.swap(parser.getRules(), index + 1, index);
      updateScroller();
      getParent().layout(true, true);
      fireModifyListeners();
   }

   /**
    * @return parser type
    */
   public LogParserType getParserType()
   {
      return type;
   }

   /**
    * Checks if file name starts with "*" and is not null, then it it Windows Event Log parser.
    * 
    * @return isWindowsEventLogParser
    */
   public boolean isWindowsEventLogParser()
   {
      boolean isWindowsEventLogParser = false;
      for(LogParserFile file : parser.getFiles())
      {
         if (file.getEditor().getFile().startsWith("*"))
         {
            isWindowsEventLogParser = true;
            break;
         }
      }

      return isWindowsEventLogParser;
   }

   public void updateRules()
   {
      for(LogParserRule rule : parser.getRules())
      {
         if (rule.getEditor() != null)
            rule.getEditor().updateWindowsEventLogFields();
      }
   }

   public boolean isEditorTabSelected()
   {
      return currentTab == TAB_XML;
   }
}
