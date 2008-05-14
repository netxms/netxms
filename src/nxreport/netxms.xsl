<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format">
	<xsl:template match="/NETXMS">
		<html><body>
			<xsl:apply-templates />
		</body></html>
	</xsl:template>
	
	<xsl:template match="CONTAINER">
		<table cellpadding="0" cellspasing="0">
			<tr>
				<td valign="top">CONTAINER:</td>
				<td>
					<strong><xsl:value-of select="@name" /></strong>
					<table>
						<xsl:apply-templates select="NODE"/>
					</table>
					<xsl:apply-templates select="CONTAINER"/>
				</td>
			</tr>
		</table>
	</xsl:template>
	
	<xsl:template match="NODE">
		<tr>
			<td valign="top">NODE: <strong><xsl:value-of select="@name" /></strong></td>
			<table>
				<xsl:apply-templates select="DCI"/>
			</table>
		</tr>
		<hr />
	</xsl:template>
	
	<xsl:template match="DCI">
		<tr>
			<td>DCI name: <strong><xsl:value-of select="@name" /></strong></td>
			<td>DCI description: <strong><xsl:value-of select="." /></strong></td>
		</tr>
	</xsl:template>
</xsl:stylesheet>
