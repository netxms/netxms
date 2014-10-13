/**
 *
 */
package com.radensolutions.reporting.application;

import org.apache.commons.dbcp.BasicDataSource;
import org.netxms.base.EncryptedPassword;

import java.sql.SQLFeatureNotSupportedException;
import java.util.logging.Logger;

/**
 * Custom data source
 */
public class ReportingDataSource extends BasicDataSource {
    private String encryptedPassword = null;
    private String userName = null;

    /* (non-Javadoc)
     * @see javax.sql.CommonDataSource#getParentLogger()
     */
    @Override
    public Logger getParentLogger() throws SQLFeatureNotSupportedException {
        throw new SQLFeatureNotSupportedException("getParentLogger not supported");
    }

    /* (non-Javadoc)
     * @see org.apache.commons.dbcp.BasicDataSource#setUsername(java.lang.String)
     */
    @Override
    public void setUsername(String username) {
        userName = username;
        super.setUsername(username);
        if (encryptedPassword != null) {
            setPassword(encryptedPassword);
        }
    }

    /* (non-Javadoc)
     * @see org.apache.commons.dbcp.BasicDataSource#setPassword(java.lang.String)
     */
    @Override
    public void setPassword(String password) {
        encryptedPassword = password;
        if (userName != null) {
            try {
                super.setPassword(EncryptedPassword.decrypt(userName, password));
            } catch (Exception e) {
                e.printStackTrace();
                super.setPassword(password);
            }
        }
    }
}
