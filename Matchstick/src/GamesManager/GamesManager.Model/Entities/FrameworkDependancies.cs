using System;
using System.Collections.Generic;
using System.Collections.Specialized;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Newtonsoft.Json.Linq;

namespace MGDF.GamesManager.Model.Entities
{
	public class FrameworkDependancies : JsonEntity
	{
		Dictionary<string, string> _dependancies = new Dictionary<string, string>();

		//when installed and loaded from the filesystem
        public FrameworkDependancies(string depsFile)
			: base(depsFile)
        {
        }

        //when loaded from an install package
		public FrameworkDependancies(IArchiveFile depsFile)
			: base(depsFile)
        {
        }

		#region FrameworkDependencies Members

		public Dictionary<string, string> Dependencies { get { return _dependancies; } }

        #endregion

        protected override void Load(JObject json)
        {
			var dependencies = json["dependancies"]; 
			if (dependencies == null) throw new Exception("Required attribute 'dependancies' missing");
			if (dependencies.Type != JTokenType.Array) throw new Exception("Required attribute 'dependancies' is not an array");

			foreach (var dep in dependencies)
			{
				_dependancies.Add(
					dep.ReadRequiredValue("command"),
					dep.ReadRequiredValue("arguments")
				);
			}
        }
	}
}
