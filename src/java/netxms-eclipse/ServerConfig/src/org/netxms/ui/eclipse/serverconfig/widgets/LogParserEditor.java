/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.widgets;

import java.util.HashSet;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.jface.viewers.ArrayContentProvider;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.viewers.TableViewer;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Table;
import org.eclipse.swt.widgets.TableColumn;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.netxms.ui.eclipse.serverconfig.dialogs.LogMacroEditDialog;
import org.netxms.ui.eclipse.serverconfig.widgets.helpers.LogParser;
import org.netxms.ui.eclipse.serverconfig.widgets.helpers.LogParserRule;
import org.netxms.ui.eclipse.serverconfig.widgets.helpers.LogParserRuleEditor;
import org.netxms.ui.eclipse.shared.SharedIcons;

/**
 * Log parser editor
 */
public class LogParserEditor extends Composite
{
	private FormToolkit toolkit;
	private ScrolledForm form;
	private Set<ModifyListener> listeners = new HashSet<ModifyListener>();
	private LogParser parser = new LogParser();
	private Composite rulesArea;
	private TableViewer macroList;

	/**
	 * @param parent
	 * @param style
	 */
	public LogParserEditor(Composite parent, int style)
	{
		super(parent, style);
		
		setLayout(new FillLayout());

		/* FORM */
		toolkit = new FormToolkit(getDisplay());
		form = toolkit.createScrolledForm(this);
		form.setText("Log Parser");

		TableWrapLayout layout = new TableWrapLayout();
		layout.numColumns = 2;
		form.getBody().setLayout(layout);

		/* Rules section */
		Section section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
		section.setText("Rules");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		rulesArea = toolkit.createComposite(section);
		GridLayout rulesAreaLayout = new GridLayout();
		rulesAreaLayout.marginHeight = 0;
		rulesAreaLayout.marginWidth = 0;
		rulesArea.setLayout(rulesAreaLayout);
		
		section.setClient(rulesArea);
		
		final ImageHyperlink addColumnLink = toolkit.createImageHyperlink(rulesArea, SWT.NONE);
		addColumnLink.setText("Add rule");
		addColumnLink.setImage(SharedIcons.IMG_ADD_OBJECT);
		addColumnLink.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				addRule(addColumnLink);
			}
		});
		
		/* Macros section */
		section = toolkit.createSection(form.getBody(), Section.TITLE_BAR);
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
	public void addModifyListener(ModifyListener modifyListener)
	{
		listeners.add(modifyListener);
	}

	/**
	 * @param modifyListener
	 */
	public void removeModifyListener(ModifyListener modifyListener)
	{
		listeners.remove(modifyListener);
	}

	/**
	 * Get parser XML
	 * 
	 * @return
	 */
	public String getParserXml()
	{
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
			MessageDialog.openError(getShell(), "Error", "Log parser definition is invalid");
			parser = new LogParser();
		}
		
		/* rules */
		for(LogParserRule rule : parser.getRules())
			createRuleEditor(rule);
		
		/* macros */
		macroList.setInput(parser.getMacros().entrySet().toArray());
		
		getParent().layout(true, true);
	}

	/**
	 * @param rule
	 */
	private LogParserRuleEditor createRuleEditor(LogParserRule rule)
	{
		LogParserRuleEditor editor = new LogParserRuleEditor(rulesArea, toolkit);
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
	private void addRule(Control lastControl)
	{
		LogParserRule rule = new LogParserRule();
		LogParserRuleEditor editor = createRuleEditor(rule);
		editor.moveAbove(lastControl);
		parser.getRules().add(rule);
		form.reflow(true);
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
	}
}
