package org.netxms.ui.eclipse.market.objects;

import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

/**
 * Objects category (events, templates, etc.)
 */
public class Category implements MarketObject
{
   private String name;
   private MarketObject parent;
   private List<MarketObject> objects = new ArrayList<MarketObject>();

   /**
    * @param name
    */
   public Category(String name, MarketObject parent)
   {
      this.name = name;
      this.parent = parent;
   }
   
   /**
    * Add object to category
    * 
    * @param object
    */
   public void add(RepositoryElement object)
   {
      objects.add(object);
   }
   
   /**
    * Clear objects
    */
   public void clear()
   {
      objects.clear();
   }
   
   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#getName()
    */
   @Override
   public String getName()
   {
      return name;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#getGuid()
    */
   @Override
   public UUID getGuid()
   {
      return null;
   }

   @Override
   public MarketObject getParent()
   {
      return parent;
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#getChildren()
    */
   @Override
   public MarketObject[] getChildren()
   {
      return objects.toArray(new MarketObject[objects.size()]);
   }

   /* (non-Javadoc)
    * @see org.netxms.ui.eclipse.market.objects.MarketObject#hasChildren()
    */
   @Override
   public boolean hasChildren()
   {
      return !objects.isEmpty();
   }
}
