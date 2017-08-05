package com.rfelements.config;

import org.netxms.bridge.Config;
import com.rfelements.Protocol;
import com.rfelements.exception.CollectorException;
import com.rfelements.model.DeviceCredentials;

/**
 * @author Pichanič Ján
 */
public interface ConfigReader {

    void setConfig(Config config);

    void readSettings();

    DeviceCredentials getDeviceCredentials(Protocol defaultProtocol, String basePath, String ip) throws CollectorException;
}
