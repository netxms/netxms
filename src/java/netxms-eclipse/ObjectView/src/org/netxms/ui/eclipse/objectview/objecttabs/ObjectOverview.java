/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2021 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.objectview.objecttabs;

import java.util.HashSet;
import java.util.Set;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Layout;
import org.eclipse.ui.IViewPart;
import org.netxms.client.objects.AbstractObject;
import org.netxms.ui.eclipse.console.resources.ThemeEngine;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Capabilities;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Commands;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Comments;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.Connection;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.ExternalResources;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.GeneralInfo;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.LastValues;
import org.netxms.ui.eclipse.objectview.objecttabs.elements.OverviewPageElement;
import org.netxms.ui.eclipse.objectview.views.TabbedObjectView;
import org.netxms.ui.eclipse.tools.ViewRefreshController;
import org.netxms.ui.eclipse.tools.VisibilityValidator;
import org.netxms.ui.eclipse.tools.WidgetHelper;

/**
 * Object overview tab
 */
public class ObjectOverview extends ObjectTab
{
	private Set<OverviewPageElement> elements = new HashSet<OverviewPageElement>();
	private ScrolledComposite scroller;
	private Composite viewArea;
	private Composite leftColumn;
	private Composite rightColumn;
   private ViewRefreshController refreshController;
   private boolean updateLayoutOnSelect = false;

	/**
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#createTabContent(org.eclipse.swt.widgets.Composite)
	 */
	@Override
	protected void createTabContent(Composite parent)
	{
		scroller = new ScrolledComposite(parent, SWT.V_SCROLL);
		scroller.setExpandVertical(true);
		scroller.setExpandHorizontal(true);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);
		scroller.addControlListener(new ControlAdapter() {
			public void controlResized(ControlEvent e)
			{
				Rectangle r = scroller.getClientArea();
				scroller.setMinSize(viewArea.computeSize(r.width, SWT.DEFAULT));
				objectChanged(getObject());
			}
		});

		viewArea = new Composite(scroller, SWT.NONE);
      viewArea.setBackground(ThemeEngine.getBackgroundColor("Dashboard"));
		GridLayout layout = new GridLayout();
		layout.numColumns = 2;
		viewArea.setLayout(layout);
		scroller.setContent(viewArea);

		leftColumn = new Composite(viewArea, SWT.NONE);
		leftColumn.setLayout(createColumnLayout());
		leftColumn.setBackground(viewArea.getBackground());
		GridData gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.FILL;
		gd.grabExcessHorizontalSpace = true;
		leftColumn.setLayoutData(gd);

		rightColumn = new Composite(viewArea, SWT.NONE);
		rightColumn.setLayout(createColumnLayout());
		rightColumn.setBackground(viewArea.getBackground());
		gd = new GridData();
		gd.verticalAlignment = SWT.TOP;
		gd.horizontalAlignment = SWT.LEFT;
		gd.grabExcessHorizontalSpace = true;
		gd.minimumWidth = SWT.DEFAULT;
		rightColumn.setLayoutData(gd);

		OverviewPageElement e = new GeneralInfo(leftColumn, null, this);
		elements.add(e);
      e = new LastValues(leftColumn, e, this);
      elements.add(e);
		e = new Commands(leftColumn, e, this);
		elements.add(e);
      e = new ExternalResources(leftColumn, e, this);
      elements.add(e);
		e = new Comments(leftColumn, e, this);
		elements.add(e);
		e = new Capabilities(rightColumn, null, this);
		elements.add(e);
		e = new Connection(rightColumn, e, this);
		elements.add(e);

      VisibilityValidator validator = new VisibilityValidator() {
         @Override
         public boolean isVisible()
         {
            return isActive();
         }
      };

      refreshController = new ViewRefreshController(getViewPart(), -1, new Runnable() {
         @Override
         public void run()
         {
            if (viewArea.isDisposed())
               return;

            IViewPart viewPart = getViewPart();
            if (viewPart instanceof TabbedObjectView)
               ((TabbedObjectView)getViewPart()).refreshCurrentTab();
         }
      }, validator);
      refreshController.setInterval(30);

      viewArea.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            refreshController.dispose();
         }
      });
	}

	/**
	 * Create layout for column
	 * 
	 * @return
	 */
	private Layout createColumnLayout()
	{
		GridLayout layout = new GridLayout();
		layout.marginHeight = 0;
		layout.marginWidth = 0;
		layout.verticalSpacing = 5;
		return layout;
	}

	/**
	 * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#currentObjectUpdated(org.netxms.client.objects.AbstractObject)
	 */
	@Override
	public void currentObjectUpdated(AbstractObject object)
	{
		changeObject(object);
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#objectChanged(org.netxms.client.objects.AbstractObject)
    */
	@Override
	public void objectChanged(AbstractObject object)
	{
	   if (isActive())
         updateLayout();
      else
         updateLayoutOnSelect = true;
	}

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#selected()
    */
   @Override
   public void selected()
   {
      super.selected();
      if (updateLayoutOnSelect)
      {
         updateLayout();
         updateLayoutOnSelect = false;
      }
   }

   /**
    * Update tab layout
    */
   private void updateLayout()
   {
      AbstractObject object = getObject();

      viewArea.setRedraw(false);
      for(OverviewPageElement element : elements)
      {
         if ((object != null) && element.isApplicableForObject(object))
         {
            element.setObject(object);
         }
         else
         {
            element.dispose();
         }
      }
      for(OverviewPageElement element : elements)
         element.fixPlacement();
      viewArea.layout(true, true);
      viewArea.setRedraw(true);

      Rectangle r = scroller.getClientArea();
      scroller.setMinSize(viewArea.computeSize(r.width, SWT.DEFAULT));

      Point s = viewArea.getSize();
      viewArea.redraw(0, 0, s.x, s.y, true);
   }

   /**
    * @see org.netxms.ui.eclipse.objectview.objecttabs.ObjectTab#refresh()
    */
	@Override
	public void refresh()
	{
		objectChanged(getObject());
	}
}
