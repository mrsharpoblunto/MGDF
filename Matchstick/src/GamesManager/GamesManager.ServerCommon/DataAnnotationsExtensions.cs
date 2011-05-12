using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.ServerCommon
{
    public class ErrorInfo
    {
        public string Name { get; set; }
        public string ErrorMessage { get; set; }
        public object Instance { get; set; }
    }

    public static class DataAnnotationsExtensions
    {
        public static IEnumerable<ErrorInfo> GetValidationErrors(this object instance)
        {
            var metadataAttrib = instance.GetType().GetCustomAttributes(typeof(MetadataTypeAttribute), true).OfType<MetadataTypeAttribute>().FirstOrDefault();
            var buddyClassOrModelClass = metadataAttrib != null ? metadataAttrib.MetadataClassType : instance.GetType();
            var buddyClassProperties = TypeDescriptor.GetProperties(buddyClassOrModelClass).Cast<PropertyDescriptor>();
            var modelClassProperties = TypeDescriptor.GetProperties(instance.GetType()).Cast<PropertyDescriptor>();

            return from buddyProp in buddyClassProperties
                   join modelProp in modelClassProperties on buddyProp.Name equals modelProp.Name
                   from attribute in buddyProp.Attributes.OfType<ValidationAttribute>()
                   where !attribute.IsValid(modelProp.GetValue(instance))
                   select new ErrorInfo { Name = buddyProp.Name, ErrorMessage = attribute.FormatErrorMessage(string.Empty), Instance = instance };
        }
    }
}