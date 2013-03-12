<?xml version="1.0"?>
<!--
	starting screen for inventory
	starts with a query for all Classes in inventory namespace and all instances of each class
-->
<xsl:stylesheet version="1.0" 
	xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
	xmlns:s='http://www.w3.org/2005/sparql-results#'
	xmlns='http://www.w3.org/1999/xhtml'
	xmlns:xlink='http://www.w3.org/1999/xlink'
	xmlns:rdf='http://www.w3.org/1999/02/22-rdf-syntax-ns#'
	xmlns:obj='http://www.example.org/objrdf#'
	xmlns:geo='http://www.w3.org/2003/01/geo/wgs84_pos#'
	xmlns:inv='http://inventory.unicefuganda.org/#'
	xmlns:svg='http://www.w3.org/2000/svg'
>
<xsl:template match='/'>
<html>
<head>
<script src="jquery.js" type="text/javascript"></script>
<script type="text/javascript">
$("#by_id").submit(function(){
	document.location.href='/sparql?query=describe &lt;'+$('#by_id input').val()+'&gt;&amp;xsl=inventory.xsl';
})
$("#by_site").submit(function(){
	document.location.href='/sparql?query=prefix inv:&lt;http://inventory.unicefuganda.org/%23&gt; describe ?e where {?x inv:located &lt;'+$('#by_site input').val()+'&gt; .?e inv:partOf ?x .}&amp;xsl=inventory.xsl';
});
</script>
<link rel='stylesheet' type='text/css' href='inventory.css' media='all'/>
<link rel='stylesheet' type='text/css' href='inventory_print_medium.css' media='print'/>
<link rel='alternate stylesheet' type='text/css' href='inventory_print_big.css' title='big'/>
<link rel='alternate stylesheet' type='text/css' href='inventory_print_medium.css' title='medium'/>
<link rel='alternate stylesheet' type='text/css' href='inventory_print_small.css' title='small'/>
<!--
title should give more information
-->
<xsl:apply-templates select='/' mode='title'/>
</head>
<body>
<!-- different ways to search db: id, site, type,...-->
<!-- careful with {} it has special meaning for xsl -->
<div id='top'>
<a href="/sparql?query=select * where %7B?x a rdfs:Class .%7Dorder by ?x&amp;xsl=inventory.xsl">home</a> | 
search by id: <form id='by_id'><input type='text'/></form> |
search by site: <form id='by_site'><input type='text'/></form> |
<a href='/sparql?query=describe * where %7B?x a rdfs:Resource .%7D'>dump</a> in <a href='http://www.w3.org/RDF/'>RDF</a> | 
</div>
<!-- select query -->
<xsl:apply-templates select='s:sparql/s:results/s:result/s:binding/s:uri'/>
<!-- describe query -->
<xsl:apply-templates select='rdf:RDF/*'/>
</body>
</html>
</xsl:template>
<xsl:template match="s:uri[substring-before(.,'#')='http://inventory.unicefuganda.org/']">
<xsl:variable name='esc_type' select="concat(substring-before(.,'#'),'%23',substring-after(.,'#'))"/>
<!-- problem with chrome -->
<xsl:variable name='jm' select="document('jm')"/>
<!-- query all instances of each class -->
<!--<xsl:variable name='instances' select="document(concat('/sparql?query=select * where {?s a &lt;',$esc_type,'&gt; .}'))" />-->
<div class='jm'>
<!-- Chrome does not support xslt-param -->
<div class='header'>
<b><xsl:value-of select="substring-after(.,'#')"/></b> | 
<a href="{concat('/sparql?query=describe * where {?s a &lt;',$esc_type,'&gt; .}&amp;xsl=inventory.xsl')}">all</a> |
<a href="{concat('/sparql?query=select * where {?p rdfs:domain &lt;',$esc_type,'&gt; . ?p rdfs:range ?r .}&amp;xsl=inventory_add.xsl&amp;xslt-param-name=type&amp;xslt-param-value=',$esc_type)}" title="{concat('add a new ',substring-after(.,'#'))}">add</a> |
<!-- 
     a table representation 
     we need to get all the properties and 
