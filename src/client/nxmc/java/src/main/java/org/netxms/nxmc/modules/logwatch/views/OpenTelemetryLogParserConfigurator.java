/**
 * NetXMS - open source network management system
 * Copyright (C) 2026 Raden Solutions
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
package org.netxms.nxmc.modules.logwatch.views;

import org.netxms.nxmc.localization.LocalizationHelper;
import org.netxms.nxmc.modules.logwatch.widgets.helpers.LogParserType;

/**
 * Parser configurator for OpenTelemetry log parser
 */
public class OpenTelemetryLogParserConfigurator extends LogParserConfigurator
{
   /**
    * Create OpenTelemetry log parser configuration view
    */
   public OpenTelemetryLogParserConfigurator()
   {
      super(LogParserType.OTEL, "OpenTelemetryLogParser", LocalizationHelper.getI18n(OpenTelemetryLogParserConfigurator.class).tr("OpenTelemetry Log"), "icons/config-views/otel-log-parser.png");
   }
}
