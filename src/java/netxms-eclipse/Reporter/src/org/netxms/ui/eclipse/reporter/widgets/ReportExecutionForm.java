/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2011 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.reporter.widgets;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;

import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.dialogs.MessageDialog;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Text;
import org.eclipse.ui.IWorkbenchPage;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.forms.events.HyperlinkAdapter;
import org.eclipse.ui.forms.events.HyperlinkEvent;
import org.eclipse.ui.forms.widgets.FormToolkit;
import org.eclipse.ui.forms.widgets.ImageHyperlink;
import org.eclipse.ui.forms.widgets.ScrolledForm;
import org.eclipse.ui.forms.widgets.Section;
import org.eclipse.ui.forms.widgets.TableWrapData;
import org.eclipse.ui.forms.widgets.TableWrapLayout;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.Report;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.reporter.Activator;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ReportDefinition;
import org.netxms.ui.eclipse.reporter.widgets.helpers.ReportParameter;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;

/**
 * Report execution widget. Provides form with report-specific parameters.
 */
public class ReportExecutionForm extends Composite
{
	private IWorkbenchPart workbenchPart = null;
	private Report report;
	private FormToolkit toolkit;
	private ScrolledForm form;
	private List<Control> fields = new ArrayList<Control>();
	private List<ReportParameter> parameters;
	
	/**
	 * @param parent
	 * @param style
	 */
	public ReportExecutionForm(Composite parent, int style, Report report)
	{
		super(parent, style);
		this.report = report;
		
		setLayout(new FillLayout());
		
		toolkit = new FormToolkit(getDisplay());
		form = toolkit.createScrolledForm(this);
		form.setText(report.getObjectName());
		
		TableWrapLayout layout = new TableWrapLayout();
		form.getBody().setLayout(layout);
		
		if (!report.getComments().isEmpty())
			toolkit.createLabel(form.getBody(), report.getComments(), SWT.WRAP);
		
		Section section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Parameters");
		section.setDescription("Please provide parameters necessary to run this report in fields below");
		TableWrapData td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		final Composite paramArea = toolkit.createComposite(section);
		layout = new TableWrapLayout();
		layout.numColumns = 2;
		paramArea.setLayout(layout);
		section.setClient(paramArea);
		createParamEntryFields(paramArea);
		
		section = toolkit.createSection(form.getBody(), Section.DESCRIPTION | Section.TITLE_BAR);
		section.setText("Actions");
		td = new TableWrapData();
		td.align = TableWrapData.FILL;
		td.grabHorizontal = true;
		section.setLayoutData(td);
		
		final Composite actionArea = toolkit.createComposite(section);
		layout = new TableWrapLayout();
		actionArea.setLayout(layout);
		section.setClient(actionArea);
		
		ImageHyperlink link = toolkit.createImageHyperlink(actionArea, SWT.WRAP);
		link.setImage(Activator.getImageDescriptor("icons/execute.png").createImage());
		link.setText("Execute report");
		link.addHyperlinkListener(new HyperlinkAdapter() {
			@Override
			public void linkActivated(HyperlinkEvent e)
			{
				executeReport();
			}
		});
	}

	/**
	 * Create entry fields for parameters
	 * 
	 * @param parent
	 */
	private void createParamEntryFields(Composite parent)
	{
		try
		{
			final ReportDefinition definition = ReportDefinition.createFromXml(report.getDefinition());
			parameters = definition.getParameters();
			for(ReportParameter p : parameters)
			{
				toolkit.createLabel(parent, p.getDisplayName());
				Control control;
				switch(p.getDataType())
				{
					case ReportParameter.INTEGER:
						control = toolkit.createText(parent, p.getDefaultValue());
						break;
					default:		// everything else as string
						control = toolkit.createText(parent, p.getDefaultValue());
						break;
				}
				
				TableWrapData td = new TableWrapData();
				td.align = TableWrapData.FILL;
				td.grabHorizontal = true;
				control.setLayoutData(td);
				fields.add(control);
			}
		}
		catch(Exception e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
	}
	
	/**
	 * Execute report
	 */
	private void executeReport()
	{
		final Map<String, String> execParameters = new HashMap<String, String>(parameters.size());
		for(int i = 0; i < parameters.size(); i++)
		{
			execParameters.put(parameters.get(i).getName(), ((Text)fields.get(i)).getText());
		}
		
		new ConsoleJob("Execute report", workbenchPart, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				NXCSession session = (NXCSession)ConsoleSharedData.getSession();
				final long jobId = session.executeReport(report.getObjectId(), execParameters);
				getDisplay().asyncExec(new Runnable() {
					@Override
					public void run()
					{
						MessageDialog.openInformation(getShell(), "Report Execution", 
								"Report " + report.getObjectName() + " execution started successfully. Job ID is " + jobId + ".");
					}
				});
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot execute report " + report.getObjectName();
			}
		}.start();
	}

	/**
	 * @param workbenchPart the workbenchPart to set
	 */
	public void setWorkbenchPart(IWorkbenchPart workbenchPart)
	{
		this.workbenchPart = workbenchPart;
	}
}
