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
package org.netxms.nxmc.modules.objects.views;

import java.awt.image.BufferedImage;
import java.awt.image.DirectColorModel;
import java.awt.image.IndexColorModel;
import java.awt.image.WritableRaster;
import java.util.HashMap;
import java.util.Map;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.window.Window;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.ScrolledComposite;
import org.eclipse.swt.events.ControlAdapter;
import org.eclipse.swt.events.ControlEvent;
import org.eclipse.swt.events.KeyEvent;
import org.eclipse.swt.events.KeyListener;
import org.eclipse.swt.events.MouseEvent;
import org.eclipse.swt.events.MouseListener;
import org.eclipse.swt.events.MouseMoveListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.Cursor;
import org.eclipse.swt.graphics.Image;
import org.eclipse.swt.graphics.ImageData;
import org.eclipse.swt.graphics.PaletteData;
import org.eclipse.swt.graphics.RGB;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Label;
import org.netxms.client.NXCSession;
import org.netxms.client.objects.AbstractNode;
import org.netxms.client.objects.Node;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.dialogs.PasswordRequestDialog;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.objecttools.TcpPortForwarder;
import org.netxms.nxmc.resources.ResourceManager;
import org.netxms.nxmc.tools.WidgetHelper;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import org.xnap.commons.i18n.I18n;
import com.shinyhut.vernacular.client.VernacularClient;
import com.shinyhut.vernacular.client.VernacularConfig;
import com.shinyhut.vernacular.client.rendering.ColorDepth;

/**
 * Remote control view (embedded VNC viewer)
 */
public class RemoteControlView extends AdHocObjectView
{
   private static final Logger logger = LoggerFactory.getLogger(RemoteControlView.class);

   private static final Map<Integer, Integer> KEYMAP = new HashMap<>();
   static
   {
      // SWT to VNC key mapping according to https://cgit.freedesktop.org/xorg/proto/x11proto/plain/keysymdef.h
      KEYMAP.put(8, 0xff08);
      KEYMAP.put(9, 0xff09);
      KEYMAP.put(13, 0xff0d);
      KEYMAP.put(27, 0xff1b);
      KEYMAP.put(127, 0xffff);
      KEYMAP.put(SWT.ALT, 0xffe9);
      KEYMAP.put(SWT.ALT_GR, 0xffea);
      KEYMAP.put(SWT.ARROW_DOWN, 0xff54);
      KEYMAP.put(SWT.ARROW_LEFT, 0xff51);
      KEYMAP.put(SWT.ARROW_RIGHT, 0xff53);
      KEYMAP.put(SWT.ARROW_UP, 0xff52);
      KEYMAP.put(SWT.CTRL, 0xffe3);
      KEYMAP.put(SWT.END, 0xff57);
      KEYMAP.put(SWT.F1, 0xffbe);
      KEYMAP.put(SWT.F2, 0xffbf);
      KEYMAP.put(SWT.F3, 0xffc0);
      KEYMAP.put(SWT.F4, 0xffc1);
      KEYMAP.put(SWT.F5, 0xffc2);
      KEYMAP.put(SWT.F6, 0xffc3);
      KEYMAP.put(SWT.F7, 0xffc4);
      KEYMAP.put(SWT.F8, 0xffc5);
      KEYMAP.put(SWT.F9, 0xffc6);
      KEYMAP.put(SWT.F10, 0xffc7);
      KEYMAP.put(SWT.F11, 0xffc8);
      KEYMAP.put(SWT.F12, 0xffc9);
      KEYMAP.put(SWT.F13, 0xffca);
      KEYMAP.put(SWT.F14, 0xffcb);
      KEYMAP.put(SWT.F15, 0xffcc);
      KEYMAP.put(SWT.F16, 0xffcd);
      KEYMAP.put(SWT.F17, 0xffce);
      KEYMAP.put(SWT.F18, 0xffcf);
      KEYMAP.put(SWT.F19, 0xffd0);
      KEYMAP.put(SWT.F20, 0xffd1);
      KEYMAP.put(SWT.HOME, 0xff50);
      KEYMAP.put(SWT.INSERT, 0xff63);
      KEYMAP.put(SWT.PAGE_DOWN, 0xff56);
      KEYMAP.put(SWT.PAGE_UP, 0xff55);
      KEYMAP.put(SWT.PAUSE, 0xff13);
      KEYMAP.put(SWT.SHIFT, 0xffe1);
   }

