package com.rfelements.model;

import com.rfelements.Protocol;

/**
 * @author Pichanič Ján
 */
public class DeviceCredentials {

    private Protocol protocol;

    private String ip;

    private String username;

    private String password;

    public DeviceCredentials(Protocol protocol, String ip, String username, String password) {
        this.protocol = protocol;
        this.ip = ip;
        this.username = username;
        this.password = password;
    }

    public String getIp() {
        return ip;
    }

    public void setIp(String base_url) {
        this.ip = base_url;
    }

    public String getUsername() {
        return username;
    }

    public void setUsername(String username) {
        this.username = username;
    }

    public String getPassword() {
        return password;
    }

    public void setPassword(String password) {
        this.password = password;
    }

    public Protocol getProtocol() {
        return protocol;
    }

    public void setProtocol(Protocol protocol) {
        this.protocol = protocol;
    }

    public String getUrl() {
        switch (protocol) {
            case HTTP:
                return String.format("http://%s", ip);
            case HTTPS:
                return String.format("https://%s", ip);
            default:
                throw new RuntimeException("Invalid  protocol");
        }
    }
}
