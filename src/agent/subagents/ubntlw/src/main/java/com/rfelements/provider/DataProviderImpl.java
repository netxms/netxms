package com.rfelements.provider;

import org.netxms.bridge.Config;
import org.netxms.bridge.LogLevel;
import org.netxms.bridge.Platform;
import com.rfelements.DeviceType;
import com.rfelements.Protocol;
import com.rfelements.cache.Cache;
import com.rfelements.cache.CacheImpl;
import com.rfelements.config.ConfigReader;
import com.rfelements.config.ConfigReaderImpl;
import com.rfelements.exception.CollectorException;
import com.rfelements.model.DeviceCredentials;
import com.rfelements.model.json.ligowave.Ligowave;
import com.rfelements.model.json.ubiquiti.Ubiquiti;
import com.rfelements.workers.WorkersProvider;
import com.rfelements.workers.WorkersProviderImpl;

/**
 * @author Pichanič Ján
 */
public class DataProviderImpl implements DataProvider {

    private static DataProviderImpl instance;
    private static final int DEBUG_LEVEL = 6;

    public static DataProviderImpl getInstance() {
        if (instance == null)
            instance = new DataProviderImpl();
        return instance;
    }

    private Cache cache;

    private Config config;

    private ConfigReader configReader;

    private WorkersProvider workersProvider;

    private DataProviderImpl() {
       Platform.writeLog(LogLevel.INFO, Thread.currentThread().getName() + " [DataProviderImpl] Data provider initialized ...");
        this.cache = CacheImpl.getInstance();
        this.workersProvider = WorkersProviderImpl.getInstance();
        this.configReader = ConfigReaderImpl.getInstance();
    }

    @Override
    public void setConfig(Config config) {
        // I don't know how many Entry points calling this method (I think all of them) , therefore i check if config was already instantiated
        if (this.config == null) {
            this.config = config;
            this.configReader.setConfig(config);
            this.configReader.readSettings();
        }
    }

    @Override
    public Ligowave getLigowaveObject(Protocol defaultProtocol, String basePath, String ip, DeviceType type) throws CollectorException {
        DeviceCredentials deviceCredentials = configReader.getDeviceCredentials(defaultProtocol, basePath, ip);
        this.workersProvider.startNewWorker(deviceCredentials, type);
        Ligowave ligowave = (Ligowave) cache.getJsonObject(deviceCredentials.getIp());
        if (ligowave == null) {
            String message = "Value of ligowave JSON object in the cache, under key : " + ip + " , is NULL !";
            Platform.writeDebugLog(DEBUG_LEVEL, message);
            throw new CollectorException(message);
        }
        return ligowave;
    }

    @Override
    public Ubiquiti getUbiquitiObject(Protocol defaultProtocol, String basePath, String ip, DeviceType type) throws CollectorException {
        DeviceCredentials deviceCredentials = configReader.getDeviceCredentials(defaultProtocol, basePath, ip);
        this.workersProvider.startNewWorker(deviceCredentials, type);
        Ubiquiti ubnt = (Ubiquiti) cache.getJsonObject(deviceCredentials.getIp());
        if (ubnt == null) {
            String message = "Value of ubiquiti JSON object in the cache, under key : " + ip + " , is NULL !";
            Platform.writeDebugLog(DEBUG_LEVEL, message);
            throw new CollectorException(message);
        }
        return ubnt;
    }

    @Override
    public void onShutdown(DeviceType type) {
        this.workersProvider.stopDeviceTypeWorkers(type);
    }
}
