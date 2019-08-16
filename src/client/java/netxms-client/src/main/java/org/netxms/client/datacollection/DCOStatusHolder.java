package org.netxms.client.datacollection;

/*
 * Class that stores DCO id and new status. Used in update notifications.
 */
public class DCOStatusHolder
{
   private long[] items;
   private int status;

   /**
    * Status holder constructor
    *
    * @param items TODO
    * @param status TODO
    */
   public DCOStatusHolder(long[] items, int status)
   {
      super();
      this.items = items;
      this.status = status;
   }

   /**
    * @return the dciId
    */
   public long[] getDciIdArray()
   {
      return items;
   }

   /**
    * TODO
    *
    * @param items TODO
    */
   public void setDciIdArray(long[] items)
   {
      this.items = items;
   }

   /**
    * @return the status
    */
   public int getStatus()
   {
      return status;
   }

   /**
    * @param status the status to set
    */
   public void setStatus(int status)
   {
      this.status = status;
   }
}
