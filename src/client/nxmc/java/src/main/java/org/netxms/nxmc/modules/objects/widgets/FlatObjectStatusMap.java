/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Raden Solutions
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
package org.netxms.nxmc.modules.objects.widgets;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.StructuredSelection;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.layout.RowLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.eclipse.swt.widgets.Menu;
import org.netxms.client.objects.AbstractObject;
import org.netxms.client.objects.Chassis;
import org.netxms.client.objects.Cluster;
import org.netxms.client.objects.Collector;
import org.netxms.client.objects.Container;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.ServiceRoot;
import org.netxms.nxmc.modules.objects.views.ObjectView;

/**
 * Widget showing "heat" map of nodes under given root object
 */
public class FlatObjectStatusMap extends AbstractObjectStatusMap
{
	private List<Composite> sections = new ArrayList<Composite>();
	private Map<Long, ObjectStatusWidget> statusWidgets = new HashMap<Long, ObjectStatusWidget>();
	private Font titleFont;
	private boolean groupObjects = true;
   protected Composite dataArea;

   /**
    * Create flat status map.
    *
    * @param view owning view
    * @param parent parent composite
    * @param style widget's style
    */
   public FlatObjectStatusMap(ObjectView view, Composite parent, int style)
	{
      super(view, parent, style);
      titleFont = JFaceResources.getFontRegistry().getBold(JFaceResources.BANNER_FONT);
	}

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractObjectStatusMap#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Composite createContent(Composite parent)
   {
      dataArea = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.verticalSpacing = 10;
      dataArea.setLayout(layout);
      dataArea.setBackground(getBackground());
      return dataArea;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractObjectStatusMap#refresh()
    */
	@Override
	public void refresh()
	{
		for(Composite s : sections)
			s.dispose();
		sections.clear();

		synchronized(statusWidgets)
		{
			statusWidgets.clear();
		}

		if (groupObjects)
         buildSection(rootObjectId, "");
		else
			buildFlatView();
		dataArea.layout(true, true);
		updateScrollBars();

		for(Runnable l : refreshListeners)
		   l.run();
	}

	/**
	 * Build flat view - all nodes in one group
	 */
	private void buildFlatView()
	{
      AbstractObject root = session.findObjectById(rootObjectId);
      if ((root == null) || !((root instanceof Collector) || (root instanceof Container) || (root instanceof ServiceRoot) || (root instanceof Cluster) || (root instanceof Rack) || (root instanceof Chassis)))
			return;

      List<AbstractObject> objects = new ArrayList<AbstractObject>(
            root.getAllChildren(new int[] { AbstractObject.OBJECT_NODE, AbstractObject.OBJECT_CLUSTER, AbstractObject.OBJECT_MOBILEDEVICE, AbstractObject.OBJECT_ACCESSPOINT }));
		filterObjects(objects);
      Collections.sort(objects, (o1, o2) -> o1.getNameWithAlias().compareToIgnoreCase(o2.getNameWithAlias()));

		final Composite clientArea = new Composite(dataArea, SWT.NONE);
		clientArea.setBackground(getBackground());
		GridData gd = new GridData();
		gd.grabExcessHorizontalSpace = true;
		gd.horizontalAlignment = SWT.FILL;
		clientArea.setLayoutData(gd);
		RowLayout clayout = new RowLayout();
		clayout.marginBottom = 0;
		clayout.marginTop = 0;
		clayout.marginLeft = 0;
		clayout.marginRight = 0;
		clayout.type = SWT.HORIZONTAL;
		clayout.wrap = true;
		clayout.pack = false;
		clientArea.setLayout(clayout);
		sections.add(clientArea);

		for(AbstractObject o : objects)
		{
			if (!isLeafObject(o))
				continue;

			addObjectElement(clientArea, o);
		}
	}

	/**
	 * Build section of the form corresponding to one container
	 */
	private void buildSection(long rootId, String namePrefix)
	{
		AbstractObject root = session.findObjectById(rootId);
      if ((root == null) || !((root instanceof Collector) || (root instanceof Container) || (root instanceof ServiceRoot) || (root instanceof Cluster) || (root instanceof Rack) || (root instanceof Chassis)))
			return;

		List<AbstractObject> objects = new ArrayList<AbstractObject>(Arrays.asList(root.getChildrenAsArray()));
      Collections.sort(objects, (o1, o2) -> o1.getNameWithAlias().compareToIgnoreCase(o2.getNameWithAlias()));

		Composite section = null;
		Composite clientArea = null;

		// Add nodes and clusters
		for(AbstractObject o : objects)
		{
         if (!isLeafObject(o))
				continue;

			if (!isAcceptedByFilter(o))
				continue;

			if (section == null)
			{
				section = new Composite(dataArea, SWT.NONE);
				section.setBackground(getBackground());
				GridData gd = new GridData();
				gd.grabExcessHorizontalSpace = true;
				gd.horizontalAlignment = SWT.FILL;
				section.setLayoutData(gd);

				GridLayout layout = new GridLayout();
				layout.marginHeight = 0;
				layout.marginWidth = 0;
				section.setLayout(layout);

				final Label title = new Label(section, SWT.NONE);
				title.setBackground(getBackground());
				title.setFont(titleFont);
            title.setText(namePrefix + root.getNameWithAlias());

				clientArea = new Composite(section, SWT.NONE);
				clientArea.setBackground(getBackground());
				gd = new GridData();
				gd.grabExcessHorizontalSpace = true;
				gd.horizontalAlignment = SWT.FILL;
				clientArea.setLayoutData(gd);
				RowLayout clayout = new RowLayout();
				clayout.marginBottom = 0;
				clayout.marginTop = 0;
				clayout.marginLeft = 0;
				clayout.marginRight = 0;
				clayout.type = SWT.HORIZONTAL;
				clayout.wrap = true;
				clayout.pack = false;
				clientArea.setLayout(clayout);

				sections.add(section);
			}

			addObjectElement(clientArea, o);
		}

		// Add subcontainers
		for(AbstractObject o : objects)
		{
         if (isContainerObject(o))
            buildSection(o.getObjectId(), namePrefix + root.getNameWithAlias() + " / ");
		}
	}

	/**
	 * @param object
	 */
	private void addObjectElement(final Composite parent, final AbstractObject object)
	{
		ObjectStatusWidget w = new ObjectStatusWidget(parent, object);
		w.setBackground(getBackground());
		w.addMouseListener(new MouseListener() {
			@Override
			public void mouseUp(MouseEvent e)
			{
			}

			@Override
			public void mouseDown(MouseEvent e)
			{
				setSelection(new StructuredSelection(object));
				if (e.button == 1)
               showObjectDetails(object);
			}

			@Override
			public void mouseDoubleClick(MouseEvent e)
			{
			}
		});

		// Create popup menu
		Menu menu = menuManager.createContextMenu(w);
		w.setMenu(menu);

		synchronized(statusWidgets)
		{
			statusWidgets.put(object.getObjectId(), w);
		}
	}

	/**
	 * @return the groupObjects
	 */
	public boolean isGroupObjects()
	{
		return groupObjects;
	}

	/**
	 * @param groupObjects the groupObjects to set
	 */
	public void setGroupObjects(boolean groupObjects)
	{
		this.groupObjects = groupObjects;
	}

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractObjectStatusMap#onObjectChange(org.netxms.client.objects.AbstractObject)
    */
	@Override
	protected void onObjectChange(final AbstractObject object)
	{
		if (!isContainerObject(object) && !isLeafObject(object))
			return;

		synchronized(statusWidgets)
		{
			final ObjectStatusWidget w = statusWidgets.get(object.getObjectId());
			if (w != null)
			{
            getDisplay().asyncExec(() -> {
               if (w.isDisposed())
                  return;

               if (isAcceptedByFilter(object))
               {
                  w.updateObject(object);
               }
               else
               {
                  // object no longer pass filters
                  synchronized(statusWidgets)
                  {
                     w.dispose();
                     statusWidgets.remove(object.getObjectId());
                     dataArea.layout(true, true);
                     updateScrollBars();
                  }
               }
				});
			}
         else if ((object.getObjectId() == rootObjectId) || (object.isChildOf(rootObjectId) && (isContainerObject(object) || isAcceptedByFilter(object))))
			{
				refreshTimer.execute();
			}
		}
	}

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractObjectStatusMap#onObjectDelete(long)
    */
	@Override
	protected void onObjectDelete(final long objectId)
	{
		synchronized(statusWidgets)
		{
         final ObjectStatusWidget w = statusWidgets.get(objectId);
         if (w != null)
         {
            getDisplay().asyncExec(() -> {
               if (w.isDisposed())
                  return;

               synchronized(statusWidgets)
               {
                  w.dispose();
                  statusWidgets.remove(objectId);
                  dataArea.layout(true, true);
                  updateScrollBars();
               }
            });
         }
		}
	}

   /**
    * @see org.netxms.nxmc.modules.objects.widgets.AbstractObjectStatusMap#computeSize()
    */
   @Override
   protected Point computeSize()
   {
      Rectangle r = getClientArea();
      return dataArea.computeSize(r.width, SWT.DEFAULT);
   }
}
