/**
 * NetXMS - open source network management system
 * Copyright (C) 2022 Raden Solutions
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
package org.netxms.nxmc.modules.objects.views.helpers;

import java.util.ArrayList;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.eclipse.core.runtime.IProgressMonitor;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.TextOutputListener;
import org.netxms.client.constants.ObjectPollType;
import org.netxms.client.objects.interfaces.PollingTarget;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.xnap.commons.i18n.I18n;

/**
 * Proxy for multi view polling 
 */
public class PollerProxy implements TextOutputListener
{
   private final I18n i18n = LocalizationHelper.getI18n(PollerProxy.class);

   private Set<TextOutputListener> listenerSet;
   private List<String> content;
   private boolean pollActive;
   private PollingTarget target;
   private ObjectPollType pollType;

   /**
    * Default constructor
    */
   public PollerProxy(PollingTarget target, ObjectPollType pollType, TextOutputListener listener)
   {
      listenerSet = new HashSet<TextOutputListener>();
      listenerSet.add(listener);
      content = new ArrayList<String>();
      pollActive = false;
      this.target = target;
      this.pollType = pollType;
      startPoll();
   }

   /**
    * Add new listener
    * 
    * @param listener new listener to add
    * @param sendAllPreviousContent 
    */
   public void addListener(TextOutputListener listener, boolean sendAllPreviousContent)
   {
      synchronized(listenerSet)
      {
         if (sendAllPreviousContent && !listenerSet.contains(listener)) 
         {
            for (String s : content)
               listener.messageReceived(s);
         }
         else
         {
            listenerSet.add(listener);
         }
      }
   }
   
   /**
    * Remove listener and check if this poller should be seleted 
    * 
    * @param listener to be removed
    * 
    * @return true if no more listener left
    */
   public boolean removeListenerAndCheck(TextOutputListener listener)
   {
      boolean noMoreListenners = true;
      synchronized(listenerSet)
      {
         listenerSet.remove(listener);
         noMoreListenners = listenerSet.size() == 0;
      }
      return noMoreListenners;
   }
   

   /**
    * Start poll
    */
   public void startPoll()
   {
      if (pollActive)
         return;
      pollActive = true;

      messageReceived("\u007Fl**** Poll request sent to server ****\n");

      NXCSession session = Registry.getSession();
      Job job = new Job(String.format(i18n.tr("Node poll: %s [%d]"), target.getObjectName(), target.getObjectId()), null) {

         @Override
         protected void run(IProgressMonitor monitor) 
         {
            try
            {
               session.pollObject(target.getObjectId(), pollType, PollerProxy.this);
            }
            catch(Exception e)
            {
            }
            pollActive = false;
         }

         @Override
         protected String getErrorMessage()
         {
            return null;
         }
      };
      job.setSystem(true);
      job.start();
   }

   /**
    * @see org.netxms.client.TextOutputListener#messageReceived(java.lang.String)
    */
   @Override
   public void messageReceived(String text)
   {
      synchronized(listenerSet)
      {
         content.add(text);
         for (TextOutputListener l :listenerSet)
         {
            l.messageReceived(text);
         }
      }
   }

   /**
    * @see org.netxms.client.TextOutputListener#setStreamId(long)
    */
   @Override
   public void setStreamId(long streamId)
   {
      synchronized(listenerSet)
      {
         for (TextOutputListener l :listenerSet)
         {
            l.setStreamId(streamId);
         }
      }
   }

   /**
    * @see org.netxms.client.TextOutputListener#onFailure(java.lang.Exception)
    */
   @Override
   public void onFailure(Exception exception)
   {
      messageReceived(String.format("\u007FePOLL ERROR: %s\n", (exception instanceof NXCException) ? exception.getLocalizedMessage() : exception.getClass().getName()));
      messageReceived("\u007Fl**** Poll failed ****\n\n");
   }

   /**
    * @see org.netxms.client.TextOutputListener#onSuccess()
    */
   @Override
   public void onSuccess()
   {
      messageReceived("\u007Fl**** Poll completed successfully ****\n\n");
      synchronized(listenerSet)
      {
         for (TextOutputListener l :listenerSet)
         {
            l.onSuccess();
         }
      }
   }
}
