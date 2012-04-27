/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.dashboard.actions;

import java.io.StringWriter;
import java.util.ArrayList;
import java.util.List;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.transform.OutputKeys;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.TransformerFactoryConfigurationError;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.IAction;
import org.eclipse.jface.viewers.ISelection;
import org.eclipse.jface.viewers.IStructuredSelection;
import org.eclipse.jface.window.Window;
import org.eclipse.ui.IObjectActionDelegate;
import org.eclipse.ui.IWorkbenchPart;
import org.eclipse.ui.IWorkbenchWindow;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.objects.Dashboard;
import org.netxms.client.objects.DashboardRoot;
import org.netxms.client.objects.GenericObject;
import org.netxms.ui.eclipse.dashboard.Activator;
import org.netxms.ui.eclipse.dashboard.dialogs.ImportDashboardDialog;
import org.netxms.ui.eclipse.jobs.ConsoleJob;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

/**
 * Create dashboard object
 *
 */
public class ImportDashboard implements IObjectActionDelegate
{
	private IWorkbenchWindow window;
	private IWorkbenchPart part;
	private long parentId = -1;

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IObjectActionDelegate#setActivePart(org.eclipse.jface.action.IAction, org.eclipse.ui.IWorkbenchPart)
	 */
	@Override
	public void setActivePart(IAction action, IWorkbenchPart targetPart)
	{
		part = targetPart;
		window = targetPart.getSite().getWorkbenchWindow();
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#selectionChanged(org.eclipse.jface.action.IAction, org.eclipse.jface.viewers.ISelection)
	 */
	@Override
	public void selectionChanged(IAction action, ISelection selection)
	{
		if ((selection instanceof IStructuredSelection) && (((IStructuredSelection)selection).size() == 1))
		{
			final Object object = ((IStructuredSelection)selection).getFirstElement();
			if ((object instanceof Dashboard) || (object instanceof DashboardRoot))
			{
				parentId = ((GenericObject)object).getObjectId();
			}
			else
			{
				parentId = -1;
			}
		}
		else
		{
			parentId = -1;
		}

		action.setEnabled(parentId != -1);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.ui.IActionDelegate#run(org.eclipse.jface.action.IAction)
	 */
	@Override
	public void run(IAction action)
	{
		final ImportDashboardDialog dlg = new ImportDashboardDialog(window.getShell());
		if (dlg.open() != Window.OK)
			return;
		
		new ConsoleJob("Import dashboard", part, Activator.PLUGIN_ID, null) {
			@Override
			protected void runInternal(IProgressMonitor monitor) throws Exception
			{
				DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
				DocumentBuilder db = dbf.newDocumentBuilder();
				Document dom = db.parse(dlg.getImportFile());
				
				Element root = dom.getDocumentElement();
				if (!root.getNodeName().equals("dashboard"))
					throw new Exception("Invalid import file");
				
				root.normalize();
				
				List<DashboardElement> dashboardElements = new ArrayList<DashboardElement>(); 
				
				NodeList elementsRoot = root.getElementsByTagName("elements");
				for(int i = 0; i < elementsRoot.getLength(); i++)
				{
					if (elementsRoot.item(i).getNodeType() != Node.ELEMENT_NODE)
						continue;
					
					NodeList elements = ((Element)elementsRoot.item(i)).getElementsByTagName("dashboardElement");
					for(int j = 0; j < elements.getLength(); j++)
					{
						Element e = (Element)elements.item(j);
						DashboardElement de = new DashboardElement(getNodeValueAsInt(e, "type", 0), getNodeValueAsXml(e, "element"));
						de.setLayout(getNodeValueAsXml(e, "layout"));
						dashboardElements.add(de);
					}
				}
				
				NXCSession session = (NXCSession)ConsoleSharedData.getSession();
				NXCObjectCreationData cd = new NXCObjectCreationData(GenericObject.OBJECT_DASHBOARD, dlg.getObjectName(), parentId);
				final long objectId = session.createObject(cd);
				
				NXCObjectModificationData md = new NXCObjectModificationData(objectId);
				md.setColumnCount(getNodeValueAsInt(root, "columns", 1));
				md.setDashboardOptions(getNodeValueAsInt(root, "options", 0));
				md.setDashboardElements(dashboardElements);
				session.modifyObject(md);
			}

			@Override
			protected String getErrorMessage()
			{
				return "Cannot import dashboard object \"" + dlg.getObjectName() + "\"";
			}
		}.start();
	}
	
	/**
	 * Get value of given node as integer.
	 * 
	 * @param parent
	 * @param tag
	 * @param defaultValue
	 * @return
	 */
	private static int getNodeValueAsInt(Element parent, String tag, int defaultValue)
	{
		NodeList l = parent.getElementsByTagName(tag);
		if ((l.getLength() == 0) || (l.item(0).getNodeType() != Node.ELEMENT_NODE))
			return defaultValue;
		
		try
		{
			return Integer.parseInt(((Element)l.item(0)).getTextContent());
		}
		catch(NumberFormatException e)
		{
			return defaultValue;
		}
	}
	
	/**
	 * Get node value as XML document.
	 * 
	 * @param parent
	 * @param tag
	 * @return
	 * @throws TransformerFactoryConfigurationError
	 * @throws TransformerException
	 */
	private static String getNodeValueAsXml(Element parent, String tag) throws TransformerFactoryConfigurationError, TransformerException
	{
		NodeList l = parent.getElementsByTagName(tag);
		if ((l.getLength() == 0) || (l.item(0).getNodeType() != Node.ELEMENT_NODE))
			return "<" + tag + "></" + tag + ">";
		return nodeToString(l.item(0));
	}
	
	/**
	 * Convert DOM tree node to string (XML document).
	 * 
	 * @param node
	 * @return
	 * @throws TransformerFactoryConfigurationError
	 * @throws TransformerException
	 */
	private static String nodeToString(Node node) throws TransformerFactoryConfigurationError, TransformerException
	{
		StringWriter sw = new StringWriter();
		Transformer t = TransformerFactory.newInstance().newTransformer();
		t.setOutputProperty(OutputKeys.OMIT_XML_DECLARATION, "yes");
		t.setOutputProperty(OutputKeys.INDENT, "yes");
		t.transform(new DOMSource(node), new StreamResult(sw));
		return sw.toString();
	}
}
