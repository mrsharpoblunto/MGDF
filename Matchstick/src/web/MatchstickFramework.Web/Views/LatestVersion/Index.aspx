<%@ Page Language="C#" Inherits="System.Web.Mvc.ViewPage<LatestVersionViewData>" %>
<%@ Import Namespace="Newtonsoft.Json"%>
<%@ Import Namespace="MGDF.GamesManager.Common.Extensions"%>
<%@ Import Namespace="MatchstickFramework.Web.Views.LatestVersion"%>
<%
    Response.ContentType = "application/json";   
    if (ViewData.Model.Version !=null){
        Response.StatusCode = 200;
        Response.Write(
            JsonConvert.SerializeObject(
                new
                    {
                        Version = ViewData.Model.Version.Version.ToString(),
                        Url = ViewData.Model.Version.FileName.AbsoluteUri(Request.Url.Scheme + "://" + Request.Url.Host),
                        MD5 = ViewData.Model.Version.MD5
                    }));
    } 
    else
    {
        Response.StatusCode = 404;
    }
    
%>


