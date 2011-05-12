using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.GameSource.Model
{
    public class GameMetadata
    {
        [StringLength(255)]
        [Required(ErrorMessage = "Name is required")]
        public string Name { get; set; }

        [StringLength(255)]
        [Required(ErrorMessage = "Uid is required")]
        [RegularExpression("^([A-Za-z0-9]|\\-)*$", ErrorMessage = "Uid's can only contain alphanumeric characters and the '-' symbol")]
        public string Uid { get; set; }

        [StringLength(255)]
        [Required(ErrorMessage = "Homepage is required")]
        public string Homepage { get; set; }

        [StringLength(255)]
        [Required(ErrorMessage = "Description is required")]
        public string Description { get; set; }
    }

    [MetadataType(typeof(GameMetadata))]
    partial class Game
    {
    }
}
