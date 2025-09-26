/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2025 Reden Solutions
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
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
package com.netxms.mcp;

import java.util.ArrayList;
import java.util.List;
import org.netxms.base.VersionInfo;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.netxms.mcp.resources.ServerResource;
import com.netxms.mcp.tools.Alarms;
import com.netxms.mcp.tools.LastValues;
import com.netxms.mcp.tools.ObjectDetails;
import com.netxms.mcp.tools.ObjectList;
import com.netxms.mcp.tools.ProcessList;
import com.netxms.mcp.tools.RenameObject;
import com.netxms.mcp.tools.ServerTool;
import com.netxms.mcp.tools.ServerVersion;
import com.netxms.mcp.tools.SetObjectManagedState;
import com.netxms.mcp.tools.SoftwareInventory;
import io.modelcontextprotocol.server.McpServer;
import io.modelcontextprotocol.server.McpServerFeatures.SyncResourceSpecification;
import io.modelcontextprotocol.server.McpServerFeatures.SyncToolSpecification;
import io.modelcontextprotocol.server.McpSyncServer;
import io.modelcontextprotocol.server.transport.HttpServletSseServerTransportProvider;
import io.modelcontextprotocol.server.transport.StdioServerTransportProvider;
import io.modelcontextprotocol.spec.McpSchema;
import io.modelcontextprotocol.spec.McpSchema.CallToolResult;
import io.modelcontextprotocol.spec.McpSchema.ReadResourceResult;
import io.modelcontextprotocol.spec.McpSchema.Resource;
import io.modelcontextprotocol.spec.McpSchema.ResourceContents;
import io.modelcontextprotocol.spec.McpSchema.TextResourceContents;
import io.modelcontextprotocol.spec.McpSchema.Tool;
import io.modelcontextprotocol.spec.McpServerTransportProvider;

/**
 * MCP server startup code
 */
public class Startup
{
   private static final Logger logger = LoggerFactory.getLogger(Startup.class);

   private static NXCSession session = null;

   /**
    * Main entry point
    *
    * @param args command line arguments
    */
   public static void main(String[] args)
   {
      for(String arg : args)
      {
         if (arg.equalsIgnoreCase("-help") || arg.equalsIgnoreCase("--help"))
         {
            System.out.println("Usage: java -jar mcp-server.jar [options]");
            System.out.println("Options:");
            System.out.println("  -help, --help     Show this help message");
            System.out.println("  -stdio, --stdio   Start server with stdio transport");
            return;
         }
      }

      session = connectToServer(args);
      if (session == null)
      {
         logger.error("Failed to connect to NetXMS server. Exiting.");
         return;
      }

      try
      {
         McpSchema.ServerCapabilities serverCapabilities = McpSchema.ServerCapabilities.builder().tools(true).prompts(true).resources(true, true).build();
         McpServerTransportProvider transport = createTransportProvider(args);
         McpSyncServer server = McpServer.sync(transport)
               .serverInfo("NetXMS MCP Server", VersionInfo.version())
               .capabilities(serverCapabilities)
               .tools(createTools())
               .resources(createResources())
               .build();
         logger.info("MCP server started");
      }
      catch(Exception e)
      {
         logger.error("MCP server error", e);
      }
   }

   /**
    * Create tools for MCP server.
    *
    * @return tools instance
    */
   private static List<SyncToolSpecification> createTools()
   {
      List<SyncToolSpecification> tools = new ArrayList<>();
      registerTool(tools, new Alarms());
      registerTool(tools, new LastValues());
      registerTool(tools, new ObjectDetails());
      registerTool(tools, new ObjectList());
      registerTool(tools, new ProcessList());
      registerTool(tools, new RenameObject());
      registerTool(tools, new ServerVersion());
      registerTool(tools, new SetObjectManagedState());
      registerTool(tools, new SoftwareInventory());
      return tools;
   }

