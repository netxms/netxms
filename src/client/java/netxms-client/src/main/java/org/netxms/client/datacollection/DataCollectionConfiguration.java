/**
 * Copyright (C) 2003-2013 Victor Kirhenshtein
 * <p>
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * <p>
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * <p>
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.client.datacollection;

import java.io.IOException;
import java.util.HashMap;

import org.netxms.base.NXCPCodes;
import org.netxms.base.NXCPMessage;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.SessionListener;
import org.netxms.client.SessionNotification;
import org.netxms.client.constants.RCC;

/**
 * Data collection configuration for node
 */
public class DataCollectionConfiguration
{
   private NXCSession session;
   private long nodeId;
   private HashMap<Long, DataCollectionObject> items;
   private boolean isOpen = false;
   private Object userData = null;
   private SessionListener listener;
   private DataCollectionConfigurationChangeListener changeListener;

   /**
    * Create empty data collection configuration.
    *
    * @param session The NXCSession
    * @param nodeId  The node ID
    */
   public DataCollectionConfiguration(final NXCSession session, final long nodeId)
   {
      this.session = session;
      this.nodeId = nodeId;
      items = new HashMap<Long, DataCollectionObject>(0);
   }

   /**
    * Open data collection configuration.
    *
    * @param changeListener change listener
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void open(DataCollectionConfigurationChangeListener changeListener) throws IOException, NXCException
   {
      NXCPMessage msg = session.newMessage(NXCPCodes.CMD_GET_NODE_DCI_LIST);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      session.sendMessage(msg);
      session.waitForRCC(msg.getMessageId());
      this.changeListener = changeListener;

      while(true)
      {
         final NXCPMessage response = session.waitForMessage(NXCPCodes.CMD_NODE_DCI, msg.getMessageId());
         if (response.isEndOfSequence())
            break;

         int type = response.getFieldAsInt32(NXCPCodes.VID_DCOBJECT_TYPE);
         DataCollectionObject dco;
         switch(type)
         {
            case DataCollectionObject.DCO_TYPE_ITEM:
               dco = new DataCollectionItem(this, response);
               break;
            case DataCollectionObject.DCO_TYPE_TABLE:
               dco = new DataCollectionTable(this, response);
               break;
            default:
               dco = null;
               break;
         }
         if (dco != null)
            items.put(dco.getId(), dco);
      }

      listener = new SessionListener() {
         @Override
         public void notificationHandler(SessionNotification n)
         {
            if (n.getSubCode() != nodeId)
               return;

            if (n.getCode() == SessionNotification.DCI_UPDATE)
            {
               final DataCollectionObject dco = (DataCollectionObject)n.getObject();
               updateItemFromNotification(dco);
               if (DataCollectionConfiguration.this.changeListener != null)
                  DataCollectionConfiguration.this.changeListener.onUpdate(dco);
            }
            else if (n.getCode() == SessionNotification.DCI_DELETE)
            {
               final long id = (Long)n.getObject();
               removeItemFromNotification(id);
               if (DataCollectionConfiguration.this.changeListener != null)
                  DataCollectionConfiguration.this.changeListener.onDelete(id);
            }
            else if (n.getCode() == SessionNotification.DCI_STATE_CHANGE)
            {
               DCOStatusHolder stHolder = (DCOStatusHolder)n.getObject();
               updateItemStatusFromNotification(stHolder.getDciIdArray(), stHolder.getStatus());
               if (DataCollectionConfiguration.this.changeListener != null)
               {
                  for(long id : stHolder.getDciIdArray())
                     DataCollectionConfiguration.this.changeListener.onStatusChange(id, stHolder.getStatus());
               }
            }
         }
      };
      session.addListener(listener);
      isOpen = true;
   }

   /**
    * Close data collection configuration.
    *
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void close() throws IOException, NXCException
   {
      NXCPMessage msg = session.newMessage(NXCPCodes.CMD_UNLOCK_NODE_DCI_LIST);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      session.sendMessage(msg);
      session.waitForRCC(msg.getMessageId());
      items.clear();
      session.removeListener(listener);
      isOpen = false;
      changeListener = null;
   }

   /**
    * Get list of data collection items
    *
    * @return List of data collection items
    */
   public DataCollectionObject[] getItems()
   {
      return items.values().toArray(new DataCollectionObject[items.size()]);
   }

