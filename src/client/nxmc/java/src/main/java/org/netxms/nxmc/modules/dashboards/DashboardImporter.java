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
package org.netxms.nxmc.modules.dashboards;

import java.io.StringWriter;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
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
import org.eclipse.jface.window.Window;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCObjectCreationData;
import org.netxms.client.NXCObjectModificationData;
import org.netxms.client.NXCSession;
import org.netxms.client.dashboards.DashboardElement;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfig;
import org.netxms.nxmc.modules.dashboards.config.DashboardElementConfigFactory;
import org.netxms.nxmc.modules.dashboards.dialogs.IdMatchingDialog;
import org.netxms.nxmc.modules.dashboards.dialogs.ImportDashboardDialog;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.DciIdMatchingData;
import org.netxms.nxmc.modules.dashboards.dialogs.helpers.ObjectIdMatchingData;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xnap.commons.i18n.I18n;

/**
 * Dashboard importer
 */
public class DashboardImporter
{
   private final I18n i18n = LocalizationHelper.getI18n(DashboardImporter.class);

   private View view;
   private long parentId;
	private String objectName;

   /**
    * Create new dashboard importer.
    *
    * @param view owning view
    * @param parentId parent object ID
    * @param objectName name of new dashboard object
    */
   public DashboardImporter(View view, long parentId, String objectName)
   {
      this.view = view;
      this.parentId = parentId;
      this.objectName = objectName;
   }

   /**
    * Run dashboard import
    */
   public void run()
	{
      final ImportDashboardDialog dlg = new ImportDashboardDialog(view.getWindow().getShell());
		if (dlg.open() != Window.OK)
			return;

      final NXCSession session = Registry.getSession();
		final Display display = Display.getCurrent();

      new Job(i18n.tr("Import dashboard"), view) {
			@Override
         protected void run(IProgressMonitor monitor) throws Exception
			{
				DocumentBuilderFactory dbf = DocumentBuilderFactory.newInstance();
            dbf.setFeature("http://apache.org/xml/features/disallow-doctype-decl", true);
				DocumentBuilder db = dbf.newDocumentBuilder();
				Document dom = db.parse(dlg.getImportFile());

				Element root = dom.getDocumentElement();
            if (!root.getNodeName().equals("dashboard"))
               throw new Exception(i18n.tr("Invalid format of dashboard definition file"));

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
                  DashboardElement de = new DashboardElement(getNodeValueAsInt(e, "type", 0), getNodeValueAsXml(e, "element"), j);
                  de.setLayout(getNodeValueAsXml(e, "layout"));
						dashboardElements.add(de);
					}
				}

				root.normalize();

				objectName = dlg.getObjectName();

				if (doIdMapping(display, session, dashboardElements, root))
				{
					NXCObjectCreationData cd = new NXCObjectCreationData(AbstractObject.OBJECT_DASHBOARD, objectName, parentId);
					final long objectId = session.createObject(cd);

					NXCObjectModificationData md = new NXCObjectModificationData(objectId);
               md.setColumnCount(getNodeValueAsInt(root, "columns", 1));
               md.setObjectFlags(getNodeValueAsInt(root, "options", 0));
					md.setDashboardElements(dashboardElements);
					session.modifyObject(md);
				}
			}

