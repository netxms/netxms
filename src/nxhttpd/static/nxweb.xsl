<?xml version="1.0" encoding="UTF-8"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:fo="http://www.w3.org/1999/XSL/Format">
	<xsl:output method="html" />
	<xsl:include href="global.xsl" />
	
	<xsl:template match="/">
		<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="en" lang="en-GB">
			<head>
				<title><xsl:value-of select="/nxweb/title" /></title>
				<link rel="stylesheet" type="text/css" href="/screen.css" media="screen, tv, projection" title="Default" />
				<xsl:comment>[if gte IE 5.5000]>
					&lt;script type="text/javascript" src="/pngfix.js"&gt;&lt;/script&gt;
				&lt;![endif]</xsl:comment>
			</head>

			<body onLoad="e = document.getElementById('focus_on_load'); if (e) e.focus();">
				<!-- hide menu on login page -->
				<xsl:if test="$sid">
					<div id="nav_menu">
						<ul>
							<li><a href="/netxms.app?cmd=overview&amp;sid={$sid}">Overview</a></li>
							<li><a href="/netxms.app?cmd=alarms&amp;sid={$sid}">Alarms</a></li>
							<li><a href="/netxms.app?cmd=objects&amp;sid={$sid}">Objects</a></li>
							<!-- <li><a href="/netxms.app?cmd=reports&amp;sid={$sid}">Reports</a></li>-->
	
							<!-- right part of menu -->
							<li style="float: right"><a href="/netxms.app?cmd=logout&amp;sid={$sid}">Logout</a></li>
						</ul>
					</div>
				</xsl:if>

				<div id="main-copy">
					<xsl:apply-templates select="/nxweb/login" />
					<xsl:apply-templates select="/nxweb/overview" />
					<xsl:apply-templates select="/nxweb/alarms" />
					<xsl:apply-templates select="/nxweb/objects" />
				</div>
			</body>
		</html>
	</xsl:template>

	<xsl:template match="login">
			<form action="/netxms.app" method="post">
				<input type="hidden" name="cmd" value="login" />
				<div id="horizon">
					<div id="content">
						<table id="login_dialog" border="0" cellpadding="4">
							<tr>
								<td colspan="3">
									<img src="/images/login.jpg" />
								</td>
							</tr>
							<tr>
								<td colspan="3">
									<hr noshade="" width="90%" align="center" />
								</td>
							</tr>

							<tr>
								<td align="center">
									<table border="0" cellpadding="1">
										<tr>
											<td>
												<strong>Login:</strong>
											</td>
											<td>
												<input id="focus_on_load" name="user" tabindex="1" accesskey="l" type="text" size="21" />
											</td>
											<td rowspan="2" style="padding: 1em;">
												<input tabindex="3" accesskey="s" type="submit" value="Login" />
											</td>
										</tr>
										<tr>
											<td>
												<strong>Password:</strong>
											</td>
											<td>
												<input name="pwd" tabindex="2" accesskey="p" type="password" size="21" />
											</td>
										</tr>
									</table>
								</td>
							</tr>
						</table>
					</div>
				</div>
			</form>
	</xsl:template>
	
	<xsl:template match="overview">
		<div class="full">
			<h1>Current server status:</h1>
			<table border="1" cellpadding="4" width="96%">
				<xsl:for-each select="server/item">
					<tr>
						<td width="30%"><xsl:value-of select="@name" /></td>
						<td><xsl:value-of select="." /></td>
					</tr>
				</xsl:for-each>
			</table>
			
			<xsl:if test="alarms">
				<p />
				<h1>Alarms distribution by priority:</h1>
	
				<xsl:variable name="normal" select="alarms/normal" />
				<xsl:variable name="warning" select="alarms/warning" />
				<xsl:variable name="minor" select="alarms/minor" />
				<xsl:variable name="major" select="alarms/major" />
				<xsl:variable name="critical" select="alarms/critical" />
				<img src="/pie.png?l0=Normal&amp;v0={$normal}&amp;l1=Warning&amp;v1={$warning}&amp;l2=Minor&amp;v2={$minor}&amp;l3=Major&amp;v3={$major}&amp;l4=Critical&amp;v4={$critical}" />
			</xsl:if>
		</div>
	</xsl:template>

	<xsl:template match="alarms">
		<div class="full">
			<xsl:choose>
				<xsl:when test="count(alarm) = 0">
					<i><small>no alarms to display</small></i>
				</xsl:when>
				<xsl:otherwise>
					<table border="1" cellpadding="4" width="96%" id="alarms">
						<tr>
							<th>Severity</th>
							<th>Source</th>
							<th>Message</th>
							<th>Timestamp</th>
							<th>Ack</th>
						</tr>


				<xsl:for-each select="alarm">
					<xsl:choose>
						<!-- move to template!!! -->
						<xsl:when test="position() mod 2">
							<tr id="odd">
								<xsl:variable name="alarm_id" select="@id" />
								<xsl:variable name="severity" select="@severity" />
		
								<td>
									<nobr><img src="/images/status_{$severity}.png" alt="Status: {$severity}"/>&#160;<xsl:value-of select="$severity" /></nobr>
								</td>
								<td><xsl:value-of select="@source" /></td>
								<td><xsl:value-of select="." /></td>
								<td><nobr><xsl:value-of select="@timestamp" /></nobr></td>
								<td align="center"><a href="/netxms.app?cmd=alarms&amp;sid={$sid}&amp;ack={$alarm_id}"><img src="/images/ack.png" alt="Acknowledge alarm" title="Acknowledge alarm" width="16" height="16" /></a></td>
							</tr>
						</xsl:when>
						<xsl:otherwise>
							<tr id="even">
								<xsl:variable name="alarm_id" select="@id" />
								<xsl:variable name="severity" select="@severity" />
		
								<td>
									<nobr><img src="/images/status_{$severity}.png" alt="Status: {$severity}"/>&#160;<xsl:value-of select="$severity" /></nobr>
								</td>
								<td><xsl:value-of select="@source" /></td>
								<td><xsl:value-of select="." /></td>
								<td><nobr><xsl:value-of select="@timestamp" /></nobr></td>
								<td align="center"><a href="/netxms.app?cmd=alarms&amp;sid={$sid}&amp;ack={$alarm_id}"><img src="/images/ack.png" alt="Acknowledge alarm" title="Acknowledge alarm" width="16" height="16" /></a></td>
							</tr>
						</xsl:otherwise>
					</xsl:choose>
				</xsl:for-each>
					</table>
				</xsl:otherwise>
			</xsl:choose>
		</div>
	</xsl:template>

	<xsl:template match="objects">
		<script type="text/javascript" src="/xtree.js"></script>
		<script type="text/javascript" src="/xmlextras.js"></script>
		<script type="text/javascript" src="/xloadtree.js"></script>
		
		<div class="left">
			<div id="jsTree"></div>
		</div>
		<div class="right">
			<div id="objectData"></div>
		</div>

		<script type="text/javascript">
			var net = new WebFXLoadTree("Object Browser Root", "/netxms.app?cmd=objectsList&amp;sid=<xsl:value-of select="$sid" />");
			document.getElementById("jsTree").innerHTML = net;

			function showObjectInfo(id)
			{
				var xmlHttp = XmlHttp.create();
				xmlHttp.open("GET", "/netxms.app?cmd=objectInfo&amp;sid=<xsl:value-of select="$sid" />&amp;id=" + id, false);
				xmlHttp.send(null);
				document.getElementById("objectData").innerHTML = xmlHttp.responseText;
			}
		</script>
	</xsl:template>

</xsl:stylesheet>
