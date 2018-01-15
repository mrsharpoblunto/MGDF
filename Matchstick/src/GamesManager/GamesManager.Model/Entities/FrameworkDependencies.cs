using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json.Linq;

namespace MGDF.GamesManager.Model.Entities
{
	public class FrameworkDependencies : JsonEntity
	{
		Dictionary<string, string> _dependencies = new Dictionary<string, string>();

		//when installed and loaded from the filesystem
        public FrameworkDependencies(string depsFile)
			: base(depsFile)
        {
        }

        //when loaded from an install package
		public FrameworkDependencies(IArchiveFile depsFile)
			: base(depsFile)
        {
        }

		#region FrameworkDependencies Members

		public Dictionary<string, string> Dependencies { get { return _dependencies; } }

        #endregion

        protected override void Load(JObject json)
        {
			var dependencies = json["dependencies"]; 
			if (dependencies == null) throw new Exception("Required attribute 'dependencies' missing");
			if (dependencies.Type != JTokenType.Array) throw new Exception("Required attribute 'dependencies' is not an array");

			foreach (var dep in dependencies)
			{
				_dependencies.Add(
					dep.ReadRequiredValue("command"),
					dep.ReadRequiredValue("arguments")
				);
			}
        }
	}
}
