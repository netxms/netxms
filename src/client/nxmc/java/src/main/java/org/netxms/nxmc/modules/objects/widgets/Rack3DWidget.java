/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2026 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.objects.widgets;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Base64;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import java.util.UUID;
import org.eclipse.swt.SWT;
import org.eclipse.swt.browser.Browser;
import org.eclipse.swt.browser.BrowserFunction;
import org.eclipse.swt.browser.ProgressAdapter;
import org.eclipse.swt.browser.ProgressEvent;
import org.eclipse.swt.events.DisposeEvent;
import org.eclipse.swt.events.DisposeListener;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.netxms.base.NXCommon;
import org.netxms.client.LibraryImage;
import org.netxms.client.constants.ObjectStatus;
import org.netxms.client.constants.RackOrientation;
import org.netxms.client.objects.Rack;
import org.netxms.client.objects.configs.PassiveRackElement;
import org.netxms.client.objects.interfaces.HardwareEntity;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.modules.imagelibrary.ImageProvider;
import org.netxms.nxmc.modules.imagelibrary.ImageUpdateListener;
import org.netxms.nxmc.modules.objects.widgets.helpers.ElementSelectionListener;
import org.netxms.nxmc.resources.StatusDisplayInfo;
import org.netxms.nxmc.resources.ThemeEngine;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.google.gson.Gson;

/**
 * WebGL-based 3D rack display widget.
 *
 * The rack is rendered inside an embedded {@link Browser} using Three.js. This
 * approach works on both the SWT desktop client and the RWT web client because
 * both toolkits provide org.eclipse.swt.browser.Browser (same pattern as
 * AiAssistantChatWidget and MarkdownViewer).
 *
 * Division of responsibilities:
 * <ul>
 *   <li>Java owns the model and computes status colors; it serializes a scene
 *       description to JSON and pushes it to the page via {@code nxSetScene()}.</li>
 *   <li>JavaScript owns geometry, camera and picking; it calls back into Java
 *       through {@link BrowserFunction}s for object selection and texture data.</li>
 * </ul>
 *
 * NOTE: this is an MVP skeleton. It renders status-colored boxes at the correct
 * rack positions and maps existing front/rear device images onto the device
 * face. Partial depth is derived from the element orientation (a FRONT- or
 * REAR-only element occupies a fraction of the rack depth at the matching end;
 * FILL spans the whole depth) - no model extension needed for that. True
 * per-device depth/width and physical link (cable) visualization are out of
 * scope for this skeleton.
 */
public class Rack3DWidget extends Composite implements ImageUpdateListener
{
   private static final Logger logger = LoggerFactory.getLogger(Rack3DWidget.class);

   /** Resource path of the page template (Three.js is inlined into it at load time). */
   private static final String TEMPLATE_RESOURCE = "/webgl/rack3d.html";
   private static final String THREEJS_RESOURCE = "/webgl/three.min.js";
   private static final String THREEJS_MARKER = "/*__THREEJS__*/";
   private static final String BG_COLOR_MARKER = "__BG_COLOR__";

   private final Rack rack;
   private final Browser browser;
   private final Set<ElementSelectionListener> selectionListeners = new HashSet<ElementSelectionListener>(0);
   private final List<BrowserFunction> browserFunctions = new ArrayList<BrowserFunction>(2);
   private boolean pageLoaded = false;