			@Override
			protected String getErrorMessage()
			{
            return String.format(i18n.tr("Cannot import dashboard %s"), dlg.getObjectName());
			}
		}.start();
	}

	/**
	 * Map node and DCI ID from source system to destination system
	 * @throws Exception 
	 * @return true if import operation should continue
	 */
	private boolean doIdMapping(final Display display, final NXCSession session, List<DashboardElement> dashboardElements, Element root) throws Exception
	{
		final Map<Long, ObjectIdMatchingData> objects = readSourceObjects(root);
		final Map<Long, DciIdMatchingData> dcis = readSourceDci(root);

		// add all node IDs from DCI list if they are missing
		for(DciIdMatchingData d : dcis.values())
		{
         if ((d.srcNodeId != AbstractObject.CONTEXT) && !objects.containsKey(d.srcNodeId))
            objects.put(d.srcNodeId, new ObjectIdMatchingData(d.srcNodeId, "", AbstractObject.OBJECT_NODE));
		}

		// try to match objects
		for(ObjectIdMatchingData d : objects.values())
		{
         if ((d.srcId < 10) || (d.srcId == AbstractObject.CONTEXT))
			{
				// built-in object 
				d.dstId = d.srcId;
				continue;
			}

			if (d.srcName.isEmpty())
				continue;

			AbstractObject object = session.findObjectByName(d.srcName);
			if ((object != null) && isCompatibleClasses(object.getObjectClass(), d.objectClass))
			{
				d.dstId = object.getObjectId();
				d.dstName = object.getObjectName();
			}
		}
		
		// try to match DCIs
		for(DciIdMatchingData d : dcis.values())
		{
			// get node ID on target system
			ObjectIdMatchingData od = objects.get(d.srcNodeId);

			// bind DCI data to appropriate node data
			od.dcis.add(d);

         if (od.dstId == AbstractObject.UNKNOWN)
				continue;	// no match for node

			d.dstNodeId = od.dstId;
			DciValue[] dciValues = session.getLastValues(d.dstNodeId);
			for(DciValue v : dciValues)
			{
				if (v.getDescription().equalsIgnoreCase(d.srcName))
				{
					d.dstDciId = v.getId();
					d.dstName = v.getDescription();
					break;
				}
			}
		}

		// show matching results to user
      boolean[] success = new boolean[1];
      display.syncExec(() -> {
         IdMatchingDialog dlg = new IdMatchingDialog(view.getWindow().getShell(), objects, dcis);
         success[0] = (dlg.open() == Window.OK);
      });
      if (success[0])
		{
			// update dashboard elements with mapping data
			for(DashboardElement e : dashboardElements)
				updateDashboardElement(e, objects, dcis);
			return true;
		}
		return false;
	}

	/**
	 * Check if two classes are compatible for object matching
	 * 
	 * @param c1
	 * @param c2
	 * @return
	 */
	public static boolean isCompatibleClasses(int c1, int c2)
	{
	   return (c1 == c2) || 
	          ((c1 == AbstractObject.OBJECT_NODE) && (c2 == AbstractObject.OBJECT_CLUSTER)) ||
	          ((c1 == AbstractObject.OBJECT_CLUSTER) && (c2 == AbstractObject.OBJECT_NODE));
	}
	
	/**
	 * Update dashboard element from mapping data
	 * 
	 * @param e
	 * @param objects
	 * @param dcis
	 * @throws Exception 
	 */
	private void updateDashboardElement(DashboardElement e, Map<Long, ObjectIdMatchingData> objects, Map<Long, DciIdMatchingData> dcis) throws Exception
	{
      DashboardElementConfig config = DashboardElementConfigFactory.create(e);
		if (config == null)
			return;

		config.remapObjects(objects);
		config.remapDataCollectionItems(dcis);
		e.setData(config.createJson());
	}

	/**
	 * Read source objects from XML document
	 * 
	 * @param root
	 * @return
	 */
	private Map<Long, ObjectIdMatchingData> readSourceObjects(Element root)
	{
		Map<Long, ObjectIdMatchingData> objects = new HashMap<Long, ObjectIdMatchingData>();
		NodeList objectsRoot = root.getElementsByTagName("objectMap"); //$NON-NLS-1$
		for(int i = 0; i < objectsRoot.getLength(); i++)
		{
			if (objectsRoot.item(i).getNodeType() != Node.ELEMENT_NODE)
				continue;

         NodeList elements = ((Element)objectsRoot.item(i)).getElementsByTagName("object");
			for(int j = 0; j < elements.getLength(); j++)
			{
				Element e = (Element)elements.item(j);
				long id = getAttributeAsLong(e, "id", 0); //$NON-NLS-1$
            objects.put(id, new ObjectIdMatchingData(id, e.getTextContent(), (int)getAttributeAsLong(e, "class", 0)));
			}
		}
		return objects;
	}

	/**
	 * Read source DCI from XML document
	 * 
	 * @param root
	 * @return
	 */
	private Map<Long, DciIdMatchingData> readSourceDci(Element root)
	{
		Map<Long, DciIdMatchingData> dcis = new HashMap<Long, DciIdMatchingData>();
		NodeList objectsRoot = root.getElementsByTagName("dciMap"); //$NON-NLS-1$
		for(int i = 0; i < objectsRoot.getLength(); i++)
		{
			if (objectsRoot.item(i).getNodeType() != Node.ELEMENT_NODE)
				continue;
			
         NodeList elements = ((Element)objectsRoot.item(i)).getElementsByTagName("dci");
			for(int j = 0; j < elements.getLength(); j++)
			{
				Element e = (Element)elements.item(j);
            long id = getAttributeAsLong(e, "id", 0);
            dcis.put(id, new DciIdMatchingData(getAttributeAsLong(e, "node", 0), id, e.getTextContent()));
			}
		}
		return dcis;
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
	 * Get value of given attribute as integer.
	 * 
	 * @param parent
	 * @param tag
	 * @param defaultValue
	 * @return
	 */
	private static long getAttributeAsLong(Element element, String attribute, long defaultValue)
	{
		try
		{
			return Long.parseLong(element.getAttribute(attribute));
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
			return "<" + tag + "></" + tag + ">"; //$NON-NLS-1$ //$NON-NLS-2$ //$NON-NLS-3$
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
		t.setOutputProperty(OutputKeys.OMIT_XML_DECLARATION, "yes"); //$NON-NLS-1$
		t.setOutputProperty(OutputKeys.INDENT, "yes"); //$NON-NLS-1$
		t.transform(new DOMSource(node), new StreamResult(sw));
		return sw.toString();
	}
}
