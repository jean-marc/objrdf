<?xml version="1.0"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:s='http://www.w3.org/2005/sparql-results#'
	xmlns='http://www.w3.org/1999/xhtml'
	xmlns:xlink='http://www.w3.org/1999/xlink'
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
>
<xsl:template match='/'>
<div id='jm'>
<table>
<xsl:apply-templates/>
</table>
</div>
</xsl:template>
<xsl:template match='s:sparql'>
	<table>
	<xsl:apply-templates select='s:head'/>
	<xsl:apply-templates select='s:results/s:result'/>
	</table>
</xsl:template>
<xsl:template match='s:head'>
	<tr><xsl:apply-templates select='s:variable'/></tr>
</xsl:template>
<xsl:template match='s:variable'><th><xsl:value-of select='@name'/></th>
</xsl:template>
<xsl:template match='s:result'><tr><xsl:apply-templates/></tr></xsl:template>
<xsl:template match='s:binding'><xsl:apply-templates/></xsl:template>
<xsl:template match='s:uri'><td><a href='#'><xsl:value-of select='.'/></a></td></xsl:template>
<xsl:template match='s:literal'><td><xsl:apply-templates/></td></xsl:template>
<xsl:template match='rdf:RDF'>
	<xsl:apply-templates mode='resource'/>
</xsl:template>
<xsl:template match='*' mode='resource'>
	<!--
<xsl:template match='*[@rdf:about]' mode='resource'>
	<tr>
	<td><a href="{concat('/sparql?query=DESCRIBE &lt;',substring-before(@rdf:about,'#'),'%23',substring-after(@rdf:about,'#'),'&gt;')}"><xsl:value-of select='@rdf:about'/></a></td>
	-->
	<xsl:apply-templates select='*' mode='property'/>
	<!--
	</tr>
	<xsl:call-template name='statement'>
		<xsl:with-param name='subject' select='@rdf:about'/>
	</xsl:call-template>
	-->
</xsl:template>
<!--
<xsl:template match='*[@rdf:ID]' mode='resource'>
	<tr>
	<td><a href="{concat('/sparql?query=DESCRIBE &lt;',@rdf:ID,'&gt;')}"><xsl:value-of select='@rdf:ID'/></a></td>
	<xsl:apply-templates select='*' mode='property'/>
	</tr>
</xsl:template>
-->
<xsl:template match='*[@rdf:about]' mode='uri'><xsl:value-of select='@rdf:about'/></xsl:template>
<xsl:template match='*[@rdf:ID]' mode='uri'><a href='/data/{@rdf:ID}'><xsl:value-of select='@rdf:ID'/></a></xsl:template>
<xsl:template match='*[@rdf:resource]' mode='property'>
<!--
	we need to get URI for property
	INSERT DATA 
-->
	<tr>
	<td><xsl:apply-templates select='..' mode='uri'/></td>
	<!--<td><xsl:value-of select="concat(namespace-uri(),local-name())"/></td>-->
	<td><xsl:value-of select='name()'/></td>
	<!--<td><input type='text' value='{@rdf:resource}'/></td>-->
	<td><a href='#'><xsl:value-of select='@rdf:resource'/></a></td>
	<td><a href='#' onclick='edit(this)'>edit</a></td>
	</tr>
</xsl:template>
<xsl:template match='*' mode='property'>
	<tr>
	<td><xsl:apply-templates select='..' mode='uri'/></td>
	<td><xsl:value-of select='name()'/></td>
	<!--<td><xsl:value-of select='text()'/></td>-->
	<td><xsl:apply-templates/></td>
	</tr>
</xsl:template>	
<xsl:template match='node()'>
	<xsl:copy-of select='.'/>
</xsl:template>
</xsl:stylesheet>
