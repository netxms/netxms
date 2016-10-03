package com.rfelements.config;

import com.rfelements.Logger;
import com.rfelements.Protocol;
import com.rfelements.cache.Cache;
import com.rfelements.cache.CacheImpl;
import com.rfelements.config.ConfigReader;
import com.rfelements.config.Settings;
import com.rfelements.exception.CollectorException;
import com.rfelements.model.DeviceCredentials;
import org.netxms.agent.Config;

/**
 * @author Pichanič Ján
 */
public class ConfigReaderImpl implements ConfigReader {

    private static final Logger log = Logger.getInstance(ConfigReaderImpl.class);

    private static final String DATACOLLECTOR_CONFIG_BASIC_PATH = "/datacollector/";

    private static ConfigReaderImpl instance;

    private Cache cache;

    private Config config;

    private ConfigReaderImpl() {
        log.i("Config reader initialized");
        this.cache = CacheImpl.getInstance();
    }

    public static ConfigReaderImpl getInstance() {
        if (instance == null) {
            instance = new ConfigReaderImpl();
        }
        return instance;
    }

    @Override
    public void setConfig(Config config) {
        this.config = config;
    }

    @Override
    public void readSettings() {
        if (config == null) {
            log.e("[readSettings] Config is null");
            return;
        }

        log.i("[readSettings] Variables initialization");
        Settings.CONNECTION_TIMEOUT = config.getValueInt(DATACOLLECTOR_CONFIG_BASIC_PATH + "CONNECTION_TIMEOUT", 8000);
        Settings.REQUEST_TIMEOUT = config.getValueInt(DATACOLLECTOR_CONFIG_BASIC_PATH + "REQUEST_TIMEOUT", 8000);
        Settings.SOCKET_TIMEOUT = config.getValueInt(DATACOLLECTOR_CONFIG_BASIC_PATH + "SOCKET_TIMEOUT", 8000);
        Settings.LOGIN_TRIES_COUNT = config.getValueInt(DATACOLLECTOR_CONFIG_BASIC_PATH + "LOGIN_TRIES_COUNT", 3);
        Settings.UPDATE_PERIOD = config.getValueInt(DATACOLLECTOR_CONFIG_BASIC_PATH + "UPDATE_PERIOD", 20000);

        log.i("CONNECTION_TIMEOUT  : " + Settings.CONNECTION_TIMEOUT);
        log.i("REQUEST_TIMEOUT     : " + Settings.REQUEST_TIMEOUT);
        log.i("SOCKET_TIMEOUT      : " + Settings.SOCKET_TIMEOUT);
        log.i("LOGIN_TRIES_COUNT   : " + Settings.LOGIN_TRIES_COUNT);
        log.i("UPDATE_PERIOD       : " + Settings.UPDATE_PERIOD);
    }

    @Override
    public synchronized DeviceCredentials getDeviceCredentials(Protocol defaultProtocol, String basePath, String ip) throws CollectorException {
        if (config == null) {
            log.e("[getDeviceCredentials] Config is NULL");
            throw new CollectorException("Config is NULL");
        }
        String key = defaultProtocol + ip + basePath;
        if (cache.containsAccess(key)) {
            return cache.getAccess(key);
        }

        String credentials = config.getValue(basePath + ip, "");
        if (credentials.equals("")) {
            log.e(String.format("[getDeviceCredentials] Device IP not found in config file (%s%s)", basePath, ip));
            throw new CollectorException("Device IP not found in config file");
        }

        log.i("[getAccess] Accessing entry point " + defaultProtocol + ip + basePath + " , result : " + credentials + "");
        String[] split = credentials.split(":");
        DeviceCredentials access = new DeviceCredentials(defaultProtocol, ip, split[0], split[1]);
        cache.putAccess(key, access);
        return access;
    }
}