-->
<xsl:variable name='properties' select="document(concat('/sparql?query=select * where {?p rdfs:domain &lt;',$esc_type,'&gt; .}'))//s:uri"/>
<!-- build a query 
	select * where {?x a type,

-->
<xsl:variable name='query'>select * where {?x a &lt;<xsl:value-of select='$esc_type'/>&gt;;a ?type<xsl:for-each select='$properties[position()&gt;2]'><xsl:variable name='esc_type_0' select="concat(substring-before(.,'#'),'%23',substring-after(.,'#'))"/>;&lt;<xsl:value-of select='$esc_type_0'/>&gt; ?<xsl:value-of select="substring-after(.,'#')"/></xsl:for-each> .}order by ?x</xsl:variable>
<a href="{concat('/sparql?query=',$query,'&amp;xsl=table.xsl')}">table</a>

</div>
<!--
<ul>
<xsl:for-each select='$instances/s:sparql/s:results/s:result/s:binding/s:uri'>
<li><a href="{concat('/sparql?query=describe &lt;',.,'&gt;&amp;xsl=inventory.xsl')}"><xsl:value-of select='.'/></a></li>
</xsl:for-each>
</ul>
-->
</div>
</xsl:template>
<xsl:template match="s:uri"/>
<xsl:template match="*">
<!-- describe resource -->
<div class='jm'>
<!-- should use css classes instead -->
<xsl:if test='inv:on=1'><xsl:attribute name='style'>background-color:lime;</xsl:attribute></xsl:if>
<xsl:if test='inv:on=0 and inv:uptime&gt;0'><xsl:attribute name='style'>background-color:#ccffcc;</xsl:attribute></xsl:if>
<div class='header'><b><xsl:value-of select="concat(substring-after(*[1]/@rdf:resource,'#'),' ',*[2])"/></b> |
<xsl:choose>
<xsl:when test="obj:next">
	<strike> edit </strike>
</xsl:when>
<xsl:otherwise>
	<a href="{concat('/sparql?query=select * where {&lt;',@rdf:ID,'&gt; ?p ?v .}&amp;xsl=inventory_edit.xsl')}">edit</a>
</xsl:otherwise>
</xsl:choose>
</div>
<table>
<xsl:apply-templates mode='property'/>
<xsl:if test='geo:lat'>
<tr><td>geo:lat</td><td><xsl:value-of select='geo:lat'/></td></tr>
<tr><td>geo:long</td><td><xsl:value-of select='geo:long'/></td></tr>
<tr><td><a href="{concat('http://maps.google.com/maps?q=',geo:lat,',',geo:long,'&amp;iwloc=A&amp;hl=en')}">map</a></td><td/></tr>
</xsl:if>
</table>
<!--<div class='qr_code'>-->
<!-- let's use google chart API https://developers.google.com/chart/infographics/docs/qr_codes -->
<!--
<a href="{concat('http://chart.googleapis.com/chart?cht=qr&amp;chs=150x150&amp;choe=UTF-8&amp;chl=',/rdf:RDF/@xml:base,'%23',*[2])}">
<img src="{concat('http://chart.googleapis.com/chart?cht=qr&amp;chs=150x150&amp;choe=UTF-8&amp;chl=',/rdf:RDF/@xml:base,'%23',*[2])}"/>
</a>
-->
<!--</div>-->
</div>
<xsl:if test='inv:status'><!-- check if it is inv:Equipment -->
<!--
<div class='qr_code'>
<img src="{concat('http://chart.googleapis.com/chart?cht=qr&amp;chs=150x150&amp;choe=UTF-8&amp;chl=',/rdf:RDF/@xml:base,'%23',*[2])}"/>
<div class='label'><xsl:value-of select="concat(/rdf:RDF/@xml:base,'#',*[2])"/></div>
</div>
-->
</xsl:if>
</xsl:template>
<xsl:template match='inv:Report'>
<div class='report'>
<div class='time'><xsl:value-of select='inv:time_stamp_v'/> | <a href="{concat('/sparql?query=select * where {&lt;',@rdf:ID,'&gt; ?p ?v .}&amp;xsl=inventory_edit.xsl')}">edit</a> </div>
<div class='text'><xsl:copy-of select='inv:text'/></div>
</div>
</xsl:template>
<!-- should be more general than that -->
<xsl:template match="inv:Kiosk|inv:DDrum|inv:Uniport">
<!-- describe resource -->
<div class='jm'>
<!-- should use css classes instead -->
<xsl:if test='inv:on=1'><xsl:attribute name='style'>background-color:lime;</xsl:attribute></xsl:if>
<xsl:if test='inv:on=0 and inv:uptime&gt;0'><xsl:attribute name='style'>background-color:#ccffcc;</xsl:attribute></xsl:if>
<div class='header'><b><xsl:value-of select="concat(substring-after(*[1]/@rdf:resource,'#'),' ')"/><a href="{concat('/sparql?query=describe &lt;',*[2],'&gt;&amp;xsl=inventory.xsl')}"><xsl:value-of select='*[2]'/></a></b> | <a href="{concat('/sparql?query=select * where {&lt;',@rdf:ID,'&gt; ?p ?v .}&amp;xsl=inventory_edit.xsl')}">edit</a>
 | <a href="{concat('/sparql?query=prefix :&lt;http://inventory.unicefuganda.org/%23&gt; select * where {?x a :Person;:site &lt;',substring-after(inv:located/@rdf:resource,'#'),'&gt;;:phone ?phone .}&amp;xsl=table.xsl')}">contact</a> 
 <!--
 | <a href="{concat('/sparql?query=prefix :&lt;http://inventory.unicefuganda.org/%23&gt; describe * where {&lt;',substring-after(inv:located/@rdf:resource,'#'),'&gt; :report ?report .}&amp;xsl=inventory.xsl')}">report</a>
 -->
<!-- optimization -->
 | <a href="{concat('/sparql?query=prefix :&lt;http://inventory.unicefuganda.org/%23&gt; describe ?report where {?x obj:id %27',substring-after(inv:located/@rdf:resource,'#'),'%27;:report ?report;a :Site .}&amp;xsl=inventory.xsl')}">report</a>
 | <a href="{concat('/sparql?query=prefix :&lt;http://inventory.unicefuganda.org/%23&gt; select * where {?x a :Equipment;:partOf &lt;',@rdf:ID,'&gt;;a ?type;:model ?model .}&amp;xsl=table.xsl')}">equipment</a> 
</div>
<table>
<xsl:apply-templates mode='property'/>
<xsl:if test='geo:lat'>
<tr><td>geo:lat</td><td><xsl:value-of select='geo:lat'/></td></tr>
<tr><td>geo:long</td><td><xsl:value-of select='geo:long'/></td></tr>
<tr><td><a href="{concat('http://maps.google.com/maps?q=',geo:lat,',',geo:long,'&amp;iwloc=A&amp;hl=en')}">map</a></td><td/></tr>
</xsl:if>
</table>
<!--<div class='qr_code'>-->
<!-- let's use google chart API https://developers.google.com/chart/infographics/docs/qr_codes -->
<!--
<a href="{concat('http://chart.googleapis.com/chart?cht=qr&amp;chs=150x150&amp;choe=UTF-8&amp;chl=',/rdf:RDF/@xml:base,'%23',*[2])}">
<img src="{concat('http://chart.googleapis.com/chart?cht=qr&amp;chs=150x150&amp;choe=UTF-8&amp;chl=',/rdf:RDF/@xml:base,'%23',*[2])}"/>
</a>
-->
<!--</div>-->
</div>
<xsl:if test='inv:status'><!-- check if it is inv:Equipment -->
<!--
<div class='qr_code'>
<img src="{concat('http://chart.googleapis.com/chart?cht=qr&amp;chs=150x150&amp;choe=UTF-8&amp;chl=',/rdf:RDF/@xml:base,'%23',*[2])}"/>
<div class='label'><xsl:value-of select="concat(/rdf:RDF/@xml:base,'#',*[2])"/></div>
</div>
-->
</xsl:if>
</xsl:template>

<xsl:template match='*' mode='property'>
<tr><td><xsl:value-of select='name()'/></td><td><xsl:apply-templates/></td></tr>
</xsl:template>
<xsl:template match='*[svg:svg]' mode='property'>
<tr>
<td><xsl:value-of select='name()'/></td>
<td>
<!-- link to the raw data -->
<a href="{concat('/sparql?query=prefix :&lt;http://inventory.unicefuganda.org/%23&gt; select ?time ?uptime ?volt ?client ?data where{&lt;',../@rdf:ID,'&gt; :logger ?x .?x :time_stamp_v ?time;:uptime_v ?uptime;:volt ?volt;:n_client ?client;:data ?data .}&amp;xsl=table.xsl')}">
<xsl:copy-of select='./svg:svg'/>
</a></td>
</tr>
</xsl:template>
<xsl:template match='inv:logger[@rdf:nodeID]' mode='property'/>

<xsl:template match='*[@rdf:resource]' mode='property'>
<tr><td><xsl:value-of select='name()'/></td><td><a href="{concat('/sparql?query=describe &lt;',substring-before(@rdf:resource,'#'),'%23',substring-after(@rdf:resource,'#'),'&gt;&amp;xsl=inventory.xsl')}"><xsl:value-of select='@rdf:resource'/></a></td></tr>
</xsl:template>
<!-- blank node reference -->
<!--
<xsl:template match='*[@rdf:nodeID]' mode='property'>
<tr><td><xsl:value-of select='name()'/></td><td><a href="{concat('/sparql?query=describe &lt;',substring-before(@rdf:nodeID,'#'),'%23',substring-after(@rdf:nodeID,'#'),'&gt;&amp;xsl=inventory.xsl')}"><xsl:value-of select='@rdf:nodeID'/></a></td></tr>
</xsl:template>
-->
<xsl:template match='rdf:type[@rdf:resource]' mode='property'/>
<xsl:template match='obj:id' mode='property'/>
<xsl:template match='obj:self[@rdf:resource]' mode='property'/>
<xsl:template match='inv:time_stamp' mode='property'/>
<!-- specialize for geo -->
<xsl:template match='geo:lat' mode='property'/>
<xsl:template match='geo:long' mode='property'/>
<!-- title -->
<xsl:template match='rdf:RDF' mode='title'>
<title><xsl:value-of select="concat(substring-after(*/rdf:type/@rdf:resource,'#'),' ',*/@rdf:ID)"/>
<xsl:if test='*[2]'> ...</xsl:if>
</title> 
</xsl:template>
<xsl:template match='*' mode='title'>
<title>UNICEF T4D Inventory</title>
</xsl:template>
</xsl:stylesheet>