   /**
    * Create 3D rack widget.
    *
    * @param parent parent composite
    * @param style widget style
    * @param rack rack object to display
    * @param view owning view (used for context propagation)
    */
   public Rack3DWidget(Composite parent, int style, Rack rack, View view)
   {
      super(parent, style);
      this.rack = rack;

      setLayout(new FillLayout());
      browser = new Browser(this, SWT.NONE);

      // JS -> Java: an element was picked in the scene. Arguments: id, passive flag
      // (id < 0 means "nothing selected").
      browserFunctions.add(new BrowserFunction(browser, "nxOnSelect") {
         @Override
         public Object function(Object[] arguments)
         {
            long id = (arguments.length > 0) ? (long)(double)(Double)arguments[0] : -1;
            boolean passive = (arguments.length > 1) && Boolean.TRUE.equals(arguments[1]);
            Object selected = (id >= 0) ? findElement(id, passive) : null;
            for(ElementSelectionListener l : selectionListeners)
               l.objectSelected(selected);
            return null;
         }
      });

      // JS -> Java: lazily fetch a device face image as a data: URL, keyed by image GUID
      browserFunctions.add(new BrowserFunction(browser, "nxGetTexture") {
         @Override
         public Object function(Object[] arguments)
         {
            if ((arguments.length == 0) || !(arguments[0] instanceof String))
               return null;
            return textureDataUrl(UUID.fromString((String)arguments[0]));
         }
      });

      browser.addProgressListener(new ProgressAdapter() {
         @Override
         public void completed(ProgressEvent event)
         {
            pageLoaded = true;
            pushScene();
         }
      });

      ImageProvider.getInstance().addUpdateListener(this);

      addDisposeListener(new DisposeListener() {
         @Override
         public void widgetDisposed(DisposeEvent e)
         {
            ImageProvider.getInstance().removeUpdateListener(Rack3DWidget.this);
            for(BrowserFunction f : browserFunctions)
               f.dispose();
            browserFunctions.clear();
         }
      });

      browser.setText(loadPageTemplate());
   }

   /**
    * Rebuild the scene from the current rack state and push it to the page.
    */
   public void refresh()
   {
      if (pageLoaded)
         pushScene();
   }

   /**
    * Serialize the rack into a scene description and hand it to the JS layer.
    */
   private void pushScene()
   {
      if (browser.isDisposed())
         return;

      Scene scene = new Scene();
      scene.height = rack.getHeight();
      scene.topBottomNumbering = rack.isTopBottomNumbering();
      scene.frontOnly = rack.isFrontSideOnly();
      scene.bgColor = colorToHex(ThemeEngine.getBackgroundColor("Rack"));
      scene.fgColor = colorToHex(ThemeEngine.getForegroundColor("Rack"));

      for(HardwareEntity n : rack.getUnits())
      {
         if ((n.getRackPosition() < 1) || (n.getRackPosition() > rack.getHeight()))
            continue;
         scene.units.add(Unit.from(n));
      }
      for(PassiveRackElement p : rack.getPassiveElements())
      {
         if ((p.getPosition() < 1) || (p.getPosition() > rack.getHeight()))
            continue;
         scene.units.add(Unit.from(p));
      }

      String json = new Gson().toJson(scene);
      browser.execute("nxSetScene(" + toJsStringLiteral(json) + ");");
   }

   /**
    * Find a hardware entity in the rack by its object ID.
    *
    * @param id object id
    * @return hardware entity or null if not found
    */
   private HardwareEntity findUnit(long id)
   {
      for(HardwareEntity n : rack.getUnits())
         if (n.getObjectId() == id)
            return n;
      return null;
   }

   /**
    * Resolve a picked element by id. Passive elements and hardware entities live
    * in separate id spaces, so the passive flag selects which to search.
    *
    * @param id element id
    * @param passive true to look up a passive rack element
    * @return selected element (HardwareEntity or PassiveRackElement) or null
    */
   private Object findElement(long id, boolean passive)
   {
      if (passive)
      {
         for(PassiveRackElement p : rack.getPassiveElements())
            if (p.getId() == id)
               return p;
         return null;
      }
      return findUnit(id);
   }

   /**
    * @see org.netxms.nxmc.modules.imagelibrary.ImageUpdateListener#imageUpdated(java.util.UUID)
    */
   @Override
   public void imageUpdated(UUID guid)
   {
      // A device/passive image finished loading after the scene was first pushed;
      // re-push so the JS layer can fetch the now-available texture.
      if (pageLoaded && !browser.isDisposed() && rackUsesImage(guid))
         pushScene();
   }

