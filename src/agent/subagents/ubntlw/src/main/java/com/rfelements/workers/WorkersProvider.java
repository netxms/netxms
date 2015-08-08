package com.rfelements.workers;
import com.rfelements.DeviceType;
import com.rfelements.model.Access;
/**
 * @author Pichanič Ján
 */
public interface WorkersProvider {

	void startNewWorker(Access access, DeviceType type);

	void stopDeviceTypeWorkers(DeviceType type);

}
