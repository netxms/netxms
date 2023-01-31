/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2023 Victor Kirhenshtein
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
package org.netxms.nxmc.modules.agentmanagement.widgets;

import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;
import org.eclipse.jface.resource.JFaceResources;
import org.eclipse.swt.SWT;
import org.eclipse.swt.custom.LineStyleEvent;
import org.eclipse.swt.custom.LineStyleListener;
import org.eclipse.swt.custom.StyleRange;
import org.eclipse.swt.custom.StyledText;
import org.eclipse.swt.graphics.Color;
import org.eclipse.swt.graphics.Point;
import org.eclipse.swt.layout.FillLayout;
import org.eclipse.swt.widgets.Composite;
import org.eclipse.swt.widgets.Display;

/**
 * Agent configuration file editor
 */
public class AgentConfigEditor extends Composite
{
   private static final Color ATTRIBUTE_COLOR = new Color(Display.getCurrent(), 157, 0, 157);
   private static final Color COMMENT_COLOR = new Color(Display.getCurrent(), 128, 128, 128);
   private static final Color DEPRECATED_ATTRIBUTE_COLOR = new Color(Display.getCurrent(), 160, 0, 0);
   private static final Color ENVVAR_COLOR = new Color(Display.getCurrent(), 38, 127, 0);
   private static final Color SECTION_COLOR = new Color(Display.getCurrent(), 128, 128, 0);
   private static final Color STRING_COLOR = new Color(Display.getCurrent(), 0, 0, 192);

   private static final String[] CONFIG_KEYWORDS = {
      "Action",
      "AppAgent",
      "BackgroundLogWriter",
      "ConfigIncludeDir",
      "ControlServers",
      "CreateCrashDumps",
      "CRL",
      "CRLReloadInterval",
      "DataCollectionMaxThreadPoolSize",
      "DataCollectionMinThreadPoolSize",
      "DataDirectory",
      "DataReconciliationBlockSize",
      "DataReconciliationTimeout",
      "DataWriterFlushInterval",
      "DataWriterMaxTransactionSize",
      "DailyLogFileSuffix",
      "DebugLevel",
      "DebugTags",
      "DefaultExecutionTimeout",
      "DisableIPv4",
      "DisableIPv6",
      "DisableLocalDatabase",
      "DumpDirectory",
      "EnableActions",
      "EnableArbitraryCommandExecution",
      "EnabledCiphers",
      "EnableControlConnector",
      "EnableEventConnector",
      "EnableProxy",
      "EnablePushConnector",
      "EnableSNMPProxy",
      "EnableSNMPTrapProxy",
      "EnableSSLTrace",
      "EnableSyslogProxy",
      "EnableSubagentAutoload",
      "EnableTCPProxy",
      "EnableTFTPProxy",
      "EnableWatchdog",
      "EnableWebServiceProxy",
      "ExternalCommandTimeout",
      "ExternalList",
      "ExternalMasterAgent",
      "ExternalMetric",
      "ExternalMetricProvider",
      "ExternalMetricProviderTimeout",
      "ExternalMetricTimeout",
      "ExternalSubAgent",
      "ExternalTable",
      "FatalExitOnCRTError",
      "FileStore",
      "FullCrashDumps",
      "GroupId",
      "ListenAddress",
      "ListenPort",
      "LogFile",
      "LogHistorySize",
      "LogRotationMode",
      "LogUnresolvedSymbols",
      "LongRunningQueryThreshold",
      "MasterServers",
      "MaxLogSize",
      "MaxSessions",
      "OfflineDataExpirationTime",
      "PlatformSuffix",
      "RequireAuthentication",
      "RequireEncryption",
      "Servers",
      "SessionIdleTimeout",
      "SharedSecret",
      "SNMPTimeout",
      "SNMPTrapListenAddress",
      "SNMPTrapPort",
      "StartupDelay",
      "SubAgent",
      "SyslogListenPort",
      "SystemName",
      "TrustedRootCertificate",
      "TunnelKeepaliveInterval",
      "VerifyServerCertificate",
      "UserId",
      "WaitForProcess",
      "WebServiceCacheExpirationTime",
      "WebServiceThreadPoolSize",
      "WriteLogAsJson",
      "ZoneUIN"
   };

   private static final String[] DEPRECATED_CONFIG_KEYWORDS = {
      "ActionShellExec",
      "DataCollectionThreadPoolSize",
      "EncryptedSharedSecret",
      "ExecTimeout",
      "ExternalMetricShellExec",
      "ExternalParameter",
      "ExternalParameterShellExec",
      "ExternalParameterProvider",
      "ExternalParameterProviderTimeout",
      "ExternalParameterTimeout",
      "ExternalParametersProvider",
      "ZoneId"
   };
   
   private StyledText editor;
   private Set<String> keywords;
   private Set<String> deprecatedKeywords;

   /**
    * @param parent
    * @param style
    * @param editorStyle
    */
   public AgentConfigEditor(Composite parent, int style, int editorStyle)
   {
      super(parent, style);
      
      keywords = new HashSet<>();
      for(String s : CONFIG_KEYWORDS)
         keywords.add(s.toLowerCase());
      deprecatedKeywords = new HashSet<>();
      for(String s : DEPRECATED_CONFIG_KEYWORDS)
         deprecatedKeywords.add(s.toLowerCase());

      setLayout(new FillLayout());
      editor = new StyledText(this, editorStyle | SWT.MULTI);
      editor.setFont(JFaceResources.getTextFont());
      editor.setWordWrap(false);
      editor.addLineStyleListener(new LineStyleListener() {
         @Override
         public void lineGetStyle(LineStyleEvent event)
         {
            applyLineStyle(event);
         }
      });
   }

