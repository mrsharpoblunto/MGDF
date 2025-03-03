using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MatchstickFramework.Docgen.Model
{
  public class ApiDoc
  {
    public List<ClassDoc> Classes;
    public List<ClassDoc> Structs;
    public List<EnumDoc> Enums;
    public Dictionary<string, string> Mappings;
  }

  public class FunctionArg
  {
    public string Name;
    public string Type;
    public string TypeRefId;
    public string Description;
  }

  public enum MemberType
  {
    Field,
    Function
  }

  public class MemberDoc
  {
    public MemberType MemberType;
    public string Id;
    public string Name;
    public string Description;
    public string ReturnType;
    public string ReturnDescription;
    public string ReturnTypeRefId;
    public List<FunctionArg> Args;
    public bool Const;
  }

  public class ClassDoc
  {
    public List<string> InheritsFromTypeRefIds;
    public List<string> InheritsFromUnreferencedType;
    public string Id;
    public string Name;
    public string Description;
    public List<MemberDoc> Members;
  }

  public class EnumDoc
  {
    public string Id;
    public string Description;
    public string Name;
    public List<string> Values;
  }

  public class CommentBlock {
    public string Description;
    public string ReturnDescription;
    public Dictionary<string, string> ParamDescriptions;
  }
}
