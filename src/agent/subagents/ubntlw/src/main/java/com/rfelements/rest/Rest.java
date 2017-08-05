package com.rfelements.rest;

import java.io.IOException;
import java.security.KeyStore;
import java.util.Date;
import java.util.concurrent.ConcurrentHashMap;
import javax.net.ssl.HostnameVerifier;
import javax.net.ssl.SSLSession;
import org.apache.http.HttpResponse;
import org.apache.http.client.HttpClient;
import org.apache.http.client.config.RequestConfig;
import org.apache.http.client.methods.HttpGet;
import org.apache.http.client.methods.HttpPost;
import org.apache.http.conn.ssl.TrustSelfSignedStrategy;
import org.apache.http.entity.StringEntity;
import org.apache.http.entity.mime.MultipartEntityBuilder;
import org.apache.http.impl.client.BasicCookieStore;
import org.apache.http.impl.client.HttpClientBuilder;
import org.apache.http.ssl.SSLContextBuilder;
import org.apache.http.util.EntityUtils;
import org.netxms.bridge.LogLevel;
import org.netxms.bridge.Platform;
import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.rfelements.config.Settings;
import com.rfelements.exception.CollectorException;
import com.rfelements.gson.BooleanTypeAdapter;
import com.rfelements.gson.StringTypeAdapter;
import com.rfelements.model.DeviceCredentials;
import com.rfelements.model.json.ligowave.Ligowave;
import com.rfelements.model.json.ubiquiti.Ubiquiti;

/**
 * @author Pichanič Ján
 */
public class Rest {

    private static final int DEBUG_LEVEL = 8;

    private static final int DEBUG_LEVEL_DETAILED = 9;

    private static final ConcurrentHashMap<String, BasicCookieStore> cookiesStoreCache = new ConcurrentHashMap<>();

    private static final ConcurrentHashMap<String, HttpClient> httpClientCache = new ConcurrentHashMap<>();

    private static HttpClient initOrGetHttpClient(String ip) throws CollectorException {
        HttpClient client = httpClientCache.get(ip);
        if (client != null) {
            return client;
        }

        SSLContextBuilder ctx = SSLContextBuilder.create();
        RequestConfig.Builder request = RequestConfig.custom();
        request = request.setConnectTimeout(Settings.CONNECTION_TIMEOUT);
        request = request.setConnectionRequestTimeout(Settings.REQUEST_TIMEOUT);
        request = request.setSocketTimeout(Settings.SOCKET_TIMEOUT);

        BasicCookieStore cs;
        if (cookiesStoreCache.get(ip) == null) {
            cookiesStoreCache.put(ip, new BasicCookieStore());
            Platform.writeDebugLog(DEBUG_LEVEL, Thread.currentThread().getName() +
                    " [" + Rest.class.getName() + "][initOrGetHttpClient] ip : " + ip + " , Cookie store initialization and storing into cache ... ");
        }
        cs = cookiesStoreCache.get(ip);

        try {
            ctx.loadTrustMaterial(KeyStore.getInstance(KeyStore.getDefaultType()), new TrustSelfSignedStrategy());
            client = HttpClientBuilder.create().setDefaultRequestConfig(request.build()).setSSLHostnameVerifier(new HostnameVerifier() {

                @Override
                public boolean verify(String s, SSLSession sslSession) {
                    return true;
                }
            }).setSslcontext(ctx.build()).setDefaultCookieStore(cs).build();
        } catch (Exception e) {
           Platform.writeLog(LogLevel.ERROR, e.getLocalizedMessage());
            throw new CollectorException(e.getMessage(), e.getCause());
        }
        Platform.writeDebugLog(DEBUG_LEVEL, Thread.currentThread().getName() + " [" + Rest.class.getName() + "][initOrGetHttpClient] ip : " + ip
                + " , Http client is storing into cache , client hash code : " + client.hashCode());
        httpClientCache.put(ip, client);
        return client;
    }

    public static boolean loginUbiquiti(final DeviceCredentials deviceCredentials) throws CollectorException {
       Platform.writeDebugLog(DEBUG_LEVEL, Thread.currentThread().getName() +
                " [" + Rest.class.getName() + "] [loginUbiquiti] Performing login against (ubiquiti) " + deviceCredentials.getUrl());
        HttpClient client = initOrGetHttpClient(deviceCredentials.getIp());

        HttpPost post = new HttpPost(deviceCredentials.getUrl() + "/login.cgi");
        MultipartEntityBuilder builder = MultipartEntityBuilder.create().addTextBody("username", deviceCredentials.getUsername())
                .addTextBody("password", deviceCredentials.getPassword());
        HttpResponse response = null;
        try {
            post.setEntity(builder.build());
            response = client.execute(post);
            if (200 == response.getStatusLine().getStatusCode() || 302 == response.getStatusLine().getStatusCode()) {
                String responseStr = EntityUtils.toString(response.getEntity());
                if (responseStr.contains("Invalid credentials.")) {
                   Platform.writeDebugLog(DEBUG_LEVEL, Thread.currentThread().getName() +
                            " [" + Rest.class.getName() + "][loginUbiquiti] ip : " + deviceCredentials.getIp() + " , Failed to login. Invalid credentials");
                    return false;
                }
                Platform.writeDebugLog(DEBUG_LEVEL_DETAILED,
                        Thread.currentThread().getName() + " [" + Rest.class.getName() + "][loginUbiquiti][Ubiquiti] ip : " + deviceCredentials.getIp()
                                + " , Login has been successful !");
                return true;
            }
        } catch (IOException e) {
            Platform.writeLog(LogLevel.ERROR,
                    "[" + Rest.class.getName() + "][loginUbiquiti] Login failed because of exception : " + e.getLocalizedMessage());
            throw new CollectorException(e.getMessage(), e.getCause());
        } finally {
            if (response != null)
                EntityUtils.consumeQuietly(response.getEntity());
        }
        return false;
    }

