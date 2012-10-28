/**
 * NetXMS - open source network management system
 * Copyright (C) 2003-2010 Victor Kirhenshtein
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
package org.netxms.client;

import java.util.HashMap;
import java.util.Map;
import org.netxms.api.client.NetXMSClientException;

/**
 * NetXMS client library exception. Used to report API call errors.
 */
public class NXCException extends NetXMSClientException
{
	private static final long serialVersionUID = -3247688667511123892L;
	private static final String[] errorTexts_en =
	{
      "Request completed successfully",
      "Component locked",
      "Access denied",
      "Invalid request",
      "Request timed out",
      "Request is out of state",
      "Database failure",
      "Invalid object ID",
      "Object already exist",
      "Communication failure",
      "System failure",
      "Invalid user ID",
      "Invalid argument",
      "Duplicate DCI",
      "Invalid DCI ID",
      "Out of memory",
      "Input/Output error",
      "Incompatible operation",
      "Object creation failed",
      "Loop in object relationship detected",
      "Invalid object name",
      "Invalid alarm ID",
      "Invalid action ID",
      "Operation in progress",
      "Copy operation failed for one or more DCI(s)",
      "Invalid or unknown event code",
      "No interfaces suitable for sending magic packet",
      "No MAC address on interface",
      "Command not implemented",
      "Invalid trap configuration record ID",
      "Requested data collection item is not supported by agent",
      "Client and server versions mismatch",
      "Error parsing package information file",
      "Package with specified properties already installed on server",
      "Package file already exist on server",
      "Server resource busy",
      "Invalid package ID",
      "Invalid IP address",
      "Action is used in event processing policy",
      "Variable not found",
      "Server uses incompatible version of communication protocol",
      "Address already in use",
      "Unable to select cipher",
      "Invalid public key",
      "Invalid session key",
      "Encryption is not supported by peer",
      "Server internal error",
      "Execution of external command failed",
      "Invalid object tool ID",
      "SNMP protocol error",
      "Incorrect regular expression",
      "Parameter is not supported by agent",
      "File I/O operation failed",
      "MIB file is corrupted",
      "File transfer operation already in progress",
      "Invalid job ID",
      "Invalid script ID",
      "Invalid script name",
      "Unknown map name",
      "Invalid map ID",
      "Account disabled",
      "No more grace logins",
      "Server connection broken",
      "Invalid agent configuration ID",
      "Server has lost connection with backend database",
      "Alarm is still open in helpdesk system",
      "Alarm is not in \"outstanding\" state",
      "DCI data source is not a push agent",
      "Error parsing configuration import file",
      "Configuration cannot be imported because of validation errors",
		"Invalid graph ID",
		"Local cryptographic provider failure",
		"Unsupported authentication type",
		"Bad certificate",
		"Invalid certificate ID",
		"SNMP failure",
		"Node has no support for layer 2 topology discovery",
		"Invalid situation ID",
		"Named instance not found",
		"Invalid event ID",
		"Operation cannot be completed due to agent error",
		"Unknown variable",
		"Requested resource not available",
		"Job cannot be cancelled",
		"Invalid policy ID",
		"Unknown log name",
		"Invalid log handle",
		"New password is too weak",
		"Password was used before",
		"Invalid session handle",
		"Node already is a member of a cluster",
		"Job cannot be put on hold",
		"Job on hold cannot be resumed",
		"Zone ID is already in use",
		"Invalid zone ID",
		"Cannot delete non-empty zone object",
		"No physical component data",
		"Invalid alarm note ID",
		"Encryption error"
	};
	private static final String[] extendedErrorTexts_en =
	{
		"Bad MIB file header",
		"Bad MIB file data"
	};
	
