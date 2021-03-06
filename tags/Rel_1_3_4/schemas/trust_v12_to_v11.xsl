<?xml version="1.0" encoding="UTF-8"?>
<!--
	v12_to_v11_trust.xsl
	
	XSL stylesheet converting a Shibboleth 1.2 trust metadata file into the equivalent for
	Shibboleth 1.1.

	Author: Ian A. Young <ian@iay.org.uk>

	$Id$
-->
<xsl:stylesheet version="1.0"
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:shibb10="urn:mace:shibboleth:1.0"
	xmlns:trust10="urn:mace:shibboleth:trust:1.0"
	xmlns:ds="http://www.w3.org/2000/09/xmldsig#"
	xmlns="urn:mace:shibboleth:1.0"
	exclude-result-prefixes="shibb10 trust10">
	
	<!--
		Version information for this file.  Remember to peel off the dollar signs
		before dropping the text into another versioned file.
	-->
	<xsl:param name="cvsId">$Id$</xsl:param>

	<!--
		Add a comment to the start of the output file.
	-->
	<xsl:template match="/">
		<xsl:comment>
			<xsl:text>&#10;&#9;***DO NOT EDIT THIS FILE***&#10;&#10;</xsl:text>
			<xsl:text>&#9;Converted by:&#10;&#10;&#9;</xsl:text>
			<xsl:value-of select="substring-before(substring-after($cvsId, ': '), '$')"/>
			<xsl:text>&#10;</xsl:text>
		</xsl:comment>
		<xsl:apply-templates/>
	</xsl:template>

	<!--Force UTF-8 encoding for the output.-->
	<xsl:output omit-xml-declaration="no" method="xml" encoding="UTF-8" indent="yes"/>

	<!--trust10:Trust is the root element for the trust file.  Process it by changing the default namespace used and recursing.-->
	<xsl:template match="trust10:Trust">
		<Trust>
			<!-- <xsl:apply-templates select="@*"/> -->
			<xsl:apply-templates/>
		</Trust>
	</xsl:template>

	<!--trust10:KeyAuthority appears in the trust file, and needs its namespace changing.  After that, we need to reorder its nested elements a little.-->
	<xsl:template match="trust10:KeyAuthority">
		<KeyAuthority>
			<xsl:apply-templates select="ds:KeyInfo"/>
			<Subject>
				<xsl:value-of select="ds:KeyName"/>
			</Subject>
		</KeyAuthority>
	</xsl:template>

	<!--
		Generic recursive copy for ds:* elements.
		
		This works better than an xsl:copy-of because it does not copy across spurious
		namespace nodes.
	-->
	<xsl:template match="ds:*">
		<xsl:element name="{name()}">
			<xsl:apply-templates select="ds:* | text() | comment() | @*"/>
		</xsl:element>
	</xsl:template>

	<!--By default, copy text blocks, comments and attributes unchanged.-->
	<xsl:template match="text()|comment()|@*">
		<xsl:copy/>
	</xsl:template>

	<!--By default, copy all elements from the input to the output, along with their attributes and contents.-->
	<xsl:template match="*">
		<xsl:copy>
			<xsl:apply-templates select="node()|@*"/>
		</xsl:copy>
	</xsl:template>

</xsl:stylesheet>

