/**
 * 
 */
package org.netxms.ui.eclipse.serverconfig.widgets;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;

/**
 * Log parser editor
 */
public class LogParserEditor extends Composite
{
	private FormToolkit toolkit;
	private ScrolledForm form;
	private Set<ModifyListener> listeners = new HashSet<ModifyListener>();

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
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Rules");
		section.setDescription("Provide parameters necessary to run this report in fields below");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		final Composite rulesArea = toolkit.createComposite(section);
		layout = new TableWrapLayout();
		rulesArea.setLayout(layout);
		section.setClient(rulesArea);
		createRulesSection(rulesArea);
		
		/* Macros section */
		section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Macros");
		td = new TableWrapData();
		td.align = TableWrapData.FILL;
		section.setLayoutData(td);

		final Composite macroArea = toolkit.createComposite(section);
		layout = new TableWrapLayout();
		macroArea.setLayout(layout);
		section.setClient(macroArea);
		createMacroSection(macroArea);
	}

	/**
	 * @param macroArea
	 */
	private void createMacroSection(Composite macroArea)
	{
		// TODO Auto-generated method stub
		
	}

	/**
	 * @param rulesArea
	 */
	private void createRulesSection(Composite rulesArea)
	{
		// TODO Auto-generated method stub
		
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
		return null;
	}

	/**
	 * Set parser XML
	 * 
	 * @param content
	 */
	public void setParserXml(String content)
	{
	}
}
