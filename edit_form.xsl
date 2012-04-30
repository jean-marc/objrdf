<?xml version="1.0"?>
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:s='http://www.w3.org/2005/sparql-results#'
	xmlns='http://www.w3.org/1999/xhtml'
	xmlns:xlink='http://www.w3.org/1999/xlink'
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
>
<!--
	how do we get here? describe or select?

	describe <id>

	select * where {<id> ?p ?v .} 

	this is not going to show properties that are not set

	alternatively we could have the server create the forms (some extra-work but would be really fast and robust)

-->
<!--
	we can extract the type from the query result
-->
<xsl:param name='id' select="string('?')"/>
<xsl:variable name='type' select="s:sparql/s:results/s:result[1]/s:binding[@name='v']/s:uri"/>
<xsl:variable name='esc_type' select="concat(substring-before($type,'#'),'%23',substring-after($type,'#'))"/>
<!-- 
	could be more specific, we don't need all the literal properties, only the ones relevant to
	the current class
	we could also split literal and non-literal properties
	a lot of queries could be cached on server

-->
<xsl:variable
	name='literals'
	select="document('/sparql?query=select * where {?s rdfs:subClassOf rdf:Literal .}')/s:sparql/s:results/s:result/s:binding/s:uri"
/>
<xsl:variable
	name='properties'
	select="document(concat('/sparql?query=select * where {?p rdfs:domain &lt;',$esc_type,'&gt; . ?p rdfs:range ?r .}'))"
/>
<xsl:variable
	name='values'
	select='.'
/>	
<!--
	could also query rdfs:comment for properties but we need support for optional in sparql first
-->
<xsl:template match="s:results">
<html>
<head>
<script src="jquery.js" type="text/javascript"></script>
<style>
body{
	font-family:arial;
}
</style>

</head>
<body>
<h1>Edit <xsl:value-of select="substring-after($type,'#')"/></h1>
<form name='add' method='get'>
<table>
<tr>
<!--
	SPARQL does not allow modifying the ID but we could define
	a pseudo-attribute to work around that limitation
	it would also allow to pass the ID in the sparql reply
-->
<td>ID:</td><td><input type='text' name='ID' value='{$id}'/></td></tr>
<xsl:call-template name='form'>
<xsl:with-param name='properties' select='$properties'/>
</xsl:call-template>
<!--<xsl:apply-templates/>-->
<tr><input type='submit' value='submit'/></tr>
</table>
</form>
[<a href='/sparql?query=describe &lt;{$id}&gt;'>rdf</a>]
<!--
	insert data {<id> a <some_type>;<p_0> <v_0>;... .}
	need to go through all the input
-->
<script type="text/javascript">
$("form").submit(function(){
	//it's good to send type along to speed up query, even better use special id
	var s='insert data {&lt;'+$('form input:first').val()+'&gt; a &lt;<xsl:value-of select='$type'/>&gt;'
	$('form input[type!=submit][name!=ID]').each(function(){
		if($(this).attr('value')!=$(this).val())
			s+=';&lt;'+$(this).attr('name')+'&gt; '+$(this).val();
	});	
	$('form select').each(function(){
		//need to limit scope!!!
		if($('option[selected=true]').attr('value')!=$(this).val())
			s+=';&lt;'+$(this).attr('name')+'&gt; &lt;'+$(this).val()+'&gt;';
	});
	s+=' .}'
	$.post('/',s,function(){})
	//alert(s)
})
</script>
</body>
</html>
</xsl:template>

<xsl:template name='form'>
<!--
	3 parameters
	select * where {?p rdfs:domain <type> .?p rdfs:range ?r .}
	select * where {<id> ?p ?v .}
	select * where {?p rdfs:subClassOf rdf:Literal .}
-->
<xsl:param name='properties'/>
	<xsl:apply-templates select='$properties/s:sparql/s:results/*'/>
</xsl:template>

<xsl:template match="s:result">
<!--
	we want to create an INSERT DATA { } query but we need to know the type

	the HTML way to send a form is 
	http://server/page?a=0&b=1&c=2&...
	we could handle that on the server and create the SPARQL query or use javascript

-->
<xsl:variable name='tmp' select='.'/>
<xsl:variable name='range' select="s:binding[@name='r']/s:uri"/>
<xsl:variable name='esc_range' select="concat(substring-before($range,'#'),'%23',substring-after($range,'#'))"/>
<!--
	we have to look-up the value
-->
<xsl:variable name='p' select="$values/s:sparql/s:results/s:result[s:binding[@name='p']/s:uri=$tmp/s:binding[@name='p']/s:uri]"/>
<tr>
<td><xsl:value-of select="substring-after(s:binding[@name='p']/s:uri,'#')"/>:</td>
<td>
<xsl:choose>
	<xsl:when test="$literals[text()=$tmp/s:binding[@name='r']/s:uri/text()]">
		<input type='text' name="{$tmp/s:binding[@name='p']/s:uri}" value="{$p/s:binding[@name='v']/s:literal}"/>
	</xsl:when>
	<xsl:otherwise>
		<select name="{$tmp/s:binding[@name='p']/s:uri}">
		<option value='nil'>--</option>
		<!-- we also ask for the type because of entailment we will get sub-classes, we can use <optgroup/> --> 
		
		<!--<xsl:for-each select="document(concat('/sparql?query=select * where {?s a &lt;',substring-before($range,'#'),'%23',substring-after($range,'#'),'&gt;;a ?t .}'))/s:sparql/s:results/s:result/s:binding/s:uri">-->
		<!-- we should have a nil entry if not selected -->
		<xsl:for-each select="document(concat('/sparql?query=select * where {?s a &lt;',$esc_range,'&gt; .}'))/s:sparql/s:results/s:result/s:binding/s:uri">
			<!-- we have to add selected attribute if the value is set -->
			<xsl:choose>
				<xsl:when test="text()=$p/s:binding[@name='v']/s:uri/text()">
					<option value='{.}' selected='true'><xsl:value-of select="."/></option>
				</xsl:when>
				<xsl:otherwise>
					<option value='{.}'><xsl:value-of select="."/></option>
				</xsl:otherwise>
			</xsl:choose>	
		</xsl:for-each>
		</select>
	</xsl:otherwise>
</xsl:choose>
</td>
</tr>
</xsl:template>
</xsl:stylesheet>
