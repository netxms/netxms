/**
 * NetXMS - open source network management system
 * Copyright (C) 2020-2025 Raden Soultions
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
package org.netxms.nxmc.modules.users.reports.acl.constants;

public enum PermissionsSheetCells
{
   OBJECT_NAME,
   USER_OR_GROUP_NAME,
   INHERIT_ACCESS,
   READ,
   DELEGATED_READ,
   MODIFY,
   EDIT_COMMENTS,
   EDIT_RESP_USERS,
   DELETE,
   CONTROL,
   MAINTENANCE,
   CONFIGURE_AGENT,
   READ_AGENT,
   READ_SNMP,
   CREATE_CHILD_OBJECTS,
   VIEW_ALARMS,
   ACKNOWLEDGE_ALARMS,
   TERMINATE_ALARMS,
   ACCESS_CONTROL,
   CREATE_HELPDESK_ISSUES,
   DOWNLOAD_FILES,
   UPLOAD_FILES,
   MANAGE_FILES,
   SEND_EVENTS,
   PUSH_DATA,
   TAKE_SCREENSHOT,
   EDIT_MAINTENANCE_JOURNAL
}
