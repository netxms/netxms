/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Raden Solutions
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
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.widgets.Control;
import org.netxms.nxmc.tools.MapInputListener;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Server side half of the map input custom widget. Receives classified gestures from mapinput.js and dispatches them to a
 * {@link MapInputListener}.
 *
 * RAP has no server side SWT.MouseMove or SWT.MouseWheel, by documented intent, so a canvas gets no continuous input stream at all.
 * This uses the same RemoteObject mechanism as {@link MsgProxyWidget}, which exists for exactly the same reason - it was built to
 * deliver mouseHover and mouseExit, two other events RAP does not deliver server side.
 */
public class MapInputWidget
{
   private static final Logger logger = LoggerFactory.getLogger(MapInputWidget.class);

   private static final String EVENT_ZOOM = "zoom";
   private static final String EVENT_PAN = "pan";
   private static final String EVENT_DRAG_PAN = "dragpan";

   private final RemoteObject remoteObject;
   private final MapInputListener listener;

   /**
    * Create map input handler for given control.
    *
    * @param control control to receive input for
    * @param listener listener to dispatch gestures to
    */
   public MapInputWidget(Control control, MapInputListener listener)
   {
      this.listener = listener;

      Connection connection = RWT.getUISession().getConnection();
      remoteObject = connection.createRemoteObject("netxms.MapInput");
      remoteObject.setHandler(new AbstractOperationHandler() {
         @Override
         public void handleNotify(String event, JsonObject properties)
         {
            // RAP calls this while it is still reading the request, but dispatches the SWT events of the same request (such as
            // the mouse events Draw2D drags objects with) later. Queue the gesture so that both apply in the order they
            // happened.
            control.getDisplay().asyncExec(() -> {
               if (control.isDisposed())
                  return;
               // If a handler throws - most plausibly because the map view is being torn down while a touch is still in
               // flight - contain it: a dropped gesture is recoverable, a broken session is not.
               try
               {
                  handleEvent(event, properties);
               }
               catch(Exception e)
               {
                  logger.debug("Exception while handling map input event \"" + event + "\"", e);
               }
            });
         }
      });
      remoteObject.set("parent", WidgetUtil.getId(control));
      remoteObject.listen(EVENT_ZOOM, true);
      remoteObject.listen(EVENT_PAN, true);
      remoteObject.listen(EVENT_DRAG_PAN, true);

      control.addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            remoteObject.destroy();
         }
      });
   }

   /**
    * Tell the browser where the view is and how far it may go, so that its gesture preview stops where the server will stop
    * instead of overshooting and jumping back when the server's redraw arrives.
    *
    * @param viewX view location X (scaled pixels)
    * @param viewY view location Y (scaled pixels)
    * @param mapWidth map width at zoom 1, as used to clamp the view
    * @param mapHeight map height at zoom 1, as used to clamp the view
    * @param viewWidth visible area width
    * @param viewHeight visible area height
    * @param zoom current zoom
    * @param minZoom smallest zoom accepted
    * @param maxZoom largest zoom accepted
    */
   public void setViewState(int viewX, int viewY, int mapWidth, int mapHeight, int viewWidth, int viewHeight, double zoom, double minZoom,
         double maxZoom)
   {
      JsonObject state = new JsonObject();
      state.add("x", viewX);
      state.add("y", viewY);
      state.add("w", mapWidth);
      state.add("h", mapHeight);
      state.add("vw", viewWidth);
      state.add("vh", viewHeight);
      state.add("zoom", zoom);
      state.add("minZoom", minZoom);
      state.add("maxZoom", maxZoom);
      remoteObject.set("view", state);
   }

   /**
    * Handle client side event.
    *
    * @param event event name
    * @param properties event properties
    */
   private void handleEvent(String event, JsonObject properties)
   {
      if (listener == null)
         return;

      if (EVENT_ZOOM.equals(event))
      {
         double factor = getDoubleProperty(properties, "factor", 1.0);
         if (factor > 0.0)
            listener.onZoom(factor, getIntProperty(properties, "x"), getIntProperty(properties, "y"), getIntProperty(properties, "dx"),
                  getIntProperty(properties, "dy"));
      }
      else if (EVENT_PAN.equals(event))
      {
         listener.onPan(getIntProperty(properties, "dx"), getIntProperty(properties, "dy"));
      }
      else if (EVENT_DRAG_PAN.equals(event))
      {
         listener.onDragPan(getIntProperty(properties, "dx"), getIntProperty(properties, "dy"), getIntProperty(properties, "g"),
               getIntProperty(properties, "sx"), getIntProperty(properties, "sy"));
      }
   }

   /**
    * Get JSON property as integer.
    */
   private static int getIntProperty(JsonObject properties, String name)
   {
      JsonValue v = properties.get(name);
      return (v != null) ? v.asInt() : 0;
   }

   /**
    * Get JSON property as double.
    */
   private static double getDoubleProperty(JsonObject properties, String name, double defaultValue)
   {
      JsonValue v = properties.get(name);
      return (v != null) ? v.asDouble() : defaultValue;
   }
}
