using System;
using System.Collections.Generic;
using System.Linq;
using System.Web;
using System.Web.Mvc;
using System.Web.Mvc.Ajax;
using MatchstickFramework.Web.Models;
using MatchstickFramework.Web.Views.LatestVersion;

namespace MatchstickFramework.Web.Controllers
{
    public class LatestVersionController : Controller
    {
        public ActionResult Index(int id)
        {
            LatestVersionViewData viewData = new LatestVersionViewData { Version = VersionManager.GetLatestVersion(id) };
            return View(viewData);
        }

        public ActionResult SDK(int id)
        {
            LatestVersionViewData viewData = new LatestVersionViewData { Version = VersionManager.GetLatestSDKVersion(id) };
            return View("Index",viewData);
        }

    }
}
