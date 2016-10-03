package com.rfelements.workers;

import com.rfelements.DeviceType;
import com.rfelements.model.DeviceCredentials;

/**
 * @author Pichanič Ján
 */
public interface WorkersProvider {

    void startNewWorker(DeviceCredentials deviceCredentials, DeviceType type);

    void stopDeviceTypeWorkers(DeviceType type);

}