   /**
    * Check whether any rack element references the given image.
    *
    * @param guid image GUID
    * @return true if the image is used by a unit or passive element
    */
   private boolean rackUsesImage(UUID guid)
   {
      for(HardwareEntity n : rack.getUnits())
         if (guid.equals(n.getFrontRackImage()) || guid.equals(n.getRearRackImage()))
            return true;
      for(PassiveRackElement p : rack.getPassiveElements())
         if (guid.equals(p.getFrontImage()) || guid.equals(p.getRearImage()))
            return true;
      return false;
   }

   /**
    * Encode a library image as a data: URL suitable for use as a WebGL texture.
    *
    * @param guid image GUID
    * @return data URL, or null if the image is unknown/unavailable
    */
   private String textureDataUrl(UUID guid)
   {
      if ((guid == null) || guid.equals(NXCommon.EMPTY_GUID))
         return null;

      LibraryImage image = ImageProvider.getInstance().getNativeImage(guid);
      if ((image == null) || !image.isComplete() || (image.getBinaryData() == null))
         return null;

      return "data:" + image.getMimeType() + ";base64," + Base64.getEncoder().encodeToString(image.getBinaryData());
   }

   /**
    * Status color of a hardware entity as a CSS "#rrggbb" string, so the JS layer
    * does not need any NetXMS status logic.
    *
    * @param status object status
    * @return CSS color string
    */
   private static String statusColor(ObjectStatus status)
   {
      return colorToHex(StatusDisplayInfo.getStatusColor(status));
   }

   /**
    * Convert an SWT color to a CSS "#rrggbb" string.
    *
    * @param c color
    * @return CSS color string
    */
   private static String colorToHex(Color c)
   {
      return String.format("#%02x%02x%02x", c.getRed(), c.getGreen(), c.getBlue());
   }

   /**
    * Load the page template and inline the Three.js library. Inlining is required
    * because {@link Browser#setText(String)} renders without a base URL, so an
    * external &lt;script src&gt; would not resolve.
    *
    * @return assembled HTML document
    */
   private String loadPageTemplate()
   {
      String template = readResource(TEMPLATE_RESOURCE);
      if (template == null)
         return "<html><body>3D rack view template not found</body></html>";

      String threejs = readResource(THREEJS_RESOURCE);
      if (threejs == null)
      {
         logger.warn("Three.js library resource {} not found - 3D rack view will not render", THREEJS_RESOURCE);
         threejs = "";
      }
      // Seed the page background from the theme so there is no dark flash before
      // the scene (which carries the same color) is pushed.
      return template
            .replace(THREEJS_MARKER, threejs)
            .replace(BG_COLOR_MARKER, colorToHex(ThemeEngine.getBackgroundColor("Rack")));
   }

   /**
    * Read a classpath resource as a UTF-8 string.
    *
    * @param path absolute resource path
    * @return resource content or null on error
    */
   private String readResource(String path)
   {
      try(InputStream in = getClass().getResourceAsStream(path))
      {
         if (in == null)
            return null;
         byte[] data = readAllBytes(in);
         return new String(data, StandardCharsets.UTF_8);
      }
      catch(IOException e)
      {
         logger.error("Cannot read resource {}", path, e);
         return null;
      }
   }

   /**
    * Read all bytes from a stream (avoids JDK9+ InputStream.readAllBytes() to keep
    * the source within the project's baseline language level).
    */
   private static byte[] readAllBytes(InputStream in) throws IOException
   {
      ByteArrayOutputStream buffer = new ByteArrayOutputStream();
      byte[] chunk = new byte[8192];
      int n;
      while((n = in.read(chunk)) != -1)
         buffer.write(chunk, 0, n);
      return buffer.toByteArray();
   }

