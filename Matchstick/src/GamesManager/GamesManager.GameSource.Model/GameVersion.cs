using System;
using System.Collections.Generic;
using System.ComponentModel.DataAnnotations;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.GameSource.Model
{
    public class GameVersionMetadata
    {
        [StringLength(255)]
        [Required(ErrorMessage = "Version is required")]
        public string Version { get; set; }

        [StringLength(255)]
        public string UpdateMinVersion { get; set; }

        [StringLength(255)]
        public string UpdateMaxVersion { get; set; }

        [StringLength(255)]
        public string Description { get; set; }
    }

    [MetadataType(typeof(GameVersionMetadata))]
    partial class GameVersion
    {
    }
}