   private final I18n i18n = LocalizationHelper.getI18n(RemoteControlView.class);

   private Label status;
   private ScrolledComposite scroller;
   private Canvas canvas;
   private VernacularClient vncClient;
   private ImageData lastFrame;
   private Exception lastError;
   private Cursor transparentCursor;
   private int screenWidth = 640;
   private int screenHeight = 480;
   private volatile boolean stopFlag = false;

   /**
    * Create VNC viewer
    *
    * @param node target node
    * @param contextId context ID
    */
   public RemoteControlView(AbstractNode node, long contextId)
   {
      super(LocalizationHelper.getI18n(ScreenshotView.class).tr("Remote Control"), ResourceManager.getImageDescriptor("icons/object-views/remote-desktop.png"), "objects.vncviewer", node.getObjectId(), contextId, false);
   }

   /**
    * Create VNC viewer
    */
   public RemoteControlView()
   {
      super(null, null, null, 0, 0, false);
   }

   /**
    * @see org.netxms.nxmc.base.views.View#createContent(org.eclipse.swt.widgets.Composite)
    */
   @Override
   protected void createContent(Composite parent)
   {
      // create a cursor with a transparent image
      PaletteData palette = new PaletteData(new RGB[] { new RGB(255, 255, 255), new RGB(0, 0, 0) });
      ImageData imageData = new ImageData(16, 16, 1, palette);
      imageData.transparentPixel = 0;
      transparentCursor = new Cursor(getDisplay(), imageData, 0, 0);

      GridLayout layout = new GridLayout();
      layout.marginHeight = 0;
      layout.marginWidth = 0;
      layout.verticalSpacing = 0;
      parent.setLayout(layout);

      // control bar 
      Composite controlBar = new Composite(parent, SWT.NONE);
      controlBar.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      layout = new GridLayout();
      layout.numColumns = 4;
      controlBar.setLayout(layout);
      
      status = new Label(controlBar, SWT.NONE);
      status.setText(i18n.tr("Disconnected"));
      status.setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, true));

      addKeyCombinationButton(controlBar, "Ctrl+Alt+Tab", new int[] { 0xffe3, 0xffe9, 0xff09 });
      addKeyCombinationButton(controlBar, "Ctrl+Esc", new int[] { 0xffe3, 0xff1b });
      addKeyCombinationButton(controlBar, "Ctrl+Alt+Del", new int[] { 0xffe3, 0xffe9, 0xffff });

      // separator 
      new Label(parent, SWT.SEPARATOR | SWT.HORIZONTAL).setLayoutData(new GridData(SWT.FILL, SWT.CENTER, true, false));

      // screen content 
      scroller = new ScrolledComposite(parent, SWT.H_SCROLL | SWT.V_SCROLL);
      scroller.setLayoutData(new GridData(SWT.FILL, SWT.FILL, true, true));
      scroller.setBackground(getDisplay().getSystemColor(SWT.COLOR_GRAY));
      WidgetHelper.setScrollBarIncrement(scroller, SWT.HORIZONTAL, 20);
      WidgetHelper.setScrollBarIncrement(scroller, SWT.VERTICAL, 20);

      canvas = new Canvas(scroller, SWT.NONE);
      canvas.setSize(screenWidth, screenHeight);
      scroller.setContent(canvas);
      canvas.setVisible(false);

      scroller.addControlListener(new ControlAdapter() {
         public void controlResized(ControlEvent e)
         {
            scroller.setMinSize(canvas.getSize());
         }
      });

      canvas.addPaintListener((e) -> {
         if (lastFrame != null)
            e.gc.drawImage(new Image(getDisplay(), lastFrame), 0, 0);
      });

      canvas.addMouseListener(new MouseListener() {
         @Override
         public void mouseUp(MouseEvent e)
         {
            if ((vncClient != null) && vncClient.isRunning())
               vncClient.updateMouseButton(e.button, false);
         }

         @Override
         public void mouseDown(MouseEvent e)
         {
            if ((vncClient != null) && vncClient.isRunning())
               vncClient.updateMouseButton(e.button, true);
         }

         @Override
         public void mouseDoubleClick(MouseEvent e)
         {
         }
      });

      canvas.addMouseMoveListener(new MouseMoveListener() {
         @Override
         public void mouseMove(MouseEvent e)
         {
            if ((vncClient != null) && vncClient.isRunning())
               vncClient.moveMouse(e.x, e.y);
         }
      });

      canvas.addKeyListener(new KeyListener() {
         @Override
         public void keyReleased(KeyEvent e)
         {
            sendKeyUpdate(e.keyCode, false);
         }

         @Override
         public void keyPressed(KeyEvent e)
         {
            sendKeyUpdate(e.keyCode, true);
         }
      });

      connect();
   }

   /**
    * Add button for sending key combination to remote machine.
    *
    * @param parent parent composite
    * @param text button text
    * @param keys sequence of keys to be emulated
    */
   private void addKeyCombinationButton(Composite parent, String text, final int[] keys)
   {
      Button b = new Button(parent, SWT.PUSH);
      b.setText(text);
      b.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            for(int i = 0; i < keys.length; i++)
               vncClient.updateKey(keys[i], true);
            for(int i = keys.length - 1; i >= 0; i--)
               vncClient.updateKey(keys[i], false);
         }
      });

      GridData gd = new GridData();
      gd.verticalAlignment = SWT.CENTER;
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      b.setLayoutData(gd);
   }

   /**
    * Connect to remote system
    */
   private void connect()
   {
      status.setText(i18n.tr("Connecting..."));

      final Node node = (Node)getObject();
      final NXCSession session = Registry.getSession();

      VernacularConfig config = new VernacularConfig();
      config.setColorDepth(ColorDepth.BPP_24_TRUE);
      config.setErrorListener((e) -> {
         lastError = e;
         logger.debug("VNC error", e);
      });
      config.setRemoteClipboardListener(text -> getDisplay().asyncExec(() -> WidgetHelper.copyToClipboard(text)));

      config.setPasswordSupplier(() -> {
         String password = node.getVncPassword();
         if (password.isEmpty())
         {
            final String[] result = new String[1];
            getDisplay().syncExec(() -> {
               PasswordRequestDialog dlg = new PasswordRequestDialog(getWindow().getShell(), i18n.tr("Password Request"), i18n.tr("Remote system requires password authentication"));
               if (dlg.open() == Window.OK)
               {
                  result[0] = dlg.getPassword();
               }
            });
            if (result[0] != null)
               password = result[0];
         }
         return password;
      });

      config.setScreenUpdateListener(image -> {
         final ImageData fb = convertToSWT((BufferedImage)image);
         getDisplay().asyncExec(() -> updateScreen(fb));
      });

      vncClient = new VernacularClient(config);
      lastError = null;
      Job job = new Job(i18n.tr("Connecting to VNC server"), this) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            TcpPortForwarder tcpPortForwarder = new TcpPortForwarder(session, node.getObjectId(), (node.getCapabilities() & AbstractNode.NC_IS_LOCAL_VNC) != 0, 5900, 0);
            tcpPortForwarder.setDisplay(getDisplay());
            tcpPortForwarder.setMessageArea(RemoteControlView.this);
            tcpPortForwarder.run();

            logger.debug("Establishing VNC session with node {}", node.getObjectName());
            vncClient.start("127.0.0.1", tcpPortForwarder.getLocalPort());
            if (!vncClient.isRunning())
            {
               logger.debug("Cannot establish VNC session with node {}", node.getObjectName());
               if (lastError != null)
                  throw new Exception(lastError.getMessage(), lastError);
               else
                  throw new Exception("Connection error");
            }

            logger.debug("VNC client started for node {}", node.getObjectName());
            runInUIThread(() -> {
               status.setText(i18n.tr("Connected | {0}x{1}", screenWidth, screenHeight));
               canvas.setCursor(transparentCursor);
               canvas.setVisible(true);
            });

            try
            {
               while(!stopFlag)
               {
                  Thread.sleep(1000);
               }
            }
            finally
            {
               vncClient.stop();
               vncClient = null;
               tcpPortForwarder.close();
               logger.debug("VNC client stopped for node {}", node.getObjectName());

               runInUIThread(() -> {
                  if (!status.isDisposed())
                  {
                     status.setText(i18n.tr("Disconnected"));
                  }
                  if (!canvas.isDisposed())
                  {
                     canvas.setCursor(null);
                     canvas.setVisible(false);
                  }
               });
            }
         }

         /**
          * @see org.netxms.nxmc.base.jobs.Job#jobFailureHandler(java.lang.Exception)
          */
         @Override
         protected void jobFailureHandler(Exception e)
         {
            runInUIThread(() -> status.setText(i18n.tr("Connection failed")));
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("VNC connection error");
         }
      };
      job.setUser(false);
      job.start();
   }

   /**
    * Send key update to remote machine.
    *
    * @param keyCode SWT key code
    * @param pressed true if key was pressed
    */
   private void sendKeyUpdate(int keyCode, boolean pressed)
   {
      if ((vncClient == null) || !vncClient.isRunning())
         return;

      int vncKeyCode;
      if ((keyCode >= 32) && (keyCode < 127))
      {
         vncKeyCode = keyCode;
      }
      else
      {
         Integer k = KEYMAP.get(keyCode);
         if (k == null)
            return; // Unsupported key
         vncKeyCode = k;
      }
      vncClient.updateKey(vncKeyCode, pressed);
   }

   /**
    * Update screen from remote machine.
    *
    * @param fb last known framebuffer
    */
   private void updateScreen(ImageData fb)
   {
      if (canvas.isDisposed())
         return;

      if ((screenWidth != fb.width) || (screenHeight != fb.height))
      {
         screenWidth = fb.width;
         screenHeight = fb.height;
         canvas.setSize(screenWidth, screenHeight);
         scroller.setMinSize(canvas.getSize());
         status.setText(i18n.tr("Connected | {0} x {1}", Integer.toString(screenWidth), Integer.toString(screenHeight)));
      }

      lastFrame = fb;
      canvas.redraw();
   }

   /**
    * @see org.netxms.nxmc.modules.objects.views.ObjectView#dispose()
    */
   @Override
   public void dispose()
   {
      stopFlag = true;
      if (!canvas.isDisposed())
         canvas.setCursor(null);
      transparentCursor.dispose();
      super.dispose();
   }

   /**
    * Convert AWT buffered image to SWT image data
    *
    * @param bufferedImage AWT buffered image
    * @return SWT image data
    */
   private static ImageData convertToSWT(BufferedImage bufferedImage)
   {
      if (bufferedImage.getColorModel() instanceof DirectColorModel)
      {
         DirectColorModel colorModel = (DirectColorModel)bufferedImage.getColorModel();
         PaletteData palette = new PaletteData(colorModel.getRedMask(), colorModel.getGreenMask(), colorModel.getBlueMask());
         ImageData data = new ImageData(bufferedImage.getWidth(), bufferedImage.getHeight(), colorModel.getPixelSize(), palette);
         WritableRaster raster = bufferedImage.getRaster();
         int[] pixelArray = new int[3];
         for(int y = 0; y < data.height; y++)
         {
            for(int x = 0; x < data.width; x++)
            {
               raster.getPixel(x, y, pixelArray);
               int pixel = palette.getPixel(new RGB(pixelArray[0], pixelArray[1], pixelArray[2]));
               data.setPixel(x, y, pixel);
            }
         }
         return data;
      }
      else if (bufferedImage.getColorModel() instanceof IndexColorModel)
      {
         IndexColorModel colorModel = (IndexColorModel)bufferedImage.getColorModel();
         int size = colorModel.getMapSize();
         byte[] reds = new byte[size];
         byte[] greens = new byte[size];
         byte[] blues = new byte[size];
         colorModel.getReds(reds);
         colorModel.getGreens(greens);
         colorModel.getBlues(blues);
         RGB[] rgbs = new RGB[size];
         for(int i = 0; i < rgbs.length; i++)
         {
            rgbs[i] = new RGB(reds[i] & 0xFF, greens[i] & 0xFF, blues[i] & 0xFF);
         }
         PaletteData palette = new PaletteData(rgbs);
         ImageData data = new ImageData(bufferedImage.getWidth(), bufferedImage.getHeight(), colorModel.getPixelSize(), palette);
         data.transparentPixel = colorModel.getTransparentPixel();
         WritableRaster raster = bufferedImage.getRaster();
         int[] pixelArray = new int[1];
         for(int y = 0; y < data.height; y++)
         {
            for(int x = 0; x < data.width; x++)
            {
               raster.getPixel(x, y, pixelArray);
               data.setPixel(x, y, pixelArray[0]);
            }
         }
         return data;
      }
      return null;
   }
}