   /**
    * Apply line style to given line
    *
    * @param event style event
    */
   private void applyLineStyle(LineStyleEvent event)
   {
      char[] line = event.lineText.toCharArray();

      int index = 0;
      while((index < line.length) && Character.isWhitespace(line[index]))
         index++;
      if (index == line.length)
         return;

      StyleRange[] styles = new StyleRange[64];
      int styleCount = 0;

      // Find comment position
      int commentStart = -1;
      if (line[index] == '[')
      {
         // Configuration section as [name]
         int startPos = index, endPos = -1;
         while((index < line.length) && (line[index] != ']'))
            index++;
         if (index == line.length)
            return;
         endPos = index;
         while((index < line.length) && (line[index] != '#'))
            index++;
         if (index != line.length)
            commentStart = index;

         styles[styleCount++] = new StyleRange(event.lineOffset + startPos, endPos - startPos + 1, SECTION_COLOR, null, SWT.BOLD);
      }
      else if (line[index] == '*')
      {
         // Configuration section as *name
         int startPos = index;
         while((index < line.length) && (line[index] != '#'))
            index++;
         styles[styleCount++] = new StyleRange(event.lineOffset + startPos, index - startPos, SECTION_COLOR, null, SWT.BOLD);

         if (index != line.length)
            commentStart = index;
      }
      else
      {
         int startPos = index;
         while((index < line.length) && !Character.isWhitespace(line[index]) && (line[index] != '#') && (line[index] != '='))
            index++;
         while((index < line.length) && Character.isWhitespace(line[index]))
            index++;
         if (index == line.length)
            return;

         if (line[index] == '=')
         {
            String attr = event.lineText.substring(startPos, index).trim().toLowerCase();
            Color c;
            if (keywords.contains(attr))
               c = ATTRIBUTE_COLOR;
            else if (deprecatedKeywords.contains(attr))
               c = DEPRECATED_ATTRIBUTE_COLOR;
            else
               c = null;
            styles[styleCount++] = new StyleRange(event.lineOffset + startPos, index - startPos, c, null, SWT.BOLD);

            styles[styleCount++] = new StyleRange(event.lineOffset + index, 1, null, null, SWT.BOLD);
            index++;

            startPos = -1;
            boolean squotes = false, dquotes = false;
            while(index < line.length)
            {
               char ch = line[index];
               if ((ch == '"') && !squotes)
               {
                  if (dquotes)
                  {
                     dquotes = false;
                     styles[styleCount++] = new StyleRange(event.lineOffset + startPos, index - startPos + 1, STRING_COLOR, null);
                     startPos = -1;
                  }
                  else
                  {
                     dquotes = true;
                     startPos = index;
                  }
               }
               else if ((ch == '\'') && !dquotes)
               {
                  if (squotes)
                  {
                     squotes = false;
                     styles[styleCount++] = new StyleRange(event.lineOffset + startPos, index - startPos + 1, STRING_COLOR, null);
                     startPos = -1;
                  }
                  else
                  {
                     squotes = true;
                     startPos = index;
                  }
               }
               else if ((ch == '#') && !squotes && !dquotes)
               {
                  commentStart = index;
                  break;
               }
               else if ((ch == '$') && (index < line.length - 1) && (line[index + 1] == '{') && !squotes)
               {
                  if (dquotes)
                     styles[styleCount++] = new StyleRange(event.lineOffset + startPos, index - startPos, STRING_COLOR, null);
                  startPos = index;
                  index++;
               }
               else if ((ch == '}') && (startPos != -1) && !squotes)
               {
                  styles[styleCount++] = new StyleRange(event.lineOffset + startPos, index - startPos + 1, ENVVAR_COLOR, null, SWT.BOLD);
                  startPos = dquotes ? index + 1 : -1;
               }
               index++;
            }
         }
         else if (line[index] == '#')
         {
            commentStart = index;
         }
         else
         {
            while((index < line.length) && (line[index] != '#'))
               index++;
            if (index != line.length)
               commentStart = index;
         }
      }

      if (commentStart != -1)
         styles[styleCount++] = new StyleRange(event.lineOffset + commentStart, line.length - commentStart, COMMENT_COLOR, null);
      event.styles = (styleCount > 0) ? Arrays.copyOf(styles, styleCount) : null;
   }

   /**
    * Get underlying text widget
    * @return text widget
    */
   public StyledText getTextWidget()
   {
      return editor;
   }

   /**
    * Set text for editing
    * @param text
    */
   public void setText(String text)
   {
      editor.setText(text);
   }

   /**
    * Get editor's content
    * @return
    */
   public String getText()
   {
      return editor.getText();
   }

   /**
    * @see org.eclipse.swt.widgets.Composite#computeSize(int, int, boolean)
    */
   @Override
   public Point computeSize(int wHint, int hHint, boolean changed)
   {
      Point p = editor.computeSize(wHint, hHint, changed);
      p.y += 4;
      return p;
   }
}
