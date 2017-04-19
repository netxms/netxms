/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2017 Raden Solutions
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
package org.netxms.ui.android.service.tasks;

import android.os.AsyncTask;

import org.netxms.client.NXCSession;
import org.netxms.client.objecttools.ObjectTool;
import org.netxms.ui.android.R;
import org.netxms.ui.android.service.ClientConnectorService;

/**
 * Execute Object Tools
 */
public class ExecObjectToolTask extends AsyncTask<Object, Void, Exception>
{
    private ClientConnectorService service;

    /* (non-Javadoc)
     * @see android.os.AsyncTask#doInBackground(Params[])
     */
    @Override
    protected Exception doInBackground(Object... params)
    {
        service = (ClientConnectorService)params[3];
        try
        {
            switch(((ObjectTool)params[2]).getType())
            {
                case ObjectTool.TYPE_INTERNAL:
                    if (((ObjectTool)params[2]).getData().equals("wakeup"))
                    {
                        ((NXCSession)params[0]).wakeupNode((Long)params[1]);
                        break;
                    }
                case ObjectTool.TYPE_ACTION:
                    ((NXCSession)params[0]).executeAction((Long)params[1], ((ObjectTool)params[2]).getData(), null);
                    break;
                case ObjectTool.TYPE_SERVER_COMMAND:
                    ((NXCSession)params[0]).executeServerCommand((Long)params[1], ((ObjectTool)params[2]).getData(), null);
                    break;
                case ObjectTool.TYPE_SERVER_SCRIPT:
                    ((NXCSession)params[0]).executeLibraryScript((Long)params[1], ((ObjectTool)params[2]).getData(), null, null);
                    break;
            }
        }
        catch (Exception e)
        {
            return e;
        }
        return null;
    }

    /* (non-Javadoc)
     * @see android.os.AsyncTask#onPostExecute(java.lang.Object)
     */
    @Override
    protected void onPostExecute(Exception result)
    {
        if (result == null)
        {
            service.showToast(service.getString(R.string.notify_action_exec_success));
        }
        else
        {
            service.showToast(service.getString(R.string.notify_action_exec_fail, result.getLocalizedMessage()));
        }
    }
}
