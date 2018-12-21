/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.datacollection.widgets;

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
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Spinner;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.netxms.ui.eclipse.console.resources.SharedIcons;
import org.netxms.ui.eclipse.datacollection.Messages;
import org.netxms.ui.eclipse.datacollection.dialogs.LogMacroEditDialog;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.LogParser;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.LogParserFile;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.LogParserFileEditor;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.LogParserModifyListener;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.LogParserRule;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.LogParserRuleEditor;
import org.netxms.ui.eclipse.datacollection.widgets.helpers.MacroListLabelProvider;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;
import org.netxms.ui.eclipse.tools.WidgetFactory;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.netxms.ui.eclipse.widgets.SortableTableViewer;

/**
 * Log parser editor
 */
public class LogParserEditor extends Composite
{
	private static final int TAB_NONE = 0;
	private static final int TAB_BUILDER = 1;
	private static final int TAB_XML = 2;
	
	private CTabFolder tabFolder;
	private int currentTab = TAB_NONE;
	private Text xmlEditor;
	private FormToolkit toolkit;
	private ScrolledForm form;
	private Set<LogParserModifyListener> listeners = new HashSet<LogParserModifyListener>();
	private LogParser parser = new LogParser();
	private Composite rulesArea;
   private Composite fileArea;
	private ImageHyperlink addColumnLink;
   private ImageHyperlink addFileLink;
	private SortableTableViewer macroList;
	private boolean isSyslogParser;
	
	/* General section */
   private LabeledText labelName;
   private Spinner spinnerTrace;
   private Button checkProcessAll;

