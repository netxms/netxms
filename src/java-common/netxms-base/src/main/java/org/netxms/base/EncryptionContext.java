/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2013 Victor Kirhenshtein
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
package org.netxms.base;

import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.security.GeneralSecurityException;
import java.security.InvalidKeyException;
import java.security.KeyFactory;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.security.PublicKey;
import java.security.SecureRandom;
import java.security.spec.X509EncodedKeySpec;
import java.util.Arrays;
import java.util.zip.CRC32;
import javax.crypto.Cipher;
import javax.crypto.KeyGenerator;
import javax.crypto.SecretKey;
import javax.crypto.spec.IvParameterSpec;

/**
 * Encryption context for NXCP communication session
 */
public final class EncryptionContext
{
	// Ciphers
	private static final String[] CIPHERS = { "AES", "Blowfish", null, null, "AES", "Blowfish" };
	private static int[] KEY_LENGTHS = { 256, 256, 0, 0, 128, 128 };
	private static final String CIPHER_MODE = "/CBC/PKCS5Padding";
	private static final byte[] TEST_BYTES = "Test String".getBytes();
	private static final boolean[] cipherTests;

	private int cipher;
	private int keyLength;
	private Cipher encryptor; 
	private Cipher decryptor;
	private SecretKey key;
	private IvParameterSpec iv;
	private PublicKey serverPublicKey;

   /* (non-Javadoc)
    * @see java.lang.Object#toString()
    */
   @Override
   public String toString()
   {
      return "EncryptionContext [cipher=" + CIPHERS[cipher] + " keyLength=" + KEY_LENGTHS[cipher] + "]";
   }
   
	static
	{
	   cipherTests = new boolean[CIPHERS.length];
	   for(int i = 0; i < CIPHERS.length; i++)
	      cipherTests[i] = testCipher(i);
	}
	
	/**
	 * Get cipher name
	 * 
	 * @param cipher cipher ID
	 * @return cipher name or null if ID is invalid or cipher is not supported
	 */
	public static String getCipherName(int cipher)
	{
	   try
	   {
	      return CIPHERS[cipher] + "-" + Integer.toString(KEY_LENGTHS[cipher]);
	   }
	   catch(ArrayIndexOutOfBoundsException e)
	   {
	      return null;
	   }
	}
	
	/**
	 * Write bytes to output stream if byte array is not null
	 * 
	 * @param bs output stream
	 * @param bytes byte array
	 * @throws IOException
	 */
	private static void safeWriteBytes(ByteArrayOutputStream bs, byte[] bytes) throws IOException
	{
	   if (bytes != null)
	      bs.write(bytes);
	}
	
	/**
	 * Test cipher with given ID
	 * 
	 * @param cipherId
	 * @return true if cipher is available and working correctly
	 */
	public static boolean testCipher(int cipherId)
	{
	   if (CIPHERS[cipherId] == null)
	      return false;
	   
	   try
	   {
         KeyGenerator keyGen = KeyGenerator.getInstance(CIPHERS[cipherId]);
         keyGen.init(KEY_LENGTHS[cipherId]);
         SecretKey key = keyGen.generateKey();
         
         Cipher cipher = Cipher.getInstance(CIPHERS[cipherId] + CIPHER_MODE);
         
         int blockSize = cipher.getBlockSize();
         byte[] ivBytes = new byte[(blockSize > 0) ? blockSize : 16];
         SecureRandom random = SecureRandom.getInstance("SHA1PRNG");
         random.nextBytes(ivBytes);
         IvParameterSpec iv = new IvParameterSpec(ivBytes);
         
         ByteArrayOutputStream bs = new ByteArrayOutputStream(128);
	   
         cipher.init(Cipher.ENCRYPT_MODE, key, iv);
         safeWriteBytes(bs, cipher.update(TEST_BYTES));
         safeWriteBytes(bs, cipher.doFinal());
         byte[] encryptedBytes = bs.toByteArray();
         
         bs.reset();
         cipher = Cipher.getInstance(CIPHERS[cipherId] + CIPHER_MODE);
         cipher.init(Cipher.DECRYPT_MODE, key, iv);
         safeWriteBytes(bs, cipher.update(encryptedBytes));
         safeWriteBytes(bs, cipher.doFinal());
         
         return Arrays.equals(TEST_BYTES, bs.toByteArray());
	   }
	   catch(Exception e)
	   {
	      return false;
	   }
	}
	
