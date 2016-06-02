/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2016 Victor Kirhenshtein
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
package org.netxms.ui.eclipse.market.views.helpers;

import java.util.UUID;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.jface.viewers.IColorProvider;
import org.eclipse.jface.viewers.IFontProvider;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.SWT;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Font;
import org.eclipse.swt.graphics.FontData;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.widgets.Display;
import org.netxms.ui.eclipse.market.Activator;
import org.netxms.ui.eclipse.market.objects.EventReference;
import org.netxms.ui.eclipse.market.objects.MarketObject;
import org.netxms.ui.eclipse.market.objects.RepositoryElement;
import org.netxms.ui.eclipse.market.objects.RepositoryRuntimeInfo;
import org.netxms.ui.eclipse.market.objects.TemplateReference;

/**
 * Label provider for repository manager
 */
public class RepositoryLabelProvider extends LabelProvider implements ITableLabelProvider, IFontProvider, IColorProvider
{
   private Image imageEvent = Activator.getImageDescriptor("icons/event.gif").createImage();
   private Image imageRepository = Activator.getImageDescriptor("icons/repository.gif").createImage();
   private Image imageTemplate = Activator.getImageDescriptor("icons/template.png").createImage();
   private Font markFont;
   private Color markColor;
   
   /**
    * Constructor
    */
   public RepositoryLabelProvider()
   {
      FontData fd = JFaceResources.getDefaultFont().getFontData()[0];
      fd.style = SWT.BOLD;
      markFont = new Font(Display.getCurrent(), fd);
      markColor = new Color(Display.getCurrent(), 0, 148, 255);
   }
   
   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      if (columnIndex != 0)
         return null;
      
      if (element instanceof EventReference)
         return imageEvent;
      
      if (element instanceof TemplateReference)
         return imageTemplate;
      
      if (element instanceof RepositoryRuntimeInfo)
         return imageRepository;
      
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      MarketObject object = (MarketObject)element;
      switch(columnIndex)
      {
         case 0:
            return object.getName();
         case 1:
            if (object instanceof RepositoryElement)
               return Integer.toString(((RepositoryElement)object).getActualVersion());
            return "";
         case 2:
            UUID guid = object.getGuid();
            return (guid != null) ? guid.toString() : ""; 
      }
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
    */
   @Override
   public void dispose()
   {
      imageEvent.dispose();
      imageRepository.dispose();
      imageTemplate.dispose();
      markFont.dispose();
      markColor.dispose();
      super.dispose();
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IFontProvider#getFont(java.lang.Object)
    */
   @Override
   public Font getFont(Object element)
   {
      if ((element instanceof RepositoryElement) && ((RepositoryElement)element).isMarked())
         return markFont;
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      if ((element instanceof RepositoryElement) && ((RepositoryElement)element).isMarked())
         return markColor;
      return null;
   }

   /* (non-Javadoc)
    * @see org.eclipse.jface.viewers.IColorProvider#getBackground(java.lang.Object)
    */
   @Override
   public Color getBackground(Object element)
   {
      return null;
   }
}
