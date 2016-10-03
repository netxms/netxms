package com.rfelements.provider;

import com.rfelements.DeviceType;
import com.rfelements.Protocol;
import com.rfelements.exception.CollectorException;
import com.rfelements.model.json.ligowave.Ligowave;
import com.rfelements.model.json.ubiquiti.Ubiquiti;
import org.netxms.agent.Config;

/**
 * @author Pichanič Ján
 */
public interface DataProvider {

    void setConfig(Config config);

    Ligowave getLigowaveObject(Protocol defaultProtocol, String basePath, String ip, DeviceType type) throws CollectorException;

    Ubiquiti getUbiquitiObject(Protocol defaultProtocol, String basePath, String ip, DeviceType type) throws CollectorException;

    void onShutdown(DeviceType type);
}
