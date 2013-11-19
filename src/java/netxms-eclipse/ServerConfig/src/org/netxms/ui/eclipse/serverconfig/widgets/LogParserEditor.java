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
package org.netxms.ui.eclipse.serverconfig.widgets;

import java.util.Collections;
import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.CTabFolder;
import org.eclipse.swt.custom.CTabItem;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.events.SelectionListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
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
import org.netxms.ui.eclipse.serverconfig.dialogs.LogMacroEditDialog;
import org.netxms.ui.eclipse.serverconfig.widgets.helpers.LogParser;
import org.netxms.ui.eclipse.serverconfig.widgets.helpers.LogParserModifyListener;
import org.netxms.ui.eclipse.serverconfig.widgets.helpers.LogParserRule;
import org.netxms.ui.eclipse.serverconfig.widgets.helpers.LogParserRuleEditor;
import org.netxms.ui.eclipse.tools.MessageDialogHelper;

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
	private ImageHyperlink addColumnLink;
	private TableViewer macroList;

	/**
	 * @param parent
	 * @param style
	 */
	public LogParserEditor(Composite parent, int style)
	{
		super(parent, style);
		
		setLayout(new FillLayout());
		
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
						xml = buildParserXml();
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
	 * Create policy edit form
	 */
	private void createForm()
	{
		/* FORM */
		toolkit = new FormToolkit(getDisplay());
		form = toolkit.createScrolledForm(tabFolder);
		form.setText("Log Parser");

		final CTabItem tabItem = new CTabItem(tabFolder, SWT.NONE);
		tabItem.setText("Editor");
		tabItem.setImage(SharedIcons.IMG_EDIT);
		tabItem.setControl(form);
		tabItem.setData(TAB_BUILDER);
		
		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 2;
		form.getBody().setLayout(layout);

		/* Rules section */
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR | Section.COMPACT | Section.TWISTIE | Section.EXPANDED);
		section.setText("Rules");
		TableWrapData td = new TableWrapData();
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
		addColumnLink.setText("Add rule");
		addColumnLink.setImage(SharedIcons.IMG_ADD_OBJECT);
		addColumnLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addRule();
			}
		});
		
		/* Macros section */
		section = toolkit.createSection(form.getBody(), Section.TITLE_BAR | Section.COMPACT | Section.TWISTIE);
		section.setText("Macros");
		td = new TableWrapData();
		td.align = TableWrapData.FILL;
		section.setLayoutData(td);

		final Composite macroArea = toolkit.createComposite(section);
		createMacroSection(macroArea);
		
		section.setClient(macroArea);
		
		form.reflow(true);
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
		
		macroList = new TableViewer(macroArea, SWT.BORDER);
		GridData gd = new GridData();
		gd.horizontalAlignment = SWT.FILL;
		gd.verticalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		gd.grabExcessVerticalSpace = true;
		gd.verticalSpan = 3;
		final Table table = macroList.getTable();
		table.setLayoutData(gd);
		
		TableColumn tc = new TableColumn(table, SWT.LEFT);
		tc.setText("Name");
		tc.setWidth(100);

		tc = new TableColumn(table, SWT.LEFT);
		tc.setText("Value");
		tc.setWidth(200);
		
		macroList.setContentProvider(new ArrayContentProvider());
		
		ImageHyperlink link = toolkit.createImageHyperlink(macroArea, SWT.NONE);
		link.setImage(SharedIcons.IMG_ADD_OBJECT);
		link.setText("Add...");
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
		link.setText("Edit...");
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
		link.setText("Delete");
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
				return "<parser></parser>";
		}
	}
	
	/**
	 * Build parser XML from current builder state
	 * 
	 * @return
	 */
	private String buildParserXml()
	{
		for(LogParserRule rule : parser.getRules())
			rule.getEditor().save();
		try
		{
			return parser.createXml();
		}
		catch(Exception e)
		{
			e.printStackTrace();
			return "<parser>\n</parser>";
		}
	}

	/**
	 * Set parser XML
	 * 
	 * @param content
	 */
	public void setParserXml(String xml)
	{
		updateBuilderFromXml(xml);
		xmlEditor.setText(xml);
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
			MessageDialogHelper.openError(getShell(), "Error", "Log parser definition is invalid");
			parser = new LogParser();
		}
		
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
}