   /**
    * Wrap a string as a single-quoted JavaScript string literal with escaping.
    *
    * @param s input string (typically JSON)
    * @return quoted JS literal
    */
   private static String toJsStringLiteral(String s)
   {
      StringBuilder sb = new StringBuilder(s.length() + 16);
      sb.append('\'');
      for(int i = 0; i < s.length(); i++)
      {
         char c = s.charAt(i);
         switch(c)
         {
            case '\\':
               sb.append("\\\\");
               break;
            case '\'':
               sb.append("\\'");
               break;
            case '\n':
               sb.append("\\n");
               break;
            case '\r':
               sb.append("\\r");
               break;
            case '\u2028': // line separator - breaks JS string literals
               sb.append("\\u2028");
               break;
            case '\u2029': // paragraph separator - breaks JS string literals
               sb.append("\\u2029");
               break;
            default:
               sb.append(c);
               break;
         }
      }
      sb.append('\'');
      return sb.toString();
   }

   /**
    * Add selection listener.
    *
    * @param listener listener to add
    */
   public void addSelectionListener(ElementSelectionListener listener)
   {
      selectionListeners.add(listener);
   }

   /**
    * Remove selection listener.
    *
    * @param listener listener to remove
    */
   public void removeSelectionListener(ElementSelectionListener listener)
   {
      selectionListeners.remove(listener);
   }

   /**
    * Scene description passed to the JS layer as JSON. Field names are part of the
    * Java/JS contract and are consumed verbatim in rack3d.html.
    */
   @SuppressWarnings("unused")
   private static class Scene
   {
      int height;
      boolean topBottomNumbering;
      boolean frontOnly;
      String bgColor; // theme background color "#rrggbb"
      String fgColor; // theme foreground/text color "#rrggbb"
      List<Unit> units = new ArrayList<Unit>();
   }

   /**
    * Single rack-mounted element (active device or passive element) in the scene.
    */
   @SuppressWarnings("unused")
   private static class Unit
   {
      long id;          // object id for active devices; passive-element id for passive (see passive flag)
      String name;
      int position;     // bottom-most occupied unit
      int height;       // height in units
      String side;      // "FRONT", "REAR" or "FILL"
      String statusColor;
      String frontImage; // image library GUID or null
      String rearImage;  // image library GUID or null
      boolean passive;
      String type;       // passive element type ("PATCH_PANEL", ...) or null for active devices
      int status;        // ObjectStatus ordinal (0=normal..4=critical); 0 for passive elements

      static Unit from(HardwareEntity n)
      {
         Unit u = new Unit();
         u.id = n.getObjectId();
         u.name = n.getObjectName();
         u.position = n.getRackPosition();
         u.height = n.getRackHeight();
         u.side = n.getRackOrientation().toString();
         u.statusColor = statusColor(n.getStatus());
         u.frontImage = guidToString(n.getFrontRackImage());
         u.rearImage = guidToString(n.getRearRackImage());
         u.passive = false;
         u.type = null;
         u.status = n.getStatus().getValue();
         return u;
      }

      static Unit from(PassiveRackElement p)
      {
         Unit u = new Unit();
         u.id = p.getId();
         u.name = p.getName();
         u.position = p.getPosition();
         u.height = p.getHeight();
         u.side = (p.getOrientation() != null) ? p.getOrientation().toString() : RackOrientation.FILL.toString();
         u.statusColor = "#808080"; // passive elements have no status
         u.frontImage = guidToString(p.getFrontImage());
         u.rearImage = guidToString(p.getRearImage());
         u.passive = true;
         u.type = (p.getType() != null) ? p.getType().toString() : null;
         u.status = 0;
         return u;
      }

      private static String guidToString(UUID guid)
      {
         return ((guid == null) || guid.equals(NXCommon.EMPTY_GUID)) ? null : guid.toString();
      }
   }
}