   /**
    * Find data collection object by ID.
    *
    * @param id DCI ID
    * @return Data collection item or <b>null</b> if item with given ID does not exist
    */
   public DataCollectionObject findItem(long id)
   {
      return items.get(id);
   }

   /**
    * Remove data collection item from the list by ID from notification
    *
    * @param id DCI ID
    */
   private void removeItemFromNotification(long id)
   {
      items.remove(id);
   }

   /**
    * TODO
    *
    * @param dco
    */
   private void updateItemFromNotification(DataCollectionObject dco)
   {
      DataCollectionObject newDco = null;
      if (dco instanceof DataCollectionItem)
         newDco = new DataCollectionItem(this, (DataCollectionItem)dco);
      else if (dco instanceof DataCollectionTable)
         newDco = new DataCollectionTable(this, (DataCollectionTable)dco);

      items.put(newDco.getId(), newDco);
   }

   /**
    * Update DCI status from session notification
    *
    * @param idList list of DCI identifiers
    * @param status new status
    */
   private void updateItemStatusFromNotification(long[] idList, int status)
   {
      for(int i = 0; i < idList.length; i++)
      {
         DataCollectionObject o = items.get(idList[i]);
         if (o != null)
            o.setStatus(status);
      }
   }

   /**
    * Find data collection object by ID.
    *
    * @param id          data collection object ID
    * @param classFilter class filter for found object
    * @return Data collection item or <b>null</b> if item with given ID does not exist
    */
   public DataCollectionObject findItem(long id, Class<? extends DataCollectionObject> classFilter)
   {
      DataCollectionObject o = items.get(id);
      if (o == null)
         return null;
      return classFilter.isInstance(o) ? o : null;
   }

