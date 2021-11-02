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
package org.netxms.nxmc.base.widgets.helpers;

import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.eclipse.jface.fieldassist.ContentProposal;
import org.eclipse.jface.fieldassist.IContentProposal;
import org.eclipse.jface.fieldassist.IContentProposalProvider;

/**
 * Content proposal provider for editing search queries
 */
public class SearchQueryContentProposalProvider implements IContentProposalProvider
{
   private Map<String, SearchQueryAttribute> attributes;

   public SearchQueryContentProposalProvider(SearchQueryAttribute... attributes)
   {
      this.attributes = new HashMap<String, SearchQueryAttribute>(attributes.length);
      for(SearchQueryAttribute a : attributes)
         this.attributes.put(a.name.toLowerCase(), a);
   }

   /**
    * @see org.eclipse.jface.fieldassist.IContentProposalProvider#getProposals(java.lang.String, int)
    */
   @Override
   public IContentProposal[] getProposals(String contents, int position)
   {
      String element = contents.substring(contents.lastIndexOf(' ', position - 1) + 1, position);
      int index = element.indexOf(':');
      if (index != -1)
      {
         // Attribute list, suggest attributes
         SearchQueryAttribute a = attributes.get(element.substring(0, index + 1).toLowerCase());
         if (a != null)
         {
            String[] values = (a.staticValues != null) ? a.staticValues : ((a.valueProvider != null) ? a.valueProvider.getValues() : null);
            if ((values != null) && (values.length > 0))
            {
               element = element.substring(index + 1);

               // Check if there are already values in the list
               index = element.lastIndexOf(',');
               if (index != -1)
                  element = element.substring(index + 1);

               List<ContentProposal> proposals = new ArrayList<ContentProposal>(values.length);
               for(String v : values)
               {
                  if ((v.length() >= element.length()) && v.substring(0, element.length()).equalsIgnoreCase(element))
                  {
                     proposals.add(new ContentProposal(v));
                  }
               }
               proposals.sort(new Comparator<ContentProposal>() {
                  @Override
                  public int compare(ContentProposal p1, ContentProposal p2)
                  {
                     return p1.getContent().compareToIgnoreCase(p2.getContent());
                  }
               });
               return proposals.toArray(new IContentProposal[proposals.size()]);
            }
         }
      }
      else
      {
         List<ContentProposal> proposals = new ArrayList<ContentProposal>(attributes.size());
         for(SearchQueryAttribute a : attributes.values())
         {
            String name = a.name;
            if ((name.length() >= element.length()) && name.substring(0, element.length()).equalsIgnoreCase(element))
            {
               proposals.add(new ContentProposal(name));
            }
         }
         proposals.sort(new Comparator<ContentProposal>() {
            @Override
            public int compare(ContentProposal p1, ContentProposal p2)
            {
               return p1.getContent().compareToIgnoreCase(p2.getContent());
            }
         });
         return proposals.toArray(new IContentProposal[proposals.size()]);
      }
      return null;
   }
}