	/**
	 * @param parent
	 * @param style
	 */
	public LogParserEditor(Composite parent, int style, boolean isSyslogParser)
	{
		super(parent, style);
		
		setLayout(new FillLayout());
		
		this.isSyslogParser = isSyslogParser;
		
		tabFolder = new CTabFolder(this, SWT.BOTTOM | SWT.FLAT | SWT.MULTI);
		tabFolder.setUnselectedImageVisible(true);
		tabFolder.setSimple(true);
		tabFolder.addSelectionListener(new SelectionListener() {
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
			
			@Override
			public void widgetDefaultSelected(SelectionEvent e)
			{
				widgetSelected(e);
			}
		});

		createForm();
		createTextEditor();
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
	 * Create policy edit form
	 */
	private void createForm()
	{
		/* FORM */
		toolkit = new FormToolkit(getDisplay());
		form = toolkit.createScrolledForm(tabFolder);
		form.setText(Messages.get().LogParserEditor_LogParser);

		final CTabItem tabItem = new CTabItem(tabFolder, SWT.NONE);
		tabItem.setText(Messages.get().LogParserEditor_Editor);
		tabItem.setImage(SharedIcons.IMG_EDIT);
		tabItem.setControl(form);
		tabItem.setData(TAB_BUILDER);
		
		TableWrapLayout layout = new TableWrapLayout();
		form.getBody().setLayout(layout);
		
		/* General section */
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
      section.setText("General");
      TableWrapData td = new TableWrapData();
      td.align = TableWrapData.FILL;
      td.grabHorizontal = true;
      section.setLayoutData(td);
      
      final Composite generalArea = toolkit.createComposite(section);
      createGeneralArea(generalArea);      
      section.setClient(generalArea);
      
      /* Macros section */
      section = toolkit.createSection(form.getBody(), Section.TITLE_BAR | Section.COMPACT | Section.TWISTIE);
      section.setText(Messages.get().LogParserEditor_Macros);
      td = new TableWrapData();
      td.align = TableWrapData.FILL;
      section.setLayoutData(td);

      final Composite macroArea = toolkit.createComposite(section);
      createMacroSection(macroArea);
      
      section.setClient(macroArea);

		/* Rules section */
		section = toolkit.createSection(form.getBody(), Section.TITLE_BAR | Section.COMPACT | Section.TWISTIE | Section.EXPANDED);
		section.setText(Messages.get().LogParserEditor_Rules);
		td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		rulesArea = toolkit.createComposite(section);
		GridLayout rulesAreaLayout = new GridLayout();
		rulesAreaLayout.marginHeight = 0;
		rulesAreaLayout.marginWidth = 0;
		rulesAreaLayout.verticalSpacing = 1;
		rulesArea.setLayout(rulesAreaLayout);
		
		section.setClient(rulesArea);
		
		addColumnLink = toolkit.createImageHyperlink(rulesArea, SWT.NONE);
		addColumnLink.setText(Messages.get().LogParserEditor_AddRule);
		addColumnLink.setImage(SharedIcons.IMG_ADD_OBJECT);
		addColumnLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addRule();
			}
		});
		
		form.reflow(true);
	}
	
	/**
	 * @param generalArea
	 */
	private void createGeneralArea(Composite generalArea)
   {
	   GridLayout layout = new GridLayout();
	   layout.makeColumnsEqualWidth = false;
      layout.numColumns = 3;
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      generalArea.setLayout(layout);
      
      final WidgetFactory spinnerFactory = new WidgetFactory() {
         @Override
         public Control createControl(Composite parent, int style)
         {
            return new Spinner(parent, style);
         }
      };
      
      labelName = new LabeledText(generalArea, SWT.NONE);
      labelName.setLabel("Parser name");
      labelName.setText((parser.getName() != null) ? parser.getName() : ""); //$NON-NLS-1$
      GridData gd = new GridData();
      gd.grabExcessHorizontalSpace = true;
      gd.horizontalAlignment = SWT.FILL;
      labelName.setLayoutData(gd);
      labelName.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            fireModifyListeners();
         }
      });
      
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      spinnerTrace = (Spinner)WidgetHelper.createLabeledControl(generalArea, SWT.BORDER, spinnerFactory, "Trace level", gd);
      spinnerTrace.setMinimum(0);
      spinnerTrace.setMaximum(9);
      spinnerTrace.addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            fireModifyListeners();
         }
      });
      spinnerTrace.setSelection(parser.getTrace() != null ? parser.getTrace() : 0);
      
      checkProcessAll = toolkit.createButton(generalArea, "Process all", SWT.CHECK);
      checkProcessAll.setSelection(parser.getProcessALL());
      checkProcessAll.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            fireModifyListeners();
         }
      });  
      
      if (!isSyslogParser)
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
         
         //add link to add file editor
         addFileLink = toolkit.createImageHyperlink(fileArea, SWT.NONE);
         addFileLink.setText("Add file");
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
		tabItem.setText(Messages.get().LogParserEditor_XML);
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
		
	   final String[] columnNames = { Messages.get().LogParserEditor_Name, Messages.get().LogParserEditor_Value };
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
         
         /* (non-Javadoc)
          * @see org.eclipse.jface.viewers.ViewerComparator#compare(org.eclipse.jface.viewers.Viewer, java.lang.Object, java.lang.Object)
          */
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
		
		ImageHyperlink link = toolkit.createImageHyperlink(macroArea, SWT.NONE);
		link.setImage(SharedIcons.IMG_ADD_OBJECT);
		link.setText(Messages.get().LogParserEditor_Add);
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

		link = toolkit.createImageHyperlink(macroArea, SWT.NONE);
		link.setImage(SharedIcons.IMG_EDIT);
		link.setText(Messages.get().LogParserEditor_Edit);
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

		link = toolkit.createImageHyperlink(macroArea, SWT.NONE);
		link.setImage(SharedIcons.IMG_DELETE_OBJECT);
		link.setText(Messages.get().LogParserEditor_Delete);
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
	   if (!isSyslogParser)
	   {
	      for(LogParserFile file : parser.getFiles())
	         file.getEditor().save();
	   }
      parser.setName(labelName.getText());
	   parser.setProcessALL(checkProcessAll.getSelection());
	   parser.setTrace(spinnerTrace.getSelection());
	   
		for(LogParserRule rule : parser.getRules())
			rule.getEditor().save();
		
		try
		{
			return parser.createXml();
		}
		catch(Exception e)
		{
			e.printStackTrace();
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
      xmlEditor.setText(xml);
		updateBuilderFromXml(xml);
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
			e.printStackTrace();
			MessageDialogHelper.openError(getShell(), Messages.get().LogParserEditor_Error, Messages.get().LogParserEditor_InvalidDefinition);
			parser = null;
			selectXmlEditor();
			return;
		}
		parser.setSyslogParser(isSyslogParser);
		
		/* general */
		if (!isSyslogParser)
      {
	      for(LogParserFile file : parser.getFiles())
	         createFileEditor(file).moveAbove(addFileLink);
      }
		labelName.setText(parser.getName());
		spinnerTrace.setSelection(parser.getTrace() != null ? parser.getTrace() : 0);
		checkProcessAll.setSelection(parser.getProcessALL());
		
		/* rules */
		for(LogParserRule rule : parser.getRules())
			createRuleEditor(rule).moveAbove(addColumnLink);
		
		/* macros */
		macroList.setInput(parser.getMacros().entrySet().toArray());
		
		form.reflow(true);
		form.getParent().layout(true, true);
	}

	/**
	 * @param rule
	 */
	private LogParserRuleEditor createRuleEditor(LogParserRule rule)
	{
		LogParserRuleEditor editor = new LogParserRuleEditor(rulesArea, toolkit, rule, this);
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
		editor.moveAbove(addColumnLink);
		parser.getRules().add(rule);
		form.reflow(true);
		fireModifyListeners();
	}

   /**
    * @param file
    */
   private LogParserFileEditor createFileEditor(LogParserFile file)
   {
      LogParserFileEditor editor = new LogParserFileEditor(fileArea, toolkit, file, this);
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
      form.reflow(true);
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
		IStructuredSelection selection = (IStructuredSelection)macroList.getSelection();
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
		IStructuredSelection selection = (IStructuredSelection)macroList.getSelection();
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
		form.reflow(true);
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
      form.reflow(true);
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
		form.reflow(true);
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
		form.reflow(true);
		getParent().layout(true, true);
		fireModifyListeners();
	}

   /**
    * @return the isSyslogParser
    */
   public boolean isSyslogParser()
   {
      return isSyslogParser;
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
         if(file.getEditor().getFile().startsWith("*"))
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
         if(rule.getEditor() != null)
            rule.getEditor().updateWindowsEventLogFields();
      }
   }
}
