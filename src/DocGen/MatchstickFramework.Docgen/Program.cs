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

      string input = System.IO.File.ReadAllText(args[0]);

      var api = ParseInput(input);
      api.Classes.Sort((a, b) => a.Name.CompareTo(b.Name));
      api.Structs.Sort((a, b) => a.Name.CompareTo(b.Name));
      api.Enums.Sort((a, b) => a.Name.CompareTo(b.Name));

      string output = JsonConvert.SerializeObject(api);
      File.WriteAllText(args[1], output);

    }

    static ApiDoc ParseInput(string input)
    {
      List<string> errors = new List<string>();

      ApiDoc api = new ApiDoc
      {
        Classes = new List<ClassDoc>(),
        Enums = new List<EnumDoc>(),
        Structs = new List<ClassDoc>(),
        Mappings = new Dictionary<string, string>(),
      };

      CommentBlock lastComment = null;
      using (var stream = new MemoryStream(Encoding.UTF8.GetBytes(input)))
      {
        using (var reader = new StreamReader(stream))
        {
          while (!reader.EndOfStream)
          {
            var ch = (char)reader.Peek();
            switch (ch)
            {
              case '/':
                reader.Read();
                if (reader.Peek() == '*')
                {
                  reader.Read();
                  if (reader.Peek() == '*')
                  {
                    reader.Read();
                  }
                  lastComment = ParseCommentBlock(reader);
                }
                break;
              case 'i':
                {
                  var word = CaptureWord(reader);
                  if (word == "interface")
                  {
                    var c = ParseInterface(reader, api.Mappings);
                    if (c != null)
                    {
                      if (lastComment != null)
                      {
                        c.Description = lastComment.Description;
                      }
                      if (string.IsNullOrEmpty(c.Description))
                      {
                        errors.Add($"no description found for interface {c.Name}");
                      }
                      foreach (var member in c.Members)
                      {
                        if (string.IsNullOrEmpty(member.Description))
                        {
                          errors.Add($"no description found for interface member {c.Name}.{member.Name}");
                        }
                        if (string.IsNullOrEmpty(member.ReturnDescription) && member.ReturnType != "void")
                        {
                          errors.Add($"No \\return description found for interface member {c.Name}.{member.Name}");
                        }
                        member.Args.ForEach((arg) =>
                        {
                          if (string.IsNullOrEmpty(arg.Description))
                          {
                            errors.Add($"No \\param description found for {arg.Name} in interface member {c.Name}.{member.Name}");
                          }
                        });
                      }
                      api.Classes.Add(c);
                    }
                    lastComment = null;
                  }
                }
                break;
              case 't':
                {
                  var word = CaptureWord(reader);
                  if (word == "typedef")
                  {
                    var type = CaptureWord(reader);
                    if (type == "enum")
                    {
                      var e = ParseEnum(reader, api.Mappings);
                      if (e != null)
                      {
                        if (lastComment != null)
                        {
                          e.Description = lastComment.Description;
                        }
                        if (string.IsNullOrEmpty(e.Description))
                        {
                          errors.Add($"no description found for enum {e.Name}");
                        }
                        api.Enums.Add(e);
                        lastComment = null;
                      }
                    }
                    else if (type == "struct")
                    {
                      var s = ParseStruct(reader, api.Mappings);
                      if (s != null)
                      {
                        if (lastComment != null)
                        {
                          s.Description = lastComment.Description;
                        }
                        if (string.IsNullOrEmpty(s.Description))
                        {
                          errors.Add($"no description found for struct {s.Name}");
                        }
                        api.Structs.Add(s);
                        lastComment = null;
                      }
                    }

                  }
                }
                break;
              default:
                reader.Read();
                break;


            }
          }
        }
      }

      if (errors.Count > 0)
      {
        Console.WriteLine("Errors found generating docs:");
        foreach (var error in errors)
        {
          Console.WriteLine($" - {error}");
        }
        Environment.Exit(1);
      }
      return api;
    }

    static EnumDoc ParseEnum(StreamReader reader, Dictionary<string, string> mappings)
    {
      EnumDoc e = new EnumDoc
      {
        Values = new List<string>(),
      };

      e.Id = e.Name = CaptureWord(reader);
      mappings.Add(e.Name, e.Name);

      if (CapturePunctuation(reader) != "{")
      {
        throw new Exception($"Expected {{ after identifier in enum ${e.Name}");
      }

      char ch;
      do
      {
        // skip over comment blocks for individual values
        if (reader.Peek() == '*')
        {
          reader.Read();
          if (reader.Peek() == '*')
          {
            reader.Read();
          }
          ParseCommentBlock(reader);
        }

        e.Values.Add(CaptureWord(reader));

        // skip past the enum value
        do
        {
          ch = (char)reader.Read();
        }
        while (ch != ',' && ch != '}');
      } while (ch != '}' && !reader.EndOfStream);

      do
      {
        ch = (char)reader.Read();
      } while (ch != ';');

      return e;
    }
    static ClassDoc ParseStruct(StreamReader reader, Dictionary<string, string> mappings)
    {
      ClassDoc s = new ClassDoc
      {
        InheritsFromTypeRefIds = new List<string>(),
        InheritsFromUnreferencedType = new List<string>(),
        Members = new List<MemberDoc>(),
      };
      CommentBlock lastComment = null;
      MemberDoc prevMember = null;

      s.Id = s.Name = CaptureWord(reader);
      mappings.Add(s.Name, s.Name);

      if (CapturePunctuation(reader) != "{")
      {
        throw new Exception($"Expected {{ after identifier in struct ${s.Name}");
      }
      SkipWhitespace(reader);

      char ch;
      while (reader.Peek() != '}')
      {
        // skip over comment blocks for individual values
        if (reader.Peek() == '*')
        {
          reader.Read();
          if (reader.Peek() == '*')
          {
            reader.Read();
          }
          lastComment = ParseCommentBlock(reader);
        }

        if (prevMember == null)
        {
          // struct member with its own type declaration
          prevMember = new MemberDoc
          {
            Args = new List<FunctionArg>(),
            Const = false,
            MemberType = MemberType.Field,
          };

          var modifierOrType = CaptureWord(reader);
          if (modifierOrType == "const")
          {
            var type = CaptureWord(reader);
            if (mappings.ContainsKey(type))
            {
              prevMember.ReturnTypeRefId = type;
            }
            modifierOrType += " " + type;
          }
          else if (mappings.ContainsKey(modifierOrType))
          {
            prevMember.ReturnTypeRefId = modifierOrType;
          }
          // reference or pointer types
          var pointer = CapturePunctuation(reader);
          if (pointer != "")
          {
            modifierOrType += " " + pointer;
          }
          prevMember.ReturnType = modifierOrType;
        }
        else
        {
          // an additional member re-using a previously declared type
          prevMember = new MemberDoc
          {
            Args = new List<FunctionArg>(),
            Const = false,
            MemberType = MemberType.Field,
            ReturnTypeRefId = prevMember.ReturnTypeRefId,
            ReturnType = prevMember.ReturnType,
          };
        }

        prevMember.Id = prevMember.Name = CaptureWord(reader);
        if (lastComment != null)
        {
          prevMember.Description = lastComment.Description;
        }
        s.Members.Add(prevMember);
        lastComment = null;

        // skip to the next member
        do
        {
          ch = (char)reader.Read();
        }
        while (ch != ',' && ch != ';');
        if (ch == ';')
        {
          lastComment = null;
          prevMember = null;
        }

        SkipWhitespace(reader);
      }
      do
      {
        ch = (char)reader.Read();
      } while (ch != ';');
      return s;
    }

    static ClassDoc ParseInterface(StreamReader reader, Dictionary<string, string> mappings)
    {
      ClassDoc c = new ClassDoc
      {
        InheritsFromTypeRefIds = new List<string>(),
        InheritsFromUnreferencedType = new List<string>(),
        Members = new List<MemberDoc>(),
      };

      c.Id = c.Name = CaptureWord(reader);
      // may have already been forward declared, in which case ignore
      if (!mappings.ContainsKey(c.Name))
      {
        mappings.Add(c.Name, c.Name);
      }

      var separator = CapturePunctuation(reader);
      // forward declarations - ignore
      if (separator == ";")
      {
        return null;
      }
      // inheritance
      else if (separator == ":")
      {
        var super = CaptureWord(reader);
        (mappings.ContainsKey(super) ? c.InheritsFromTypeRefIds : c.InheritsFromUnreferencedType).Add(super);
        separator = CapturePunctuation(reader);
        while (separator == ",")
        {
          super = CaptureWord(reader);
          (mappings.ContainsKey(super) ? c.InheritsFromTypeRefIds : c.InheritsFromUnreferencedType).Add(super);
          separator = CapturePunctuation(reader);
        }
      }

      if (separator == "{};")
      {
        // empty class declaration
      }
      // not the opening brace we expected
      else if (separator != "{")
      {
        throw new Exception($"Expected after {{ in interface declaration {c.Id}");
      }
      else
      {
        ParseInterfaceMembers(reader, c, mappings);
      }

      return c;
    }

    static void ParseInterfaceMembers(StreamReader reader, ClassDoc c, Dictionary<string, string> mappings)
    {
      CommentBlock lastComment = null;
      while (!reader.EndOfStream)
      {
        SkipWhitespace(reader);
        var ch = (char)reader.Peek();
        switch (ch)
        {
          case '/':
            reader.Read();
            if (reader.Peek() == '*')
            {
              reader.Read();
              if (reader.Peek() == '*')
              {
                reader.Read();
              }
              lastComment = ParseCommentBlock(reader);
            }
            break;
          case '}':
            reader.Read();
            if (reader.Peek() != ';')
            {
              throw new Exception($"Expected ; after closing }} in  {c.Name}");
            }
            return;
          default:
            {
              MemberDoc member = new MemberDoc
              {
                Args = new List<FunctionArg>(),
                Const = false,
                MemberType = MemberType.Function,
              };

              var modifierOrType = CaptureWord(reader);
              if (modifierOrType == "const")
              {
                var type = CaptureWord(reader);
                if (mappings.ContainsKey(type))
                {
                  member.ReturnTypeRefId = type;
                }
                modifierOrType += " " + type;
              }
              else if (mappings.ContainsKey(modifierOrType))
              {
                member.ReturnTypeRefId = modifierOrType;
              }
              // reference or pointer types
              var pointer = CapturePunctuation(reader);
              if (pointer != "")
              {
                modifierOrType += " " + pointer;
              }
              member.ReturnType = modifierOrType;

              member.Id = member.Name = CaptureWord(reader);

              SkipWhitespace(reader);
              if (reader.Peek() != '(')
              {
                throw new Exception($"Expected ( after identifier in interface member {c.Name}.{member.Name}");
              }

              ParseArguments(reader, c, member, mappings);
              if (lastComment != null)
              {
                member.Description = lastComment.Description;
                member.ReturnDescription = lastComment.ReturnDescription;
                member.Args.ForEach((arg) =>
                {
                  if (lastComment.ParamDescriptions.ContainsKey(arg.Name))
                  {
                    arg.Description = lastComment.ParamDescriptions[arg.Name];
                  }
                });
              }
              c.Members.Add(member);
              lastComment = null;
            }
            break;


        }
      }
    }

    static void ParseArguments(StreamReader reader, ClassDoc c, MemberDoc member, Dictionary<string, string> mappings)
    {
      while (!reader.EndOfStream)
      {
        SkipWhitespace(reader);
        var ch = (char)reader.Peek();
        switch (ch)
        {
          case '(':
            reader.Read();
            break;

          case ')':
            {
              while (reader.Peek() != ';')
              {
                reader.Read();
              }
              reader.Read();
              SkipWhitespace(reader);
              return;
            }

          case ',':
            {
              reader.Read();
              SkipWhitespace(reader);
            }
            break;

          case '[':
            {
              while (reader.Peek() != ']')
              {
                reader.Read();
              }
              reader.Read();
              SkipWhitespace(reader);
            }
            break;

          default:
            {
              FunctionArg arg = new FunctionArg();

              List<string> tokens = new List<string>();
              while (!reader.EndOfStream)
              {
                var word = CaptureWord(reader);
                if (!string.IsNullOrEmpty(word))
                {
                  tokens.Add(word);
                }

                ch = (char)reader.Peek();
                if (ch == ',' || ch == ')')
                {
                  break;
                }

                var punct = CapturePunctuation(reader);
                if (!string.IsNullOrEmpty(punct))
                {
                  tokens.Add(punct);
                }
              }

              if (tokens.Count == 0)
              {
                throw new Exception($"Invalid declaration of argument in interface member {c.Name}.{member.Name}");
              }

              // Last token is the argument name
              arg.Name = tokens[tokens.Count - 1];

              // All preceding tokens form the type
              var typeTokens = tokens.Take(tokens.Count - 1).ToList();
              if (typeTokens.Count == 0)
              {
                throw new Exception($"No type found for argument {arg.Name} in interface member {c.Name}.{member.Name}");
              }

              arg.Type = string.Join(" ", typeTokens);

              // Check if type is in mappings (excluding const prefix)
              var typeForMapping = typeTokens.FirstOrDefault();
              if (typeForMapping == "const" && typeTokens.Count > 1)
              {
                typeForMapping = typeTokens[1];
              }
              if (!string.IsNullOrEmpty(typeForMapping) && mappings.ContainsKey(typeForMapping))
              {
                arg.TypeRefId = typeForMapping;
              }

              member.Args.Add(arg);
            }
            break;
        }
      }

    }

    static string CaptureWord(StreamReader reader)
    {
      StringBuilder buffer = new StringBuilder();

      SkipWhitespace(reader);
      while (!reader.EndOfStream)
      {
        var ch = (char)reader.Peek();
        if (!char.IsLetterOrDigit(ch) && ch != '_')
        {
          return buffer.ToString();
        }
        else
        {
          buffer.Append(ch);
          reader.Read();
        }
      }
      return buffer.ToString();
    }
    static string CapturePunctuation(StreamReader reader)
    {
      StringBuilder buffer = new StringBuilder();

      SkipWhitespace(reader);
      while (!reader.EndOfStream)
      {
        var ch = (char)reader.Peek();
        if (!char.IsPunctuation(ch) || ch == '_')
        {
          return buffer.ToString();
        }
        else
        {
          buffer.Append(ch);
          reader.Read();
        }
      }
      return buffer.ToString();
    }

    static void SkipWhitespace(StreamReader reader)
    {
      while (!reader.EndOfStream)
      {
        var ch = (char)reader.Peek();
        if (!char.IsWhiteSpace(ch))
        {
          break;
        }
        else
        {
          reader.Read();
        }
      }
    }

    delegate void SetComment(StringBuilder sb);

    static string CaptureTidyDescription(StringBuilder sb)
    {
      var value = sb.ToString().Trim().Replace("\r\n", " ").Capitalize();
      sb.Clear();
      return value;
    }

    static CommentBlock ParseCommentBlock(StreamReader reader)
    {
      var comment = new CommentBlock
      {
        ParamDescriptions = new Dictionary<string, string>()
      };
      SetComment setter = (StringBuilder b) => { comment.Description = CaptureTidyDescription(b); };

      StringBuilder buffer = new StringBuilder();
      while (!reader.EndOfStream)
      {
        var ch = (char)reader.Read();
        switch (ch)
        {
          case '*':
            {
              if (reader.Peek() == '/')
              {
                reader.Read();
                setter(buffer);
                return comment;
              }
            }
            break;
          case '\\':
            var word = CaptureWord(reader);
            if (word == "param" || word == "return")
            {
              setter(buffer);
              if (word == "param")
              {
                var paramName = CaptureWord(reader);
                setter = (StringBuilder b) => { comment.ParamDescriptions.Add(paramName, CaptureTidyDescription(b)); };
              }
              else
              {
                setter = (StringBuilder b) => { comment.ReturnDescription = CaptureTidyDescription(b); };
              }
            }
            else
            {
              buffer.Append(ch);
              buffer.Append(word);
            }
            break;
          default:
            buffer.Append(ch);
            break;
        }
      }
      setter(buffer);
      return comment;
    }
  }
}