	/**
	 * Create encryption context based on information from session key request message.
	 * 
	 * @param request session key request message
	 * @return encryption context
	 * @throws NXCPException if encryption context cannot be created
	 */
	public static EncryptionContext createInstance(NXCPMessage request) throws NXCPException
	{
		int serverCiphers = request.getFieldAsInt32(NXCPCodes.VID_SUPPORTED_ENCRYPTION);
		int selectedCipher = -1;
		for(int i = 0; i < CIPHERS.length; i++)
		{
			if ((CIPHERS[i] == null) || !cipherTests[i] || ((serverCiphers & (1 << i)) == 0))  // not supported by client or server
				continue;
			
			try
			{
				Cipher.getInstance(CIPHERS[i] + CIPHER_MODE);
				if (Cipher.getMaxAllowedKeyLength(CIPHERS[i] + CIPHER_MODE) >= KEY_LENGTHS[i])
				{
					selectedCipher = i;
					break;
				}
			}
			catch(Exception e)
			{
			}
		}

		if (selectedCipher != -1)
		{
			try
			{
				return new EncryptionContext(selectedCipher, request);
			}
			catch(Exception e)
			{
				throw new NXCPException(NXCPException.NO_CIPHER, e);
			}
		}
		else
		{
			throw new NXCPException(NXCPException.NO_CIPHER);
		}
	}
	
	/**
	 * Internal constructor
	 * 
	 * @param cipher cipher to use
	 */
	protected EncryptionContext(int cipher, NXCPMessage request) throws GeneralSecurityException
	{
		this.cipher = cipher;
		keyLength = KEY_LENGTHS[cipher];
		
		KeyGenerator keyGen = KeyGenerator.getInstance(CIPHERS[cipher]);
		keyGen.init(KEY_LENGTHS[cipher]);
		key = keyGen.generateKey();
		
		encryptor = Cipher.getInstance(CIPHERS[cipher] + CIPHER_MODE);
		decryptor = Cipher.getInstance(CIPHERS[cipher] + CIPHER_MODE);
      
		int blockSize = encryptor.getBlockSize();
      byte[] ivBytes = new byte[(blockSize > 0) ? blockSize : 16];
      SecureRandom random = SecureRandom.getInstance("SHA1PRNG");
      random.nextBytes(ivBytes);
      iv = new IvParameterSpec(ivBytes);
	
      if (request != null)
      {
         byte[] pkeyBytes = request.getFieldAsBinary(NXCPCodes.VID_PUBLIC_KEY);
         serverPublicKey = KeyFactory.getInstance("RSA").generatePublic(new X509EncodedKeySpec(pkeyBytes));
      }
	}	

	/**
	 * Encrypt session key with public key from encryption setup message.
	 * 
	 * @return encrypted session key
	 * @throws GeneralSecurityException 
	 */
	public byte[] getEncryptedSessionKey() throws GeneralSecurityException
	{
		Cipher cipher = Cipher.getInstance("RSA/ECB/OAEPWithSHA1AndMGF1Padding");
		cipher.init(Cipher.ENCRYPT_MODE, serverPublicKey);
		return cipher.doFinal(key.getEncoded());
	}

	/**
	 * Encrypt initialization vector with public key from encryption setup message.
	 * 
	 * @return encrypted initialization vector
	 * @throws GeneralSecurityException 
	 */
	public byte[] getEncryptedIv() throws GeneralSecurityException
	{
		Cipher cipher = Cipher.getInstance("RSA/ECB/OAEPWithSHA1AndMGF1Padding");
		cipher.init(Cipher.ENCRYPT_MODE, serverPublicKey);
		return cipher.doFinal(iv.getIV());
	}
	
	/**
	 * Create and encrypt payload header for encrypted message
	 * 
	 * @param msgBytes original message
	 * @return encrypted data block or null if there are not enough data to produce complete encrypted block
	 * @throws IOException
	 */
	private byte[] encryptPayloadHeader(byte[] msgBytes) throws IOException
	{
		ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
		DataOutputStream outputStream = new DataOutputStream(byteStream);
		
		CRC32 crc32 = new CRC32();
		crc32.update(msgBytes);
		outputStream.writeInt((int)crc32.getValue());		
		outputStream.writeInt(0);		// reserved
		
		return encryptor.update(byteStream.toByteArray());
	}
	
