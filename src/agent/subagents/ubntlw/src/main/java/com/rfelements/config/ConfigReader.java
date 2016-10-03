package com.rfelements.config;

import com.rfelements.Protocol;
import com.rfelements.exception.CollectorException;
import com.rfelements.model.DeviceCredentials;
import org.netxms.agent.Config;

/**
 * @author Pichanič Ján
 */
public interface ConfigReader {

    void setConfig(Config config);

    void readSettings();

    DeviceCredentials getDeviceCredentials(Protocol defaultProtocol, String basePath, String ip) throws CollectorException;
}
