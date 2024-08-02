/**
 * OPC UA subagent
 * Copyright (C) 2017-2021 Raden Solutions
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
package com.radensolutions.netxms.agent.opcua;

import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.concurrent.Callable;
import java.util.concurrent.CompletableFuture;
import java.util.concurrent.ExecutionException;
import java.util.concurrent.TimeUnit;
import org.eclipse.milo.opcua.sdk.client.OpcUaClient;
import org.eclipse.milo.opcua.sdk.client.api.identity.AnonymousProvider;
import org.eclipse.milo.opcua.sdk.client.api.identity.UsernameProvider;
import org.eclipse.milo.opcua.sdk.client.nodes.UaVariableNode;
import org.eclipse.milo.opcua.stack.core.Identifiers;
import org.eclipse.milo.opcua.stack.core.StatusCodes;
import org.eclipse.milo.opcua.stack.core.UaRuntimeException;
import org.eclipse.milo.opcua.stack.core.UaServiceFaultException;
import org.eclipse.milo.opcua.stack.core.security.SecurityPolicy;
import org.eclipse.milo.opcua.stack.core.types.builtin.DataValue;
import org.eclipse.milo.opcua.stack.core.types.builtin.DateTime;
import org.eclipse.milo.opcua.stack.core.types.builtin.ExpandedNodeId;
import org.eclipse.milo.opcua.stack.core.types.builtin.ExtensionObject;
import org.eclipse.milo.opcua.stack.core.types.builtin.LocalizedText;
import org.eclipse.milo.opcua.stack.core.types.builtin.NodeId;
import org.eclipse.milo.opcua.stack.core.types.builtin.StatusCode;
import org.eclipse.milo.opcua.stack.core.types.builtin.Variant;
import org.eclipse.milo.opcua.stack.core.types.builtin.unsigned.UInteger;
import org.netxms.bridge.Platform;
import com.google.common.collect.ImmutableList;

/**
 * Represents OPC-UA server connection
 */
public class ServerConnection
{
   private String name;
   private String url;
   private String login;
   private String password;
   private int timeout;
   private OpcUaClient session;
   private Map<NodeId, ExpandedNodeId> dataTypeCache = new HashMap<>();

   /**
    * @param url
    * @param login
    * @param password
    */
   public ServerConnection(String name, String url, String login, String password, int timeout)
   {
      this.name = name;
      this.url = url;
      this.login = login;
      this.password = password;
      this.timeout = timeout;
   }
   
   /**
    * @return the name
    */
   public String getName()
   {
      return name;
   }

   /**
    * @return the url
    */
   public String getUrl()
   {
      return url;
   }
   