    public static boolean loginLigowave(final DeviceCredentials deviceCredentials) throws CollectorException {
        Platform.writeDebugLog(DEBUG_LEVEL,
                Thread.currentThread().getName() + " [" + Rest.class.getName() + "] [loginLigowave] Performing login against (ligowave): " + deviceCredentials.getUrl());
        HttpClient client = initOrGetHttpClient(deviceCredentials.getIp());

        HttpPost post = new HttpPost(deviceCredentials.getUrl() + "/cgi-bin/main.cgi/login");
        post.addHeader("Content-Type", "application/json");
        HttpResponse response = null;
        try {
            StringEntity entity = new StringEntity(
                    "{\"username\":\"" + deviceCredentials.getUsername() + "\",\"password\":\"" + deviceCredentials.getPassword() + "\",\"language\":\"en_US\"}");
            entity.setContentType("application/json");
            entity.setContentEncoding("charset=utf-8");
            post.setEntity(entity);
            response = client.execute(post);
            if (200 == response.getStatusLine().getStatusCode() || 302 == response.getStatusLine().getStatusCode()) {
                String responseStr = EntityUtils.toString(response.getEntity());
                if (!responseStr.equals("{\"message\":\"Incorrect username or password\",\"status\":false}")) {
                    Platform.writeDebugLog(DEBUG_LEVEL, Thread.currentThread().getName() +
                            " [" + Rest.class.getName() + "][loginLigowave] Login against : " + deviceCredentials.getUrl() + "/cgi-bin/main.cgi/login" + " has been successful");
                    return true;
                } else {
                    Platform.writeDebugLog(DEBUG_LEVEL_DETAILED, Thread.currentThread().getName() + " [" + Rest.class.getName()
                            + "][LogIn][Deliberant] Login error , incorrect username or password");
                    return false;
                }
            }
        } catch (IOException e) {
            Platform.writeLog(LogLevel.ERROR,
                    "[" + Rest.class.getName() + "][loginLigowave] Login failed because of exception : " + e.getLocalizedMessage());
            throw new CollectorException(e.getMessage(), e.getCause());
        } finally {
            if (response != null)
                EntityUtils.consumeQuietly(response.getEntity());
        }
        return false;
    }

    public static void getIndexSiteUbnt(DeviceCredentials deviceCredentials) throws CollectorException {
        Platform.writeDebugLog(DEBUG_LEVEL,
                Thread.currentThread().getName() + " [" + Rest.class.getName() + "] [getIndexSiteUbnt] ip : " + deviceCredentials.getIp()
                        + " , DeviceCredentials to website to get cookies ... ");
        HttpClient client = initOrGetHttpClient(deviceCredentials.getIp());
        HttpGet get = new HttpGet(deviceCredentials.getUrl() + "/login.cgi");
        HttpResponse response = null;
        try {
            response = client.execute(get);
            EntityUtils.consumeQuietly(response.getEntity());
        } catch (IOException e) {
            Platform.writeLog(LogLevel.ERROR,
                    " [" + Rest.class.getName() + "] [getIndexSiteUbnt] Exception occurred while executing http get, message : " + e
                            .getLocalizedMessage());
            throw new CollectorException(e.getMessage(), e.getCause());
        } finally {
            if (response != null)
                EntityUtils.consumeQuietly(response.getEntity());
        }
    }

    public static boolean isJSONValid(String JSON_STRING) {
        try {
            Gson gson = new Gson();
            gson.fromJson(JSON_STRING, Object.class);
            return true;
        } catch (com.google.gson.JsonSyntaxException ex) {
            return false;
        }
    }

