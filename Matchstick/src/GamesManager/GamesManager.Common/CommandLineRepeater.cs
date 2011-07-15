using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Common
{
    public class CommandLineRepeater
    {
        private readonly List<string> _arguments = new List<string>();

        public CommandLineRepeater(string[] args)
        {
            _arguments = new List<string>(args);
            _arguments.RemoveAt(0);//get rid of the original program name
        }

        public override string ToString()
        {
            StringBuilder sb = new StringBuilder();
            foreach (var arg in _arguments)
            {
                if (arg.Contains(" "))
                {
                    sb.AppendFormat(" \"{0}\"", arg);
                } 
                else 
                {
                    sb.Append(" " + arg); 
                }
            }
            return sb.ToString();
        }
    }
}
