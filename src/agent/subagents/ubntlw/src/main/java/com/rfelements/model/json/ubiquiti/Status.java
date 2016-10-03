package com.rfelements.model.json.ubiquiti;

/**
 * @author Pichanič Ján
 */
public class Status {

    private Integer plugged;

    private Integer speed;

    private Integer duplex;

    public Integer getPlugged() {
        return plugged;
    }

    public void setPlugged(Integer plugged) {
        this.plugged = plugged;
    }

    public Integer getSpeed() {
        return speed;
    }

    public void setSpeed(Integer speed) {
        this.speed = speed;
    }

    public Integer getDuplex() {
        return duplex;
    }

    public void setDuplex(Integer duplex) {
        this.duplex = duplex;
    }
}
