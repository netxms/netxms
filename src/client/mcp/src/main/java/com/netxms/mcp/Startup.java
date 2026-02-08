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

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import org.netxms.base.VersionInfo;
import org.netxms.client.NXCException;
import org.netxms.client.NXCSession;
import org.netxms.client.ai.AiAssistantFunction;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import com.fasterxml.jackson.databind.ObjectMapper;
import com.netxms.mcp.docbook.DocBookArticle;
import com.netxms.mcp.docbook.DocBookGetArticle;
import com.netxms.mcp.docbook.DocBookIndex;
import com.netxms.mcp.docbook.DocBookLoader;
import com.netxms.mcp.docbook.DocBookSearch;
import com.netxms.mcp.resources.ServerResource;
import com.netxms.mcp.tools.Alarms;
import com.netxms.mcp.tools.ConnectedNodes;
import com.netxms.mcp.tools.EnterMaintenance;
import com.netxms.mcp.tools.ExecuteScript;
import com.netxms.mcp.tools.FindConnectionPoint;
import com.netxms.mcp.tools.FindMACAddress;
import com.netxms.mcp.tools.GetLibraryScript;
import com.netxms.mcp.tools.GetMetricHistory;
import com.netxms.mcp.tools.GetServerStats;
import com.netxms.mcp.tools.HardwareInventory;
import com.netxms.mcp.tools.InterfaceList;
import com.netxms.mcp.tools.LeaveMaintenance;
import com.netxms.mcp.tools.ListLibraryScripts;
import com.netxms.mcp.tools.MetricList;
import com.netxms.mcp.tools.ObjectDetails;
import com.netxms.mcp.tools.ObjectList;
import com.netxms.mcp.tools.ProcessList;
import com.netxms.mcp.tools.RenameObject;
import com.netxms.mcp.tools.ServerTool;
import com.netxms.mcp.tools.ServerVersion;
import com.netxms.mcp.tools.SetObjectManagedState;
import com.netxms.mcp.tools.SoftwareInventory;
import io.modelcontextprotocol.json.McpJsonMapper;
import io.modelcontextprotocol.server.McpServer;
import io.modelcontextprotocol.server.McpServerFeatures.SyncResourceSpecification;
import io.modelcontextprotocol.server.McpServerFeatures.SyncToolSpecification;
import io.modelcontextprotocol.server.transport.HttpServletSseServerTransportProvider;
import io.modelcontextprotocol.server.transport.StdioServerTransportProvider;
import io.modelcontextprotocol.spec.McpSchema;
import io.modelcontextprotocol.spec.McpSchema.CallToolResult;
import io.modelcontextprotocol.spec.McpSchema.JsonSchema;
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
   private static String mainDocBookPath = null;
   private static String nxslDocBookFilePath = null;
   private static List<DocBookArticle> mainDocBookArticles = new ArrayList<>();
   private static List<DocBookArticle> nxslDocBookArticles = new ArrayList<>();

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
            System.out.println("  -help, --help          Show this help message");
            System.out.println("  -stdio, --stdio        Start server with stdio transport");
            System.out.println("  -docbook=<path>        Load product DocBook XML files as knowledge base");
            System.out.println("  -nxsl-docbook=<file>   Load NXSL DocBook XML file as knowledge base (single file)");
            return;
         }
         if (arg.startsWith("-docbook="))
         {
            int p = arg.indexOf('=');
            if (p > 0)
               mainDocBookPath = arg.substring(p + 1);
         }
         else if (arg.startsWith("-nxsl-docbook="))
         {
            int p = arg.indexOf('=');
            if (p > 0)
               nxslDocBookFilePath = arg.substring(p + 1);
         }
      }

      session = connectToServer(args);
      if (session == null)
      {
         logger.error("Failed to connect to NetXMS server. Exiting.");
         return;
      }

      // Load DocBook (optional)
      if (mainDocBookPath != null)
      {
         try
         {
            Path f = Path.of(mainDocBookPath);
            if (Files.isDirectory(f))
            {
               mainDocBookArticles = DocBookLoader.loadFromDirectory(f);
               logger.info("Loaded {} DocBook knowledge articles from {}", mainDocBookArticles.size(), f.toAbsolutePath());
            }
            else
            {
               logger.warn("DocBook path not found: {}", f.toAbsolutePath());
            }
         }
         catch(Exception e)
         {
            logger.error("Failed to load DocBook directory {} ({})", mainDocBookPath, e.getMessage());
         }
      }

      if (nxslDocBookFilePath != null)
      {
         try
         {
            Path f = Path.of(nxslDocBookFilePath);
            if (Files.isRegularFile(f))
            {
               nxslDocBookArticles = DocBookLoader.load(f);
               logger.info("Loaded {} DocBook knowledge articles from {}", nxslDocBookArticles.size(), f.toAbsolutePath());
            }
            else
            {
               logger.warn("DocBook file not found: {}", f.toAbsolutePath());
            }
         }
         catch(Exception e)
         {
            logger.error("Failed to load DocBook file {} ({})", nxslDocBookFilePath, e.getMessage());
         }
      }

      try
      {
         logger.info("MCP server is starting");
         McpSchema.ServerCapabilities serverCapabilities = McpSchema.ServerCapabilities.builder().tools(true).resources(true, true).build();
         McpServerTransportProvider transport = createTransportProvider(args);
         McpServer.sync(transport)
               .serverInfo("NetXMS MCP Server", VersionInfo.version())
               .capabilities(serverCapabilities)
               .tools(createTools())
               .resources(createResources())
               .build();
         logger.info("MCP server has stopped");
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
      registerTool(tools, new ConnectedNodes());
      registerTool(tools, new EnterMaintenance());
      registerTool(tools, new ExecuteScript());
      registerTool(tools, new FindConnectionPoint());
      registerTool(tools, new FindMACAddress());
      registerTool(tools, new GetLibraryScript());
      registerTool(tools, new GetMetricHistory());
      registerTool(tools, new GetServerStats());
      registerTool(tools, new HardwareInventory());
      registerTool(tools, new InterfaceList());
      registerTool(tools, new MetricList());
      registerTool(tools, new LeaveMaintenance());
      registerTool(tools, new ListLibraryScripts());
      registerTool(tools, new ObjectDetails());
      registerTool(tools, new ObjectList());
      registerTool(tools, new ProcessList());
      registerTool(tools, new RenameObject());
      registerTool(tools, new ServerVersion());
      registerTool(tools, new SetObjectManagedState());
      registerTool(tools, new SoftwareInventory());
      if (!mainDocBookArticles.isEmpty())
      {
         registerTool(tools, new DocBookSearch(mainDocBookArticles, "netxms", "NetXMS concepts and operation"));
         registerTool(tools, new DocBookGetArticle(mainDocBookArticles, "netxms", "NetXMS concepts and operation"));
      }
      if (!nxslDocBookArticles.isEmpty())
      {
         registerTool(tools, new DocBookSearch(nxslDocBookArticles, "nxsl", "NetXMS Scripting Language (NXSL)"));
         registerTool(tools, new DocBookGetArticle(nxslDocBookArticles, "nxsl", "NetXMS Scripting Language (NXSL)"));
      }

      try
      {
         List<AiAssistantFunction> serverFunctions = session.getAiAssistantFunctions();
         logger.info("Registering {} AI assistant functions from server as tools", serverFunctions.size());
         for(final AiAssistantFunction f : serverFunctions)
         {
            registerTool(tools, new ServerTool() {
               @Override
               public String getName()
               {
                  return "server_" + f.getName();
               }

               @Override
               public String getDescription()
               {
                  return f.getDescription();
               }

               @Override
               public String getSchema()
               {
                  return f.getSchema();
               }

               @Override
               public String execute(Map<String, Object> args) throws Exception
               {
                  ObjectMapper mapper = new ObjectMapper();
                  String json = mapper.writeValueAsString(args);
                  return session.callAiAssistantFunction(f.getName(), json);
               }
            });
         }
      }
      catch(Exception e)
      {
         logger.error("Cannot get AI assistant functions from server: {}", e.getMessage());
      }

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
      ObjectMapper mapper = new ObjectMapper();
      try
      {
         JsonSchema schema = mapper.createParser(tool.getSchema()).readValueAs(JsonSchema.class);
         tools.add(SyncToolSpecification.builder()
            .tool(Tool.builder()
               .name(tool.getName())
               .description(tool.getDescription())
               .inputSchema(schema)
               .build())
            .callHandler((exchange, args) -> {
               try
               {
                  String output = tool.execute(args.arguments());
                  return new CallToolResult(output, Boolean.FALSE);
               }
               catch(Exception e)
               {
                  logger.error("Error executing tool " + tool.getName() + ": " + e.getMessage(), e);
                  return new CallToolResult((e instanceof NXCException) ? e.getMessage() : "Exception: " + e.getClass().getName() + "; Message: " + e.getMessage(), Boolean.TRUE);
               }
            })
            .build());
      }
      catch(Exception e)
      {
         logger.error("Cannot parse schema for tool " + tool.getName() + ": " + e.getMessage(), e);
      }
   }

   /**
    * Create resources for MCP server.
    *
    * @return tools instance
    */
   private static List<SyncResourceSpecification> createResources()
   {
      List<SyncResourceSpecification> resources = new ArrayList<>();
      if (!mainDocBookArticles.isEmpty())
      {
         // Index first for discovery
         registerResource(resources, new DocBookIndex(mainDocBookArticles));
         // Individual articles
         for(DocBookArticle a : mainDocBookArticles)
            registerResource(resources, a);
      }
      if (!nxslDocBookArticles.isEmpty())
      {
         // Index first for discovery
         registerResource(resources, new DocBookIndex(nxslDocBookArticles));
         // Individual articles
         for(DocBookArticle a : nxslDocBookArticles)
            registerResource(resources, a);
      }
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
         new SyncResourceSpecification(Resource.builder()
               .uri(resource.getUri())
               .name(resource.getName())
               .description(resource.getDescription())
               .mimeType(resource.getMimeType())
               .build(),
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
            return new StdioServerTransportProvider(McpJsonMapper.createDefault());
         }
      }
      return HttpServletSseServerTransportProvider.builder()
            .jsonMapper(McpJsonMapper.createDefault())
            .messageEndpoint("/msg")
            .sseEndpoint("/sse")
            .build();
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