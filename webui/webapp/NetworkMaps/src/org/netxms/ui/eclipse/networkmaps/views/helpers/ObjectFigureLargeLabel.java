/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2015 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.networkmaps.views.helpers;

import org.eclipse.draw2d.Graphics;
import org.eclipse.draw2d.GridData;
import org.eclipse.draw2d.GridLayout;
import org.eclipse.draw2d.Label;
import org.eclipse.draw2d.MarginBorder;
import org.eclipse.draw2d.PositionConstants;
import org.eclipse.draw2d.geometry.Dimension;
import org.eclipse.draw2d.geometry.Rectangle;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.netxms.client.MacAddress;
import org.netxms.client.datacollection.DciValue;
import org.netxms.client.maps.elements.NetworkMapObject;
import org.netxms.client.objects.Node;
import org.netxms.ui.eclipse.console.resources.StatusDisplayInfo;

/**
 * Figure which represents NetXMS object as small icon with label on the right
 */
public class ObjectFigureLargeLabel extends ObjectFigure
{
	private static final int BORDER_WIDTH = 2;
	
	private Label icon;
	private Label name;
	private Label additionalInfo;
	private NodeLastValuesFigure lastValuesFigure;
	
	/**
	 * @param element
	 * @param labelProvider
	 */
	public ObjectFigureLargeLabel(NetworkMapObject element, MapLabelProvider labelProvider)
	{
		super(element, labelProvider);

		setOpaque(true);
		
      setBorder(new MarginBorder(3));
      GridLayout layout = new GridLayout(2, false);
      layout.horizontalSpacing = 10;
      layout.numColumns = 2;
      setLayoutManager(layout);
      
      icon = new Label(labelProvider.getImage(element));
      icon.setFont(JFaceResources.getDefaultFont());
		add(icon);
      GridData gd = new GridData();
      gd.horizontalAlignment = SWT.LEFT;
      gd.verticalAlignment = SWT.TOP;
      gd.verticalSpan = 3;
      setConstraint(icon, gd);
      
		name = new Label(object.getObjectName());
		name.setFont(JFaceResources.getDefaultFont());
		name.setLabelAlignment(PositionConstants.LEFT);
		add(name);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      gd.grabExcessHorizontalSpace = true;
      setConstraint(name, gd);

      additionalInfo = new Label(object.getObjectName());
      additionalInfo.setLabelAlignment(PositionConstants.LEFT);
      additionalInfo.setFont(labelProvider.getLabelFont());
      add(additionalInfo);
      gd = new GridData();
      gd.horizontalAlignment = SWT.FILL;
      gd.verticalAlignment = SWT.TOP;
      gd.grabExcessHorizontalSpace = true;
      setConstraint(additionalInfo, gd);

      StringBuilder sb = new StringBuilder();
      if ((object instanceof Node) && !((Node)object).getPrimaryIP().isAnyLocalAddress())
      {
         sb.append(((Node)object).getPrimaryIP().getHostAddress());
         MacAddress mac = ((Node)object).getPrimaryMAC();
         if (mac != null)
         {
            sb.append(" ("); //$NON-NLS-1$
            sb.append(mac.toString());
            sb.append(')');
         }
      }
      additionalInfo.setText(sb.toString());
      
      if (object instanceof Node)
      {
         DciValue[] values = labelProvider.getNodeLastValues(object.getObjectId());
         if ((values != null) && (values.length > 0))
         {
            lastValuesFigure = new NodeLastValuesFigure(values);
            gd = new GridData();
            gd.horizontalAlignment = SWT.FILL;
            gd.verticalAlignment = SWT.TOP;
            gd.grabExcessHorizontalSpace = true;
            add(lastValuesFigure, gd);
         }
      }
      
      Dimension ls = getPreferredSize(-1, -1);
      setSize(ls.width, ls.height);
	}

	/* (non-Javadoc)
	 * @see org.eclipse.draw2d.Figure#paintFigure(org.eclipse.draw2d.Graphics)
	 */
	@Override
	protected void paintFigure(Graphics gc)
	{
		gc.setAntialias(SWT.ON);
		Rectangle rect = new Rectangle(getBounds());
		rect.x += BORDER_WIDTH / 2;
		rect.y += BORDER_WIDTH / 2;
		rect.width -= BORDER_WIDTH;
		rect.height -= BORDER_WIDTH;
		gc.setLineWidth(BORDER_WIDTH);

		gc.setBackgroundColor(isElementSelected() ? SELECTION_COLOR : StatusDisplayInfo.getStatusColor(object.getStatus()));
		gc.setAlpha(32);
		gc.fillRoundRectangle(rect, 8, 8);
		gc.setAlpha(255);

		gc.setForegroundColor(isElementSelected() ? SELECTION_COLOR : StatusDisplayInfo.getStatusColor(object.getStatus()));
      gc.setLineStyle(labelProvider.isElementSelected(element) ? Graphics.LINE_DOT : Graphics.LINE_SOLID);
		gc.drawRoundRectangle(rect, 8, 8);
	}
}
