/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2018 Victor Kirhenshtein
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

import org.eclipse.rap.json.JsonObject;
import org.eclipse.rap.json.JsonValue;
import org.eclipse.rap.rwt.RWT;
import org.eclipse.rap.rwt.remote.AbstractOperationHandler;
import org.eclipse.rap.rwt.remote.Connection;
import org.eclipse.rap.rwt.remote.RemoteObject;
import org.eclipse.rap.rwt.widgets.WidgetUtil;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseTrackListener;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Event;

/**
 * Message proxy widget - invisible widget that can be used to pass 
 * events from client scripts to server side
 */
public class MsgProxyWidget extends Composite
{
   private static final String MOUSE_HOVER = "mouseHover";
   private static final String MOUSE_EXIT = "mouseExit";
   
   private final RemoteObject remoteObject;
   private MouseTrackListener mouseTrackListener = null;
   
   /**
    * @param parent
    * @param style
    */
   public MsgProxyWidget(Composite parent)
   {
      super(parent, SWT.NONE);
      Connection connection = RWT.getUISession().getConnection();
      remoteObject = connection.createRemoteObject("netxms.MsgProxy");
      remoteObject.setHandler(new AbstractOperationHandler() {
         @Override
         public void handleNotify(String event, JsonObject properties)
         {
            handleEvent(event, properties);
         }
      });
      remoteObject.set("parent", WidgetUtil.getId(this));
      remoteObject.listen(MOUSE_HOVER, true);
      remoteObject.listen(MOUSE_EXIT, true);
   }
   
   /**
    * Get ID of corresponding remote object
    * 
    * @return ID of corresponding remote object
    */
   public String getRemoteObjectId()
   {
      return remoteObject.getId();
   }
   
   /**
    * @param mouseTrackListener
    */
   public void setMouseTrackListener(MouseTrackListener mouseTrackListener)
   {
      this.mouseTrackListener = mouseTrackListener;
   }
   
   /**
    * Handle client side event
    * 
    * @param event
    * @param properties
    */
   private void handleEvent(String event, JsonObject properties)
   {
      if ((MOUSE_HOVER.equals(event) || MOUSE_EXIT.equals(event)) && (mouseTrackListener != null))
      {
         Event e = new Event();
         e.display = getDisplay();
         e.widget = getParent();
         e.doit = true;
         e.x = getIntProperty(properties, "x");
         e.y = getIntProperty(properties, "y");
         MouseEvent me = new MouseEvent(e);
         if (MOUSE_HOVER.equals(event))
            mouseTrackListener.mouseHover(me);
         else
            mouseTrackListener.mouseExit(me);
      }
   }
   
   /**
    * Get JSON property as integer
    * 
    * @param properties
    * @param name
    * @return
    */
   private static int getIntProperty(JsonObject properties, String name)
   {
      JsonValue v = properties.get(name);
      return (v != null) ? v.asInt() : 0;
   }
}
