/**
 * NetXMS - open source network management system
 * Copyright (C) 2025-2026 Raden Solutions
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
package org.netxms.nxmc.modules.ai.actions;

import java.io.BufferedWriter;
import java.io.FileOutputStream;
import java.io.OutputStreamWriter;
import java.nio.charset.StandardCharsets;
import org.eclipse.core.runtime.IProgressMonitor;
import org.eclipse.jface.action.Action;
import org.eclipse.swt.SWT;
import org.eclipse.swt.widgets.FileDialog;
import org.netxms.nxmc.Registry;
import org.netxms.nxmc.base.jobs.Job;
import org.netxms.nxmc.base.views.View;
import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.ai.widgets.AiAssistantChatWidget;
import org.netxms.nxmc.resources.SharedIcons;
import org.xnap.commons.i18n.I18n;

/**
 * Action for exporting AI assistant conversation to a Markdown file (SWT version).
 */
public class ExportConversationAction extends Action
{
   private final I18n i18n = LocalizationHelper.getI18n(ExportConversationAction.class);

   private View view;
   private AiAssistantChatWidget chatWidget;

   /**
    * Create export conversation action.
    *
    * @param view owning view
    * @param chatWidget chat widget to export from
    */
   public ExportConversationAction(View view, AiAssistantChatWidget chatWidget)
   {
      super(LocalizationHelper.getI18n(ExportConversationAction.class).tr("&Export conversation..."), SharedIcons.EXPORT);
      this.view = view;
      this.chatWidget = chatWidget;
   }

   /**
    * @see org.eclipse.jface.action.Action#run()
    */
   @Override
   public void run()
   {
      if (!chatWidget.hasMessagesToExport())
         return;

      FileDialog dlg = new FileDialog((view != null) ? view.getWindow().getShell() : Registry.getMainWindowShell(), SWT.SAVE);
      dlg.setOverwrite(true);
      dlg.setFilterExtensions(new String[] { "*.md", "*.*" });
      dlg.setFilterNames(new String[] { i18n.tr("Markdown files"), i18n.tr("All files") });
      dlg.setFileName("ai-conversation.md");
      final String fileName = dlg.open();
      if (fileName == null)
         return;

      final String content = chatWidget.exportAsMarkdown();
      new Job(i18n.tr("Exporting conversation"), view) {
         @Override
         protected void run(IProgressMonitor monitor) throws Exception
         {
            BufferedWriter out = new BufferedWriter(new OutputStreamWriter(new FileOutputStream(fileName), StandardCharsets.UTF_8));
            out.write('\ufeff'); // write BOM
            out.write(content);
            out.close();
         }

         @Override
         protected String getErrorMessage()
         {
            return i18n.tr("Cannot export conversation to file");
         }
      }.start();
   }
}