	private static final String[] errorTexts_es =
	{
      "Petición completada con éxito",
      "Componente bloqueado",
      "Acceso denegado",
      "Petición inválida",
      "Caducidad de la petición",
      "La petición está fuera de estado",
      "Fallo en la base de datos",
      "ID de objeto inválido",
      "El objeto ya existe",
      "Fallo en la comunicación",
      "Fallo del sistema",
      "ID de usuario inválido",
      "Argumento inválido",
      "DCI duplicado",
      "ID de DCI inválido",
      "Sin memoria",
      "Error de Entrada/Salida",
      "Operación incompatible",
      "La creación del objeto ha fallado",
      "Se ha detectado un bucle en las relaciones del objeto",
      "Nombre de objeto inválido",
      "ID de alarma inválido",
      "ID de acción inválido",
      "Operación en progreso",
      "El copiado ha fallado en uno o más DCI(s)",
      "Código de evento inválido o desconocido",
      "Sin interfaces adecuados para enviar el paquete mágico",
      "Sin dirección MAC en el interfaz",
      "Comando no implementado",
      "ID de registro de configuración de traps inválido",
      "La petición del monitor no está soportada por el agente",
      "Las versiones del cliente y del servidor son incompatibles",
      "Error al analizar el fichero de información del paquete",
      "El paquete con las propiedades especificadas ya está instalado en el servidor",
      "El fichero de paquete ya existe en el servidor",
      "El recurso del servidor está ocupado",
      "ID de paquete inválido",
      "Dirección IP inválida",
      "La acción es utilizada en una política de tratamiento de eventos",
      "Variable no encontrada",
      "El servidor utiliza un protocolo de comunicaciones incompatible",
      "La dirección ya está utilizada",
      "No es posible seleccionar cifrado",
      "Clave pública inválida",
      "Clave de sesión inválida",
      "El extremo no soporta cifrado",
      "Error interno en el servidor",
      "La ejecución del comando externo ha fallado",
      "ID de herramienta inválido",
      "Error en el protocolo SNMP",
      "Expresión regular incorrecta",
      "El parámetro no está soportado por el agente",
      "La operación de E/S del fichero ha fallado",
      "El fichero MIB está corrupto",
      "La transferencia del fichero está en progreso",
      "ID de tarea inválido",
      "ID de script inválido",
      "Nombre de script inválido",
      "Nombre de mapa desconocido",
      "ID de mapa inválido",
      "Cuenta desactivada",
      "No se permiten más intentos de conexión",
      "Conexión al servidor interrumpida",
      "ID de configuración del agente inválido",
      "El servidor ha perdido la conexión con la base de datos",
      "La alarma está todavía abierta en el sistema de soporte",
      "La alarma no está en estado \"pendiente\"",
      "El origen de datos del DCI no es un agente de envío",
      "Error al analizar el fichero para importar la configuración",
      "La configuración no puede importarse porque existen errores de validación",
      "ID de gráfico inválido",
      "Fallo en el proveedor criptográfico local",
      "Tipo de autenticación no soportada",
      "Certificado dañado",
      "ID de certificado inválido",
      "Fallo SNMP",
      "El nodo no soporta descubrimiento de topología en la capa 2",
      "ID de situación inválido",
      "No se ha encontrado la instancia indicada",
      "ID de evento inválido",
      "La operación no ha podido ser completada por un error en el agente",
      "Variable desconocida",
      "Petición de recurso no disponible",
      "La tarea no puede ser cancelada",
      "ID de política inválido",
      "Nombre del registro desconocido",
      "Handle del registro inválido",
      "La nueva contraseña es demasiado débil",
      "La contraseña ha sido utilizada anteriormente",
      "Handle de sesión inválido",
      "El nodo ya es miembro de un cluster",
      "La tarea no puede ser retenida",
      "La tarea retenida no puede ser reanudada",
      "El ID de zona ya está en uso",
      "ID de zona inválido",
      "No es posible eliminar zonas con algún contenido",
      "Sin datos en el componente físico",
      "ID del comentario de la alarma inválido",
      "Error de cifrado"
	};
	private static final String[] extendedErrorTexts_es =
	{
      "Error en la cabecera del fichero MIB",
      "Error en los datos del fichero MIB"
	};
	
	private static final Map<String, String[]> errorTexts;
	private static final Map<String, String[]> extendedErrorTexts;
	
	static
	{
		errorTexts = new HashMap<String, String[]>(2);
		errorTexts.put("en", errorTexts_en);
		errorTexts.put("es", errorTexts_es);
		
		extendedErrorTexts = new HashMap<String, String[]>(2);
		extendedErrorTexts.put("en", extendedErrorTexts_en);
		extendedErrorTexts.put("es", extendedErrorTexts_es);
	}

	/**
	 * @param errorCode
	 */
	public NXCException(int errorCode)
	{
		super(errorCode);
	}

	/**
	 * @param errorCode
	 * @param additionalInfo
	 */
	public NXCException(int errorCode, String additionalInfo)
	{
		super(errorCode, additionalInfo);
	}

	/* (non-Javadoc)
	 * @see org.netxms.api.client.NetXMSClientException#getErrorMessage(int)
	 */
	@Override
	protected String getErrorMessage(int code, String lang)
	{
		try
		{
			if (code > 1000)
			{
				String[] texts = extendedErrorTexts.get(lang);
				if (texts == null)
					texts = extendedErrorTexts_en;
				return texts[code - 1001];
			}
			else
			{
				String[] texts = errorTexts.get(lang);
				if (texts == null)
					texts = errorTexts_en;
				return texts[code];
			}
		}
		catch(ArrayIndexOutOfBoundsException e)
		{
			return "Error " + Integer.toString(code);
		}
	}
}
