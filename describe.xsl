<?xml version="1.0"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:s='http://www.w3.org/2005/sparql-results#'
	xmlns='http://www.w3.org/1999/xhtml'
	xmlns:xlink='http://www.w3.org/1999/xlink'
>
<xsl:template match='/*'>
<html>
<head>
<script type="text/javascript">
function update(predicate,value){
	request=new XMLHttpRequest();
	query='INSERT DATA {&lt;<xsl:value-of select='@id'/>&gt; &lt;'+predicate+'&gt; '+value+' .}';
	request.open("POST","/",false);
	request.send(query);
	location.reload();
}
</script>
<style>
body{
	font-family:Arial;

}
.comment{
	width:600px;
	background-color:#dddddd;
}

</style>
</head>
<body>
<!--<a href='/data/{@id}'><xsl:value-of select='@id'/></a>-->
<h1><xsl:value-of select='@id'/></h1>
<table>
<xsl:apply-templates select="*[not(name()='comment')]"/>
</table>
<xsl:apply-templates select='comment'/>
</body>
</html>
</xsl:template>
<xsl:template match='*[@resource]'>
<tr>
	<td><xsl:value-of select='name()'/></td>
	<td><a href="{@resource}"><xsl:value-of select='@resource'/></a></td>
	<td><input type='text' value='{@resource}' onchange='update("{name()}","&lt;"+this.value+"&gt;")'/></td>
</tr>
</xsl:template>
<xsl:template match='*'>
<tr>
	<td><xsl:value-of select='name()'/></td>
	<td/>
	<td><input type='text' value='{text()}' onchange='update("{name()}",this.value)'/></td>
</tr>
</xsl:template>
<xsl:template match='comment'>
<div class='comment'><xsl:copy-of select='node()'/></div>
</xsl:template>
<!--
<xsl:template match="*[type[@resource='Mod']]">
<svg width='200' height='200' xmlns='http://www.w3.org/2000/svg' xmlns:xlink='http://www.w3.org/1999/xlink'>
<style type='text/css'> 
circle{ fill:blue; } 
</style>
<defs>
<circle id='c' r='1'/>
</defs>
<xsl:apply-templates/>
</svg>
</xsl:template>
<xsl:template match='li' mode='constellation'>
<xsl:variable name='xy' select="substring-after(text(),'(')"/>
<xsl:variable name='x' select="substring-before($xy,',')"/>
<xsl:variable name='yy' select="substring-after($xy,',')"/>
<xsl:variable name='y' select="substring-before($yy,')')"/>
<use xlink:href='#c' x='{$x}' y='{$y}' xmlns='http://www.w3.org/2000/svg'/>
</xsl:template>
-->
</xsl:stylesheet>