    public static Ubiquiti updateUbiquitiJsonObject(DeviceCredentials deviceCredentials, int loginTries) throws CollectorException {
        Platform.writeDebugLog(DEBUG_LEVEL,
                Thread.currentThread().getName() + " [" + Rest.class.getName() + "] [updateUbiquitiJsonObject] ip : " + deviceCredentials.getIp()
                        + " , Ubiquiti json object update started ...");
        HttpClient client = initOrGetHttpClient(deviceCredentials.getIp());

        if (cookiesStoreCache.get(deviceCredentials.getIp()).getCookies().isEmpty()) {
            getIndexSiteUbnt(deviceCredentials);
        }

        HttpGet get = new HttpGet(deviceCredentials.getUrl() + "/status.cgi?_=" + new Date().getTime());
        HttpResponse response = null;
        String responseStr = null;
        try {
            response = client.execute(get);
            Platform.writeDebugLog(DEBUG_LEVEL,
                    Thread.currentThread().getName() + " [" + Rest.class.getName() + "] [updateUbiquitiJsonObject] ip : " + deviceCredentials.getIp()
                            + " Update UBNT obj response code : " + response.getStatusLine().getStatusCode());
            responseStr = EntityUtils.toString(response.getEntity());
            if (!isJSONValid(responseStr)) {
                //				getIndexSiteUbnt(deviceCredentials);
                if (loginTries <= 0)
                    return null;
                loginUbiquiti(deviceCredentials);
                return updateUbiquitiJsonObject(deviceCredentials, --loginTries);
            }
            Platform.writeDebugLog(DEBUG_LEVEL_DETAILED,
                    Thread.currentThread().getName() + " [" + Rest.class.getName() + "] [updateUbiquitiJsonObject] ip : " + deviceCredentials.getIp()
                            + " , response text : " + responseStr);
        } catch (IOException e) {
            Platform.writeLog(LogLevel.ERROR,
                    Thread.currentThread().getName() + " [" + Rest.class.getName() + "] [updateUbiquitiJsonObject] " + e.getLocalizedMessage());
            throw new CollectorException(e.getMessage(), e.getCause());
        } finally {
            if (response != null)
                EntityUtils.consumeQuietly(response.getEntity());
        }

        Gson gson = new GsonBuilder().registerTypeAdapter(Boolean.class, new BooleanTypeAdapter()).create();
        Ubiquiti ubnt;
        try {
            ubnt = gson.fromJson(responseStr, Ubiquiti.class);
        } catch (Exception e) {
            Platform.writeLog(LogLevel.ERROR,
                    Thread.currentThread().getName() + " [" + Rest.class.getName() + "] [updateUbiquitiJsonObject] " + e.getLocalizedMessage());
            throw new CollectorException(e.getMessage(), e.getCause());
        }
        return ubnt;
    }

    public static Ligowave updateLigowaveJsonObject(DeviceCredentials deviceCredentials, int loginTries) throws CollectorException {
        Platform.writeDebugLog(DEBUG_LEVEL,
                Thread.currentThread().getName() + " [" + Rest.class.getName() + "] [updateLigowaveJsonObject] ip : " + deviceCredentials.getIp()
                        + " , Ligowave json object update started ...");
        HttpClient client = initOrGetHttpClient(deviceCredentials.getIp());

        HttpGet get = new HttpGet(deviceCredentials.getUrl() + "/cgi-bin/main.cgi/status");
        HttpResponse response = null;
        String responseStr = null;
        try {
            response = client.execute(get);
            Platform.writeDebugLog(DEBUG_LEVEL,
                    Thread.currentThread().getName() + " [" + Rest.class.getName() + "] [updateLigowaveJsonObject] ip : " + deviceCredentials.getIp()
                            + " Update ligowave obj response code : " + response.getStatusLine().getStatusCode());
            if (response.getStatusLine().getStatusCode() != 200) {
                EntityUtils.consumeQuietly(response.getEntity());
                if (loginTries <= 0)
                    return null;
                loginLigowave(deviceCredentials);
                return updateLigowaveJsonObject(deviceCredentials, --loginTries);
            }
            responseStr = EntityUtils.toString(response.getEntity());
            Platform.writeDebugLog(DEBUG_LEVEL_DETAILED,
                    Thread.currentThread().getName() + " [" + Rest.class.getName() + "] [updateLigowaveJsonObject] ip : " + deviceCredentials.getIp()
                            + " , response text : " + responseStr);
        } catch (IOException e) {
            Platform.writeLog(LogLevel.ERROR,
                    Thread.currentThread().getName() + " [" + Rest.class.getName() + "] [updateLigowaveJsonObject] " + e.getLocalizedMessage());
            e.printStackTrace();
        } finally {
            if (response != null)
                EntityUtils.consumeQuietly(response.getEntity());
        }
        Gson gson = new GsonBuilder().registerTypeAdapter(Boolean.class, new BooleanTypeAdapter())
                .registerTypeAdapter(String.class, new StringTypeAdapter()).create();
        Ligowave ligowave;
        try {
            ligowave = gson.fromJson(responseStr, Ligowave.class);
        } catch (Exception e) {
            Platform.writeLog(LogLevel.ERROR, " [" + Rest.class.getName() + "] [updateLigowaveJsonObject] " + e.getLocalizedMessage());
            throw new CollectorException(e.getMessage(), e.getCause());
        }
        return ligowave;
    }
}
