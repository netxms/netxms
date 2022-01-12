/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2022 Raden Solutions
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
package org.netxms.ui.eclipse.usermanager.dialogs.helpers;

import java.io.StringWriter;
import java.io.UnsupportedEncodingException;
import java.net.URLEncoder;
import java.security.SecureRandom;
import java.util.Random;
import org.apache.commons.codec.binary.Base32;
import org.eclipse.swt.SWT;
import org.eclipse.swt.events.ModifyEvent;
import org.eclipse.swt.events.ModifyListener;
import org.eclipse.swt.events.PaintEvent;
import org.eclipse.swt.events.PaintListener;
import org.eclipse.swt.events.SelectionAdapter;
import org.eclipse.swt.events.SelectionEvent;
import org.eclipse.swt.graphics.GC;
import org.eclipse.swt.graphics.Rectangle;
import org.eclipse.swt.layout.GridData;
import org.eclipse.swt.layout.GridLayout;
import org.eclipse.swt.widgets.Button;
import org.eclipse.swt.widgets.Canvas;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;
import org.netxms.client.NXCSession;
import org.netxms.ui.eclipse.shared.ConsoleSharedData;
import org.netxms.ui.eclipse.tools.RefreshTimer;
import org.netxms.ui.eclipse.tools.WidgetHelper;
import org.netxms.ui.eclipse.usermanager.Activator;
import org.netxms.ui.eclipse.widgets.LabeledText;
import org.simpleframework.xml.Element;
import org.simpleframework.xml.Path;
import org.simpleframework.xml.Root;
import org.simpleframework.xml.Serializer;
import org.simpleframework.xml.core.Persister;
import io.nayuki.qrcodegen.QrCode;

/**
 * 2FA method binding configurator for "TOTP" driver
 */
public class TOTPMethodBindingConfigurator extends AbstractMethodBindingConfigurator
{
   private LabeledText secret;
   private Canvas qrCodeDisplay;
   private RefreshTimer refreshTimer;

   /**
    * Create configurator.
    *
    * @param parent parent composite
    */
   public TOTPMethodBindingConfigurator(Composite parent)
   {
      super(parent);

      GridLayout layout = new GridLayout();
      layout.marginWidth = 0;
      layout.marginHeight = 0;
      layout.verticalSpacing = 10;
      layout.numColumns = 2;
      setLayout(layout);

      secret = new LabeledText(this, SWT.NONE);
      secret.setLabel("Secret");
      secret.setLayoutData(new GridData(SWT.FILL, SWT.BOTTOM, true, false));
      secret.getTextControl().addModifyListener(new ModifyListener() {
         @Override
         public void modifyText(ModifyEvent e)
         {
            refreshTimer.execute();
         }
      });

      Button generateButton = new Button(this, SWT.PUSH);
      generateButton.setText("&Generate");
      generateButton.addSelectionListener(new SelectionAdapter() {
         @Override
         public void widgetSelected(SelectionEvent e)
         {
            secret.setText(generateRandomString());
         }
      });
      GridData gd = new GridData();
      gd.widthHint = WidgetHelper.BUTTON_WIDTH_HINT;
      gd.verticalAlignment = SWT.BOTTOM;
      generateButton.setLayoutData(gd);

      qrCodeDisplay = new Canvas(this, SWT.BORDER);
      qrCodeDisplay.addPaintListener(new PaintListener() {
         @Override
         public void paintControl(PaintEvent e)
         {
            drawQrCode(e.gc, qrCodeDisplay.getClientArea());
         }
      });
      gd = new GridData(SWT.CENTER, SWT.CENTER, false, false);
      gd.widthHint = 320;
      gd.heightHint = 320;
      gd.horizontalSpan = 2;
      qrCodeDisplay.setLayoutData(gd);

      refreshTimer = new RefreshTimer(200, this, new Runnable() {
         @Override
         public void run()
         {
            qrCodeDisplay.redraw();
         }
      });
   }

   /**
    * Draw QR code using current secret
    * 
    * @param gc GC to draw on
    * @param clientArea canvas size
    */
   private void drawQrCode(GC gc, Rectangle clientArea)
   {
      NXCSession session = ConsoleSharedData.getSession();
      String issuer = urlEncode("NetXMS (" + session.getServerName() + ")");
      String uri = "otpauth://totp/" + issuer + ":" + session.getUserName() + "?issuer=" + issuer + "&secret=" + base32Encode(secret.getText().trim());
      QrCode qr = QrCode.encodeText(uri, QrCode.Ecc.MEDIUM);

      gc.setForeground(Display.getCurrent().getSystemColor(SWT.COLOR_WIDGET_FOREGROUND));
      float scale = (float)clientArea.width / (float)qr.size;
      for(int y = 0; y < clientArea.height; y++)
      {
         for(int x = 0; x < clientArea.width; x++)
         {
            if (qr.getModule((int)((float)x / scale), (int)((float)y / scale)))
               gc.drawPoint(x, y);
         }
      }
   }

   /**
    * URL encode string
    *
    * @param s string to encode
    * @return encoded string
    */
   private static String urlEncode(String s)
   {
      try
      {
         return URLEncoder.encode(s, "UTF-8").replaceAll("\\+", "%20");
      }
      catch(UnsupportedEncodingException e)
      {
         return s;
      }
   }

   /**
    * Encode characters from given string as base32
    * 
    * @param s string to encode
    * @return encoded string
    */
   private static String base32Encode(String s)
   {
      byte[] bytes;
      try
      {
         bytes = s.getBytes("UTF-8");
      }
      catch(UnsupportedEncodingException e)
      {
         bytes = s.getBytes();
      }
      return new Base32().encodeToString(bytes);
   }

   /**
    * Symbols for random string
    */
   private static final String SYMBOLS = "ABCDEFGJKLMNPRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

   /**
    * Generate random string
    *
    * @return random string
    */
   private static String generateRandomString()
   {
      Random random = new SecureRandom();
      char[] buffer = new char[32];
      for(int i = 0; i < buffer.length; i++)
         buffer[i] = SYMBOLS.charAt(random.nextInt(SYMBOLS.length()));
      return new String(buffer);
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#setConfiguration(java.lang.String)
    */
   @Override
   public void setConfiguration(String configuration)
   {
      Serializer serializer = new Persister();
      try
      {
         Configuration config = serializer.read(Configuration.class, configuration);
         secret.setText(config.secret);
         qrCodeDisplay.redraw();
      }
      catch(Exception e)
      {
         Activator.logError("Cannot parse 2FA method binding configuration", e);
      }
   }

   /**
    * @see org.netxms.ui.eclipse.usermanager.dialogs.helpers.AbstractMethodBindingConfigurator#getConfiguration()
    */
   @Override
   public String getConfiguration()
   {
      Configuration config = new Configuration();
      config.secret = secret.getText().trim();
      Serializer serializer = new Persister();
      StringWriter writer = new StringWriter();
      try
      {
         serializer.write(config, writer);
         return writer.toString();
      }
      catch(Exception e)
      {
         Activator.logError("Cannot serialize 2FA method binding configuration", e);
         return "";
      }
   }

   /**
    * Configuration for method binding
    */
   @Root(name = "config", strict = false)
   private static class Configuration
   {
      @Element(name = "Secret", required = false)
      @Path("MethodBinding")
      String secret;
   }
}
