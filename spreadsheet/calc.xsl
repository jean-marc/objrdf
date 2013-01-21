<?xml version="1.0" encoding="utf-8"?>
<!--
	http://mashupguide.net/1.0/html/ch17s03.xhtml
	process sparql results and create a simple spreadsheet without style
	the actual archive is created this way:
	zip -r test content.xml settings.xml styles.xml META-INF/
	where META-INF contains manifest.xml:

	<?xml version="1.0" encoding="utf-8"?>
	<manifest:manifest xmlns:manifest="urn:oasis:names:tc:opendocument:xmlns:manifest:1.0">
		<manifest:file-entry manifest:media-type="application/vnd.oasis.opendocument.spreadsheet" manifest:full-path="/" />
		<manifest:file-entry manifest:media-type="text/xml" manifest:full-path="content.xml" />
		<manifest:file-entry manifest:media-type="text/xml" manifest:full-path="settings.xml" />
		<manifest:file-entry manifest:media-type="text/xml" manifest:full-path="styles.xml" />
	</manifest:manifest>
	the 2 last files are not used but need to be present

	there is an on-line validator for OO documents: http://opendocumentfellowship.com/validator
-->
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" xmlns:sparql="http://www.w3.org/2005/sparql-results#" xmlns:style="urn:oasis:names:tc:opendocument:xmlns:style:1.0" xmlns:office="urn:oasis:names:tc:opendocument:xmlns:office:1.0" xmlns:table="urn:oasis:names:tc:opendocument:xmlns:table:1.0" xmlns:text="urn:oasis:names:tc:opendocument:xmlns:text:1.0" xmlns:form="urn:oasis:names:tc:opendocument:xmlns:form:1.0" version="1.0">
  <xsl:template match="sparql:sparql">
    <office:document-content>
      <office:automatic-styles>
        <style:style style:name="ta1" style:family="table" style:master-page-name="PageStyle_5f_Sheet1">
          <style:table-properties table:display="true" style:writing-mode="lr-tb"/>
        </style:style>
      </office:automatic-styles>
      <office:body>
        <office:spreadsheet>
        <!-- seems that Google Doc requires a table style -->
	  <table:table table:name="Sheet1" table:style-name="ta1" table:print="false">
          <!--<table:table table:name="Sheet1" table:print="false">-->
            <!-- do we need all that ? -->
            <office:forms form:automatic-focus="false" form:apply-design-mode="false"/>
            <table:table-column table:number-columns-repeated="20"/>
            <table:table-column table:number-columns-repeated="236"/>
            <xsl:apply-templates/>
          </table:table>
        </office:spreadsheet>
      </office:body>
    </office:document-content>
  </xsl:template>
  <xsl:template match="sparql:head">
    <table:table-row>
      <xsl:for-each select="sparql:variable">
        <table:table-cell office:value-type="string">
          <text:p>
            <xsl:value-of select="@name"/>
          </text:p>
        </table:table-cell>
      </xsl:for-each>
    </table:table-row>
  </xsl:template>
  <xsl:template match="sparql:result">
    <table:table-row>
      <xsl:for-each select="sparql:binding">
        <table:table-cell office:value-type="string">
          <text:p>
            <xsl:value-of select="*"/>
          </text:p>
        </table:table-cell>
      </xsl:for-each>
    </table:table-row>
  </xsl:template>
</xsl:stylesheet>
