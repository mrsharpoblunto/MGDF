<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage<LatestVersionViewData>" %>
<%@ Import Namespace="MGDF.GamesManager.Common.Extensions"%>
<%@ Import Namespace="MatchstickFramework.Web.Views.LatestVersion"%>
<%
    Response.ContentType = "text/xml";   
    if (ViewData.Model.Version !=null){%><?xml version="1.0" encoding="utf-8" ?>
<latestversion>
    <version><%=ViewData.Model.Version.Version %></version>
    <url><%=ViewData.Model.Version.FileName.AbsoluteUri(Request.Url.Scheme+"://" +Request.Url.Host)%></url>
    <md5><%=ViewData.Model.Version.MD5 %></md5>
</latestversion>
<%} else { %><?xml version="1.0" encoding="utf-8" ?>
    <error>No versions available.</error>
<%} %>