   /**
    * Connect to server
    * 
    * @throws Exception on any error
    */
   private void connect() throws Exception
   {
      if (session != null)
         return;

      try
      {
         session = OpcUaClient.create(url,
               endpoints ->
                  endpoints
                     .stream()
                     .filter(e -> e.getSecurityPolicyUri().equals(SecurityPolicy.None.getUri()))
                     .findFirst(),
               configBuilder ->
                  configBuilder
                     .setApplicationName(LocalizedText.english("NetXMS Agent"))
                     .setApplicationUri("urn:netxms:agent")
                     // .setCertificate(loader.getClientCertificate())
                     // .setKeyPair(loader.getClientKeyPair())
                     .setIdentityProvider((login != null) ? new UsernameProvider(login, password) : new AnonymousProvider())
                     .setRequestTimeout(UInteger.valueOf(5000))
                     .build());
         session.connect().get(timeout, TimeUnit.MILLISECONDS);
      }
      catch(Exception e)
      {
         Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 5, String.format("Cannot setup connection to OPC UA server %s (%s)", url, e.getMessage()));
         Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 5, "   ", e);
         throw e;
      }
   }

   /**
    * Disconnect from server
    */
   private void disconnect()
   {
      if (session == null)
         return;
      
      try
      {
         session.disconnect().get();
      }
      catch(Exception e)
      {
         Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, "Exception in disconnect() call for " + url);
         Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, "   ", e);
      }
      session = null;
   }

   /**
    * Handle execution exception during operation. Depending on exception cause
    * can do reconnect and execute provided action, or re-throw original exeption.
    * 
    * @param e original exception
    * @param action action to be executed if reconnect is possible and successful
    * @return result of action execution
    * @throws Exception in case of failure
    */
   private <T extends Object> T handleExecutionException(ExecutionException e, Callable<T> action) throws Exception
   {
      if (e.getCause() instanceof UaServiceFaultException)
      {
         UaServiceFaultException se = (UaServiceFaultException)e.getCause();
         disconnect();
         long status = se.getStatusCode().getValue();
         if ((status == StatusCodes.Bad_CommunicationError) ||
             (status == StatusCodes.Bad_ConnectionClosed) ||
             (status == StatusCodes.Bad_Disconnect) ||               
             (status == StatusCodes.Bad_EndOfStream) ||
             (status == StatusCodes.Bad_InvalidState) ||
             (status == StatusCodes.Bad_NoCommunication) ||
             (status == StatusCodes.Bad_NotConnected) ||
             (status == StatusCodes.Bad_SecureChannelClosed) ||
             (status == StatusCodes.Bad_SecureChannelIdInvalid) ||
             (status == StatusCodes.Bad_ServerNotConnected) ||
             (status == StatusCodes.Bad_SessionClosed) ||
             (status == StatusCodes.Bad_SessionIdInvalid) ||
             (status == StatusCodes.Bad_TcpInternalError) ||
             (status == StatusCodes.Bad_UnexpectedError))
         {
            Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, String.format("Reconnect after error (%s)", se.getStatusCode().toString()));
            connect();
            return action.call();
         }
      }
      throw e;
   }

   /**
    * Read value of given node from server
    * 
    * @param nodeId node ID
    * @return node value
    * @throws Exception
    */
   private String readNodeValue(NodeId nodeId) throws Exception
   {
      UaVariableNode node = session.getAddressSpace().getVariableNode(nodeId);
      Object value = node.readValue().getValue().getValue();
      if (value == null)
         return null;

      Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, String.format("readNodeValue(%s) for %s: type %s", nodeId.toString(), url, value.getClass().getCanonicalName()));

      if (value instanceof String[])
      {
         StringBuilder sb = new StringBuilder();
         for(String s : (String[])value)
         {
            if (sb.length() > 0)
               sb.append(' ');
            sb.append(s);
         }
         return sb.toString();
      }

      if (value instanceof DateTime)
      {
         return Long.toString(((DateTime)value).getJavaTime());
      }

      if (value instanceof ExtensionObject)
      {
         return ((ExtensionObject)value).getBody().toString();
      }

      return value.toString();
   }

   /**
    * Get value of given node, connecting to server as necessary
    * 
    * @param name node symbolic name
    * @return node value or null to indicate unsupported node
    * @throws Exception
    */
   public synchronized String getNodeValue(String name) throws Exception
   {
      connect();

      NodeId nodeId;
      try
      {
         nodeId = NodeId.parse(name);
      }
      catch(UaRuntimeException e)
      {
         Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, String.format("Error parsing node ID \"%s\"", name));
         return null;
      }

      try
      {
         return readNodeValue(nodeId);
      }
      catch(ExecutionException e)
      {
         return handleExecutionException(e, () -> { return readNodeValue(nodeId); });
      }
      catch(Exception e)
      {
         Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, String.format("Exception in getNodeValue(%s) call for %s (%s)", name, url, e.getMessage()));
         Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, "   ", e);
         return null;
      }
   }
   
   /**
    * @param nodeId
    * @param value
    * @return
    * @throws Exception
    */
   private boolean writeNodeValue(NodeId nodeId, String newValue) throws Exception
   {
      UaVariableNode node = session.getAddressSpace().getVariableNode(nodeId);
      
      // Determine data type
      ExpandedNodeId dataType = dataTypeCache.get(nodeId);
      if (dataType == null)
      {
         Variant value = node.readValue().getValue();
         dataType = value.getDataType().get();
         if (dataType == null)
         {
            Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, String.format("writeNodeValue(%s, \"%s\") for %s: cannot read data type", nodeId.toString(), value, url));
            return false;
         }
         Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, String.format("writeNodeValue(%s, \"%s\") for %s: type %s", nodeId.toString(), value, url, dataType.toString()));
         dataTypeCache.put(nodeId, dataType);
      }

      Variant v;
      if (Identifiers.Boolean.equalTo(dataType))
      {
         v = new Variant(Boolean.parseBoolean(newValue));
      }
      else if (Identifiers.Int32.equalTo(dataType) || Identifiers.UInt32.equalTo(dataType))
      {
         v = new Variant(Integer.parseInt(newValue));
      }
      else if (Identifiers.Int64.equalTo(dataType) || Identifiers.UInt64.equalTo(dataType))
      {
         v = new Variant(Long.parseLong(newValue));
      }
      else
      {
         v = new Variant(newValue);
      }

      CompletableFuture<List<StatusCode>> f = session.writeValues(ImmutableList.of(nodeId), ImmutableList.of(new DataValue(v)));
      StatusCode status = f.get().get(0);
      Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, String.format("writeNodeValue(%s, \"%s\") for %s: status %s", nodeId.toString(), newValue, url, status.toString()));
      return status.isGood();
   }

   /**
    * @param node
    * @param value
    * @return
    * @throws Exception
    */
   public boolean writeNode(String node, String value) throws Exception
   {
      connect();

      NodeId nodeId;
      try
      {
         nodeId = NodeId.parse(node);
      }
      catch(UaRuntimeException e)
      {
         Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, String.format("Error parsing node ID \"%s\"", node));
         return false;
      }

      try
      {
         return writeNodeValue(nodeId, value);
      }
      catch(ExecutionException e)
      {
         return handleExecutionException(e, () -> { return writeNodeValue(nodeId, value); });
      }
      catch(Exception e)
      {
         Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, String.format("Exception in writeNode(%s,%s) call for %s (%s)", node, value, url, e.getMessage()));
         Platform.writeDebugLog(OpcUaPlugin.DEBUG_TAG, 6, "   ", e);
         return false;
      }
   }
}