   /**
    * Register single tool.
    *
    * @param tools list of tools
    * @param tool tool to register
    */
   private static void registerTool(List<SyncToolSpecification> tools, ServerTool tool)
   {
      tools.add(
         new SyncToolSpecification(
            new Tool(tool.getName(), tool.getDescription(), tool.getSchema()),
            (exchange, args) -> {
               try
               {
                        String output = tool.execute(args);
                        return new CallToolResult(output, Boolean.FALSE);
               }
               catch(Exception e)
               {
                  logger.error("Error executing tool {}: {}", tool.getName(), e.getMessage());
                  return new CallToolResult((e instanceof NXCException) ? e.getMessage() : "Internal error", Boolean.TRUE);
               }
            }));
   }

   /**
    * Create resources for MCP server.
    *
    * @return tools instance
    */
   private static List<SyncResourceSpecification> createResources()
   {
      List<SyncResourceSpecification> resources = new ArrayList<>();
      registerResource(resources, new com.netxms.mcp.resources.ServerVersion());
      return resources;
   }

   /**
    * Register single resource.
    *
    * @param resources list of resources
    * @param resource resource to register
    */
   private static void registerResource(List<SyncResourceSpecification> resources, ServerResource resource)
   {
      resources.add(
         new SyncResourceSpecification(
            new Resource(resource.getUri(), resource.getName(), resource.getDescription(), resource.getMimeType(), null),
            (exchange, request) -> {
               try
               {
                  ResourceContents contents = new TextResourceContents(resource.getUri(), resource.getMimeType(), resource.execute(request));
                  return new ReadResourceResult(List.of(contents));
               }
               catch(Exception e)
               {
                  logger.error("Error executing resource {}: {}", resource.getName(), e.getMessage());
                  return null;
               }
            }));
   }

   /**
    * Create transport provider based on command line arguments.
    *
    * @param args command line arguments
    * @return transport provider instance or null if help was requested
    */
   private static McpServerTransportProvider createTransportProvider(String[] args)
   {
      for(String arg : args)
      {
         if (arg.equalsIgnoreCase("-stdio") || arg.equalsIgnoreCase("--stdio"))
         {
            logger.info("Starting MCP server with stdio transport");
            return new StdioServerTransportProvider();
         }
      }
      return new HttpServletSseServerTransportProvider(new ObjectMapper(), "/msg", "/sse");
   }

   /**
    * Connect to NetXMS server using provided command line arguments.
    *
    * @param args command line arguments
    * @return NXCSession instance or null if connection failed
    */
   private static NXCSession connectToServer(String[] args)
   {
      String serverAddress = "localhost"; // Default server address
      int serverPort = 4701; // Default server port
      String loginName = "admin"; // Default login name
      String password = ""; // Default password

      for(String arg : args)
      {
         if (arg.startsWith("-server="))
         {
            serverAddress = arg.substring(8);
         }
         else if (arg.startsWith("--server="))
         {
            serverAddress = arg.substring(9);
         }
         else if (arg.startsWith("-port="))
         {
            try
            {
               serverPort = Integer.parseInt(arg.substring(6));
            }
            catch(NumberFormatException e)
            {
               logger.error("Invalid port number: {}", arg.substring(6));
            }
         }
         else if (arg.startsWith("--port="))
         {
            try
            {
               serverPort = Integer.parseInt(arg.substring(7));
            }
            catch(NumberFormatException e)
            {
               logger.error("Invalid port number: {}", arg.substring(7));
            }
         }
         else if (arg.startsWith("-login="))
         {
            loginName = arg.substring(7);
         }
         else if (arg.startsWith("--login="))
         {
            loginName = arg.substring(8);
         }
         else if (arg.startsWith("-password="))
         {
            password = arg.substring(10);
         }
         else if (arg.startsWith("--password="))
         {
            password = arg.substring(11);
         }
      }

      NXCSession session = new NXCSession(serverAddress, serverPort);
      try
      {
         session.connect();
         session.login(loginName, password);
         logger.info("Connected to NetXMS server at {}:{} as {}", serverAddress, serverPort, loginName);
         session.syncObjects();
         AlarmManager.initialize(session);
         return session;
      }
      catch(Exception e)
      {
         logger.error("Failed to connect to NetXMS server at {}:{} as {} ({})", serverAddress, serverPort, loginName, e.getMessage());
         return null;
      }
   }

   /**
    * Get current communication session.
    *
    * @return current communication session
    */
   public static NXCSession getSession()
   {
      return session;
   }
}