   /**
    * Create new data collection item. This method is deprecated - new code should call modifyObject().
    *
    * @param object The DataCollectionObject to create
    * @return Identifier assigned to created item
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   @Deprecated
   public long createItem(DataCollectionObject object) throws IOException, NXCException
   {
      if (object != null)
         return modifyObject(object);
      else
         return modifyObject(new DataCollectionItem(this, 0));
   }

   /**
    * Create new data collection table. This method is deprecated - new code should call modifyObject().
    *
    * @param object The DataCollectionObject to create
    * @return Identifier assigned to created item
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   @Deprecated
   public long createTable(DataCollectionObject object) throws IOException, NXCException
   {
      if (object != null)
         return modifyObject(object);
      else
         return modifyObject(new DataCollectionTable(this, 0));
   }

   /**
    * Modify data collection object.
    *
    * @param dcObjectId Data collection object identifier
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void modifyObject(long dcObjectId) throws IOException, NXCException
   {
      DataCollectionObject dco = items.get(dcObjectId);
      if (dco == null)
         throw new NXCException(RCC.INVALID_DCI_ID);
      modifyObject(dco);
   }

   /**
    * Modify data collection object.
    *
    * @param dco Data collection object
    * @return Identifier assigned to created item
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public long modifyObject(DataCollectionObject dco) throws IOException, NXCException
   {
      NXCPMessage msg = session.newMessage(NXCPCodes.CMD_MODIFY_NODE_DCI);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      if (dco != null)
         dco.fillMessage(msg);
      session.sendMessage(msg);
      return session.waitForRCC(msg.getMessageId()).getFieldAsInt64(NXCPCodes.VID_DCI_ID);
   }

   /**
    * Copy or move DCIs
    *
    * @param destNodeId Destination node ID
    * @param items      List of data collection items to copy or move
    * @param move       Move flag - true if items need to be moved
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   private void copyObjectsInternal(long destNodeId, long[] items, boolean move) throws IOException, NXCException
   {
      NXCPMessage msg = session.newMessage(NXCPCodes.CMD_COPY_DCI);
      msg.setFieldInt32(NXCPCodes.VID_SOURCE_OBJECT_ID, (int)nodeId);
      msg.setFieldInt32(NXCPCodes.VID_DESTINATION_OBJECT_ID, (int)destNodeId);
      msg.setFieldInt16(NXCPCodes.VID_MOVE_FLAG, move ? 1 : 0);
      msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, items.length);
      msg.setField(NXCPCodes.VID_ITEM_LIST, items);
      session.sendMessage(msg);
      session.waitForRCC(msg.getMessageId());
   }

   /**
    * Copy data collection objects.
    *
    * @param destNodeId Destination node ID
    * @param items      List of data collection items to copy
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void copyObjects(long destNodeId, long[] items) throws IOException, NXCException
   {
      copyObjectsInternal(destNodeId, items, false);
   }

   /**
    * Move data collection objects.
    *
    * @param destNodeId Destination node ID
    * @param items      List of data collection items to move
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void moveObjects(long destNodeId, long[] items) throws IOException, NXCException
   {
      copyObjectsInternal(destNodeId, items, true);
   }

   /**
    * Clear collected data for given DCI.
    *
    * @param itemId Data collection item ID
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void clearCollectedData(long itemId) throws IOException, NXCException
   {
      NXCPMessage msg = session.newMessage(NXCPCodes.CMD_CLEAR_DCI_DATA);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      msg.setFieldInt32(NXCPCodes.VID_DCI_ID, (int)itemId);
      session.sendMessage(msg);
      session.waitForRCC(msg.getMessageId());
   }

   /**
    * Set status of data collection objects.
    *
    * @param items  Data collection items' identifiers
    * @param status New status
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void setObjectStatus(long[] items, int status) throws IOException, NXCException
   {
      NXCPMessage msg = session.newMessage(NXCPCodes.CMD_SET_DCI_STATUS);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      msg.setFieldInt16(NXCPCodes.VID_DCI_STATUS, status);
      msg.setFieldInt32(NXCPCodes.VID_NUM_ITEMS, items.length);
      msg.setField(NXCPCodes.VID_ITEM_LIST, items);
      session.sendMessage(msg);
      session.waitForRCC(msg.getMessageId());
   }

   /**
    * Delete data collection object.
    *
    * @param itemId Data collection item identifier
    * @throws IOException  if socket I/O error occurs
    * @throws NXCException if NetXMS server returns an error or operation was timed out
    */
   public void deleteObject(long itemId) throws IOException, NXCException
   {
      NXCPMessage msg = session.newMessage(NXCPCodes.CMD_DELETE_NODE_DCI);
      msg.setFieldInt32(NXCPCodes.VID_OBJECT_ID, (int)nodeId);
      msg.setFieldInt32(NXCPCodes.VID_DCI_ID, (int)itemId);
      session.sendMessage(msg);
      session.waitForRCC(msg.getMessageId());
      items.remove(itemId);
   }

   /* (non-Javadoc)
    * @see java.lang.Object#finalize()
    */
   @Override
   protected void finalize() throws Throwable
   {
      if (isOpen)
         close();
      super.finalize();
   }

   /**
    * @return the nodeId
    */
   public long getNodeId()
   {
      return nodeId;
   }

   /**
    * @return the userData
    */
   public Object getUserData()
   {
      return userData;
   }

   /**
    * @param userData the userData to set
    */
   public void setUserData(Object userData)
   {
      this.userData = userData;
   }

   /**
    * @return the session
    */
   protected final NXCSession getSession()
   {
      return session;
   }
}
