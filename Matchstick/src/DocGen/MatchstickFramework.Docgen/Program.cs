using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Xml;
using System.Xml.Linq;
using Newtonsoft.Json;
using Newtonsoft.Json.Converters;
using MatchstickFramework.Docgen.Model;

namespace MatchstickFramework.Docgen
{
	static class StringExtensions
	{
		public static string Capitalize(this string input)
		{
			if (String.IsNullOrEmpty(input))
				throw new ArgumentException("ARGH!");
			return input.First().ToString().ToUpper() + String.Join("", input.Skip(1));
		}
	}

	class Program
	{
		static void Main(string[] args)
		{
			if (args.Length != 2)
			{
				Console.WriteLine("Expected usage: Docgen.exe <input folder> <output file>");
				return;
			}

			string xmlDocsPath = args[0];

			XDocument doc = XDocument.Load(Path.Combine(xmlDocsPath, "namespace_m_g_d_f.xml"));

			var classElements = doc.Descendants("innerclass").Select(c => new {
				Name = c.Value,
				Ref = c.Attribute("refid").Value
			});
			
			var api = new ApiDoc{
				Classes = new List<ClassDoc>(),
				Structs = new List<ClassDoc>(),
				Functions = new List<MemberDoc>(),
				Enums = new List<EnumDoc>(),
				Mappings = new Dictionary<string, string>()
			};

			// parse out all class information
			foreach (var c in classElements)
			{
				XDocument classDoc = XDocument.Load(Path.Combine(xmlDocsPath, c.Ref + ".xml"));
                var newClass = new ClassDoc
                {
                    Id = c.Ref,
                    Name = c.Name.Replace("MGDF::", string.Empty),
                    Description = classDoc.Descendants("compounddef").First().Element("briefdescription").Value.Capitalize(),
                    Members = new List<MemberDoc>(),
                    InheritsFromTypeRefIds = new List<string>(),
                    InheritsFromUnreferencedType = new List<string>()
				};
                Console.WriteLine($@"Processing class {newClass.Name}");

				if (classDoc.Descendants("compounddef").First().Element("detaileddescription") != null)
				{
					newClass.Description += classDoc.Descendants("compounddef").First().Element("detaileddescription").Value;
				}

                Console.WriteLine($@"  computing inheritance tree...");
				// figure out what classes this class inherits from
				var inherits = classDoc.Descendants("compounddef").First().Element("inheritancegraph");
				if ( inherits !=null) {
					var self = inherits.Descendants("node").Single(n=>
                    {
                        var link = n.Element("link");
                        return link != null && link.Attribute("refid").Value == newClass.Id;
                    });
					if (self.Elements("childnode").Count() != 0)
					{
                        var parent = inherits.Descendants("node").SingleOrDefault(n => n.Attribute("id").Value == self.Elements("childnode").First().Attribute("refid").Value);
						while (true)
						{
                            if (parent.Element("link") != null)
                            {
                                newClass.InheritsFromTypeRefIds.Add(parent.Element("link").Attribute("refid").Value);
                            }
                            else
                            {
                                newClass.InheritsFromUnreferencedType.Add(parent.Element("label").Value);
                            }
							if (parent.Elements("childnode").Count() == 0) break;
                            parent = inherits.Descendants("node").SingleOrDefault(n => n.Attribute("id").Value == parent.Elements("childnode").First().Attribute("refid").Value);
						}
					}
				}

				api.Mappings.Add(newClass.Id, newClass.Name);

                Console.WriteLine($@"  parsing members...");
				foreach (var m in classDoc.Descendants("memberdef")) {
					var newMember = ParseMemberDoc(m);
					newClass.Members.Add(newMember);
				}
				if (classDoc.Descendants("compounddef").First().Attribute("kind").Value == "class")
				{
					api.Classes.Add(newClass);
				}
				else
				{
					api.Structs.Add(newClass);
				}

			}

			// parse out all free function information
			var functionElements = doc.Descendants("memberdef").Where(m=>m.Attribute("kind").Value == "function");
			foreach (var f in functionElements)
			{
				var m = ParseMemberDoc(f);
				api.Functions.Add(m);
                Console.WriteLine($@"Processing function {m.Name}");
			}

			// parse out all enum information
			var enumElements = doc.Descendants("memberdef").Where(m => m.Attribute("kind").Value == "enum");
			foreach (var e in enumElements)
			{
				var newEnum = new EnumDoc
				{
					Id = e.Attribute("id").Value,
					Name = e.Element("name").Value,
					Description = e.Element("briefdescription").Value.Capitalize(),
					Values = new List<string>()
				};
                Console.WriteLine($@"Processing enum {newEnum.Name}");
				api.Mappings.Add(newEnum.Id, newEnum.Name);

				foreach (var value in e.Descendants("enumvalue"))
				{
					newEnum.Values.Add(value.Element("name").Value);
				}
				api.Enums.Add(newEnum);
			}

			api.Classes.Sort((a, b) => a.Name.CompareTo(b.Name));
			api.Functions.Sort((a, b) => a.Name.CompareTo(b.Name));
			api.Enums.Sort((a, b) => a.Name.CompareTo(b.Name));

			string output = JsonConvert.SerializeObject(api);
			File.WriteAllText(args[1], output);
		}

		private static MemberDoc ParseMemberDoc(XElement m)
		{
			var newMember = new MemberDoc
			{
				Id = m.Attribute("id").Value,
				Name = m.Element("name").Value,
				ReturnType = m.Element("type").Value,
				Description = m.Element("briefdescription").Value,
				MemberType = m.Attribute("kind").Value == "function" ? MemberType.Function : MemberType.Field,
				Const = m.Attribute("const")!=null && m.Attribute("const").Value == "yes",
				Args = new List<FunctionArg>()
			};
			if (!string.IsNullOrEmpty(newMember.Description))
			{
				newMember.Description = newMember.Description.Capitalize();
			}
			if (m.Element("type").Element("ref") != null)
			{
				newMember.ReturnTypeRefId = m.Element("type").Element("ref").Attribute("refid").Value;
			}
			var returnDesc = m.Descendants("simplesect").Where(s=>s.Attribute("kind").Value=="return").SingleOrDefault();
			if (returnDesc != null)
			{
				newMember.ReturnDescription = returnDesc.Value.Capitalize();
			}

			if (newMember.MemberType == MemberType.Function)
			{
				foreach (var a in m.Elements("param"))
				{
					if (a.Element("type").Value == "void") continue;
					var newArg = new FunctionArg()
					{
						Name = a.Element("declname").Value,
						Type = a.Element("type").Value,
						Description = string.Empty
					};
					if (a.Element("type").Element("ref") != null)
					{
						newArg.TypeRefId =a.Element("type").Element("ref").Attribute("refid").Value;
					}

					var parameterItem = m.Descendants("parameteritem").SingleOrDefault(p => p.Descendants("parametername").First().Value == newArg.Name);
					if (parameterItem != null)
					{
						newArg.Description = parameterItem
							.Element("parameterdescription")
							.Element("para")
							.Value.Capitalize();
					}
					newMember.Args.Add(newArg);
				}
			}
			return newMember;
		}
	}
}
