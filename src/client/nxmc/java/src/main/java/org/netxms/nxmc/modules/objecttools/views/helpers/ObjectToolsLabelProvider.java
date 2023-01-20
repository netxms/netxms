/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.nxmc.modules.objecttools.views.helpers;

import java.util.HashMap;
import java.util.Map;
import org.eclipse.jface.viewers.DecoratingLabelProvider;
import org.eclipse.jface.viewers.DecorationOverlayIcon;
import org.eclipse.jface.viewers.IDecoration;
import org.eclipse.jface.viewers.ILabelDecorator;
import org.eclipse.jface.viewers.ILabelProviderListener;
import org.eclipse.jface.viewers.ITableLabelProvider;
import org.eclipse.jface.viewers.LabelProvider;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Image;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objecttools.views.ObjectToolsEditor;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.resources.SharedIcons;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.xnap.commons.i18n.I18n;

/**
 * Label provider for object tools
 */
public class ObjectToolsLabelProvider extends DecoratingLabelProvider implements ITableLabelProvider
{
   private Color disabledColor = ThemeEngine.getForegroundColor("List.DisabledItem");

   /**
    * Create new decorating label provider for object tools
    */
   public ObjectToolsLabelProvider()
   {
      super(new BaseLabelProvider(), new Decorator());
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
    */
   @Override
   public Image getColumnImage(Object element, int columnIndex)
   {
      return (columnIndex == 0) ? getImage(element) : null;
   }

   /**
    * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
    */
   @Override
   public String getColumnText(Object element, int columnIndex)
   {
      return ((ITableLabelProvider)getLabelProvider()).getColumnText(element, columnIndex);
   }

   /**
    * @see org.eclipse.jface.viewers.DecoratingLabelProvider#getForeground(java.lang.Object)
    */
   @Override
   public Color getForeground(Object element)
   {
      return ((ObjectTool)element).isEnabled() ? null : disabledColor;
   }

   /**
    * Get names of all tool types.
    * 
    * @return
    */
   public static String[] getToolTypeNames()
   {
      I18n i18n = LocalizationHelper.getI18n(ObjectToolsLabelProvider.class);
      return new String[]
      {
         i18n.tr("Internal"),
         i18n.tr("Agent Command"),
         i18n.tr("SNMP Table"),
         i18n.tr("Agent List"),
         i18n.tr("URL"),
         i18n.tr("Local Command"),
         i18n.tr("Server Command"),
         i18n.tr("Download File"),
         i18n.tr("Server Script"),
         i18n.tr("Agent Table"),
         i18n.tr("SSH Command")
      };
   }

   /**
    * Base label provider for object tools
    */
   private static class BaseLabelProvider extends LabelProvider implements ITableLabelProvider
   {
      private final String[] toolTypeNames = getToolTypeNames();
      private final Image[] toolTypeImages = new Image[toolTypeNames.length];

      /**
   	 * The constructor
   	 */
      public BaseLabelProvider()
   	{
   		toolTypeImages[0] = ResourceManager.getImageDescriptor("icons/object-tools/internal_tool.gif").createImage();
         toolTypeImages[1] = ResourceManager.getImageDescriptor("icons/object-tools/terminal.png").createImage();
         toolTypeImages[2] = SharedIcons.PROPERTIES.createImage();
         toolTypeImages[3] = SharedIcons.PROPERTIES.createImage();
   		toolTypeImages[4] = SharedIcons.URL.createImage();
         toolTypeImages[5] = ResourceManager.getImageDescriptor("icons/object-tools/terminal.png").createImage();
         toolTypeImages[6] = ResourceManager.getImageDescriptor("icons/object-tools/terminal.png").createImage();
         toolTypeImages[7] = ResourceManager.getImageDescriptor("icons/object-tools/download.png").createImage();
         toolTypeImages[8] = ResourceManager.getImageDescriptor("icons/object-tools/script.png").createImage();
         toolTypeImages[9] = SharedIcons.PROPERTIES.createImage();
         toolTypeImages[10] = ResourceManager.getImageDescriptor("icons/object-tools/terminal.png").createImage();
   	}

      /**
       * @see org.eclipse.jface.viewers.LabelProvider#getImage(java.lang.Object)
       */
      @Override
      public Image getImage(Object element)
      {
         try
         {
            if ((((ObjectTool)element).getFlags() & ObjectTool.DISABLED) == 0)
            {
               return toolTypeImages[((ObjectTool)element).getToolType()];
            }
            else
            {
               return toolTypeImages[toolTypeImages.length - 1];
            }
         }
         catch(ArrayIndexOutOfBoundsException e)
         {
            return null;
         }
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnImage(java.lang.Object, int)
       */
      @Override
      public Image getColumnImage(Object element, int columnIndex)
      {
         return (columnIndex == 0) ? getImage(element) : null;
      }

      /**
       * @see org.eclipse.jface.viewers.ITableLabelProvider#getColumnText(java.lang.Object, int)
       */
      @Override
      public String getColumnText(Object element, int columnIndex)
      {
         ObjectTool tool = (ObjectTool)element;
         switch(columnIndex)
         {
            case ObjectToolsEditor.COLUMN_ID:
               return Long.toString(tool.getId());
            case ObjectToolsEditor.COLUMN_NAME:
               return tool.getName();
            case ObjectToolsEditor.COLUMN_TYPE:
               try
               {
                  return toolTypeNames[tool.getToolType()];
               }
               catch(ArrayIndexOutOfBoundsException e)
               {
                  return "<unknown>";
               }
            case ObjectToolsEditor.COLUMN_DESCRIPTION:
               return tool.getDescription();
         }
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.BaseLabelProvider#dispose()
       */
      @Override
      public void dispose()
      {
         for(Image i : toolTypeImages)
            i.dispose();
         super.dispose();
      }
   }

   /**
    * Label decorator for object tools
    */
   private static class Decorator implements ILabelDecorator
   {
      private Map<Image, Image> imageCache = new HashMap<Image, Image>();

      /**
       * @see org.eclipse.jface.viewers.ILabelDecorator#decorateImage(org.eclipse.swt.graphics.Image, java.lang.Object)
       */
      @Override
      public Image decorateImage(Image image, Object element)
      {
         if ((image == null) || !(element instanceof ObjectTool))
            return null;

         ObjectTool tool = (ObjectTool)element;
         if (tool.isEnabled())
            return null;

         Image decoratedImage = imageCache.get(image);
         if (decoratedImage == null)
         {
            DecorationOverlayIcon icon = new DecorationOverlayIcon(image, StatusDisplayInfo.getStatusOverlayImageDescriptor(ObjectStatus.DISABLED), IDecoration.BOTTOM_RIGHT);
            decoratedImage = icon.createImage();
            imageCache.put(image, decoratedImage);
         }
         return decoratedImage;
      }

      /**
       * @see org.eclipse.jface.viewers.ILabelDecorator#decorateText(java.lang.String, java.lang.Object)
       */
      @Override
      public String decorateText(String text, Object element)
      {
         return null;
      }

      /**
       * @see org.eclipse.jface.viewers.IBaseLabelProvider#isLabelProperty(java.lang.Object, java.lang.String)
       */
      @Override
      public boolean isLabelProperty(Object element, String property)
      {
         return false;
      }

      /**
       * @see org.eclipse.jface.viewers.IBaseLabelProvider#addListener(org.eclipse.jface.viewers.ILabelProviderListener)
       */
      @Override
      public void addListener(ILabelProviderListener listener)
      {
      }

      /**
       * @see org.eclipse.jface.viewers.IBaseLabelProvider#removeListener(org.eclipse.jface.viewers.ILabelProviderListener)
       */
      @Override
      public void removeListener(ILabelProviderListener listener)
      {
      }

      /**
       * @see org.eclipse.jface.viewers.IBaseLabelProvider#dispose()
       */
      @Override
      public void dispose()
      {
         for(Image i : imageCache.values())
            i.dispose();
      }
   }
}