	/**
	 * Encrypt NXCP message.
	 * 
	 * @param msg message to encrypt
	 * @param allowCompression true if payload compression is allowed
	 * @return encrypted message as sequence of bytes, ready to send over the network
	 * @throws IOException 
	 * @throws GeneralSecurityException 
	 * @throws InvalidKeyException 
	 */
	public byte[] encryptMessage(NXCPMessage msg, boolean allowCompression) throws IOException, GeneralSecurityException
	{
		final byte[] msgBytes = msg.createNXCPMessage(allowCompression);
		
		ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
		DataOutputStream outputStream = new DataOutputStream(byteStream);

		outputStream.writeShort(NXCPCodes.CMD_ENCRYPTED_MESSAGE); // wCode
		outputStream.writeByte(0);		// nPadding
		outputStream.writeByte(0);		// reserved
		outputStream.writeInt(0);		// length
		
		synchronized(encryptor)
		{
			encryptor.init(Cipher.ENCRYPT_MODE, key, iv);
			byte[] ph = encryptPayloadHeader(msgBytes);
			if (ph != null)
				outputStream.write(ph);
			
			outputStream.write(encryptor.update(msgBytes));
			outputStream.write(encryptor.doFinal());
		}

		int padding = (8 - (byteStream.size() % 8)) & 7;
		for (int i = 0; i < padding; i++)
			outputStream.writeByte(0);
		
		final byte[] encryptedMessage = byteStream.toByteArray();
		encryptedMessage[2] = (byte)padding;

		// update message length field
		encryptedMessage[4] = (byte)(encryptedMessage.length >> 24); 
		encryptedMessage[5] = (byte)((encryptedMessage.length >> 16) & 0xFF); 
		encryptedMessage[6] = (byte)((encryptedMessage.length >> 8) & 0xFF); 
		encryptedMessage[7] = (byte)(encryptedMessage.length & 0xFF); 

		return encryptedMessage; 
	}
	
	/**
	 * Decrypt message from input stream
	 * 
	 * @param inputStream input stream
	 * @param length length of encrypted message
	 * @return decrypted message
	 * @throws GeneralSecurityException
	 * @throws IOException
	 */
	public byte[] decryptMessage(NXCPDataInputStream inputStream, int length) throws GeneralSecurityException, IOException
	{
		ByteArrayOutputStream byteStream = new ByteArrayOutputStream();
		byte[] buffer = new byte[4096];
		int bytes = length;
		synchronized(decryptor)
		{
			decryptor.init(Cipher.DECRYPT_MODE, key, iv);
			while(bytes > 0)
			{
				int bytesRead = inputStream.read(buffer, 0, Math.min(buffer.length, bytes));
				byteStream.write(decryptor.update(buffer, 0, bytesRead));
				bytes -= bytesRead;
			}
			byteStream.write(decryptor.doFinal());
		}
		return byteStream.toByteArray();
	}
	
	/**
	 * Get cipher
	 * 
	 * @return cipher
	 */
	public int getCipher()
	{
		return cipher;
	}
	
	/**
	 * Get key length (in bytes)
	 * 
	 * @return key length (in bytes)
	 */
	public int getKeyLength()
	{
		return keyLength / 8;
	}
	
	/**
	 * Get initialization vector length (in bytes)
	 * 
	 * @return initialization vector length (in bytes)
	 */
	public int getIvLength()
	{
		return iv.getIV().length;
	}

	/**
	 * Get fingerprint of server public key
	 * 
	 * @return fingerprint of server public key
	 */
	public String getServerKeyFingerprint()
	{
	   if (serverPublicKey == null)
	      return "none";
	   
      try
      {
         MessageDigest m = MessageDigest.getInstance("MD5");
         byte[] key = serverPublicKey.getEncoded();
         m.update(key, 0, key.length);
         
         StringBuilder sb = new StringBuilder();
         for(int b : m.digest())
         {
            if (sb.length() > 0)
               sb.append(':');
            sb.append(String.format("%02x", b < 0 ? 129 + b : b));
         }
         return sb.toString();
      }
      catch(NoSuchAlgorithmException e)
      {
         return "error";
      }
	}
}
