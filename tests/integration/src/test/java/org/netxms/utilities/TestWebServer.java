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
package org.netxms.utilities;

import java.io.IOException;
import java.io.OutputStream;
import java.net.InetAddress;
import java.net.InetSocketAddress;
import java.net.Socket;
import java.nio.charset.StandardCharsets;
import org.netxms.tests.TestConstants;
import com.sun.net.httpserver.HttpExchange;
import com.sun.net.httpserver.HttpServer;

/**
 * Minimal HTTP server used as a target for web service tests, so that tests do not depend on third-party web sites. The server is
 * bound to the local address through which the test host reaches the NetXMS server, because requests are made by the agent running
 * on the management node, not by the test itself.
 *
 * Endpoints:
 * <ul>
 * <li>{@code /document/...} - responds with 200 and JSON document {@code {"path":"<request path>"}}</li>
 * <li>{@code /redirect} - responds with 302 and absolute {@code Location} pointing to {@code /document/redirected}</li>
 * <li>anything else - responds with 404</li>
 * </ul>
 */
public class TestWebServer implements AutoCloseable
{
   public static final String DOCUMENT_PATH = "/document/";
   public static final String REDIRECT_PATH = "/redirect";
   public static final String REDIRECT_TARGET_PATH = DOCUMENT_PATH + "redirected";

   private final HttpServer server;
   private final String baseUrl;

   /**
    * Start server on ephemeral port.
    *
    * @throws IOException if server cannot be started
    */
   public TestWebServer() throws IOException
   {
      InetAddress bindAddress;
      try (Socket probe = new Socket(TestConstants.SERVER_ADDRESS, TestConstants.SERVER_PORT_CLIENT))
      {
         bindAddress = probe.getLocalAddress();
      }

      server = HttpServer.create(new InetSocketAddress(bindAddress, 0), 0);
      baseUrl = "http://" + bindAddress.getHostAddress() + ":" + server.getAddress().getPort();

      server.createContext(DOCUMENT_PATH, exchange -> sendResponse(exchange, 200, documentFor(exchange.getRequestURI().getPath())));
      server.createContext(REDIRECT_PATH, exchange -> {
         exchange.getResponseHeaders().set("Location", baseUrl + REDIRECT_TARGET_PATH);
         sendResponse(exchange, 302, null);
      });
      server.createContext("/", exchange -> sendResponse(exchange, 404, null));
      server.start();
   }

   /**
    * Get base URL of this server (scheme, host, and port without trailing slash).
    *
    * @return base URL
    */
   public String getBaseUrl()
   {
      return baseUrl;
   }

   /**
    * Get document returned for given request path.
    *
    * @param path request path (starting with /)
    * @return document text
    */
   public static String documentFor(String path)
   {
      return "{\"path\":\"" + path + "\"}";
   }

   /**
    * Send HTTP response.
    *
    * @param exchange HTTP exchange
    * @param code response code
    * @param body response body or null
    * @throws IOException on I/O error
    */
   private static void sendResponse(HttpExchange exchange, int code, String body) throws IOException
   {
      if (body == null)
      {
         exchange.sendResponseHeaders(code, -1);
      }
      else
      {
         byte[] bytes = body.getBytes(StandardCharsets.UTF_8);
         exchange.getResponseHeaders().set("Content-Type", "application/json");
         exchange.sendResponseHeaders(code, bytes.length);
         try (OutputStream out = exchange.getResponseBody())
         {
            out.write(bytes);
         }
      }
      exchange.close();
   }

   /**
    * @see java.lang.AutoCloseable#close()
    */
   @Override
   public void close()
   {
      server.stop(0);
   }
}
