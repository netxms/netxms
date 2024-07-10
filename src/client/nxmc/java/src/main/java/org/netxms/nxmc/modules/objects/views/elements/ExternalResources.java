/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2024 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.views.elements;

import java.util.ArrayList;
import java.util.List;
import org.eclipse.swt.SWT;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Control;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.ObjectUrl;
import org.netxms.client.objects.AbstractObject;
import org.netxms.nxmc.base.widgets.Hyperlink;
import org.netxms.nxmc.base.widgets.events.HyperlinkAdapter;
import org.netxms.nxmc.base.widgets.events.HyperlinkEvent;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objects.views.ObjectView;
import org.netxms.nxmc.tools.ExternalWebBrowser;
import org.xnap.commons.i18n.I18n;

/**
 * "External Resources" element
 */
public class ExternalResources extends OverviewPageElement
{
   private final I18n i18n = LocalizationHelper.getI18n(ExternalResources.class);

   private Composite content;
   private List<Element> elements = new ArrayList<Element>();

   /**
    * @param parent
    * @param anchor
    * @param objectView
    */
   public ExternalResources(Composite parent, OverviewPageElement anchor, ObjectView objectView)
   {
      super(parent, anchor, objectView);
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#getTitle()
    */
   @Override
   protected String getTitle()
   {
      return i18n.tr("External Resources");
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#isApplicableForObject(org.netxms.client.objects.AbstractObject)
    */
   @Override
   public boolean isApplicableForObject(AbstractObject object)
   {
      return (object != null) && object.hasUrls();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#createClientArea(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected Control createClientArea(Composite parent)
   {
      content = new Composite(parent, SWT.NONE);
      GridLayout layout = new GridLayout();
      layout.numColumns = 2;
      content.setLayout(layout);
      content.setBackground(parent.getDisplay().getSystemColor(SWT.COLOR_LIST_BACKGROUND));
      return content;
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.elements.OverviewPageElement#onObjectChange()
    */
   @Override
   protected void onObjectChange()
   {
      for(Element e : elements)
         e.dispose();
      elements.clear();
      for(ObjectUrl u : getObject().getUrls())
         elements.add(new Element(content, u));
      content.layout();
   }

   /**
    * Display element
    */
   private class Element
   {
      ObjectUrl url;
      Hyperlink link;
      Label description;
      
      Element(Composite parent, ObjectUrl url)
      {
         this.url = url;
         
         link = new Hyperlink(parent, SWT.NONE);
         link.setBackground(content.getBackground());
         link.setForeground(getDisplay().getSystemColor(SWT.COLOR_LIST_FOREGROUND));
         link.setText(url.getUrl().toExternalForm());
         link.addHyperlinkListener(new HyperlinkAdapter() {
            @Override
            public void linkActivated(HyperlinkEvent e)
            {
               ExternalWebBrowser.open(Element.this.url.getUrl());
            }
            
         });
         
         description = new Label(parent, SWT.NONE);
         description.setBackground(content.getBackground());
         description.setText(url.getDescription());
      }
      
      void dispose()
      {
         link.dispose();
         description.dispose();
      }
   }
}
