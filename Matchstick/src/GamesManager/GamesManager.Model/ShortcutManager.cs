using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;
using IWshRuntimeLibrary;
using MGDF.GamesManager.Common;
using MGDF.GamesManager.Common.Framework;
using File = System.IO.File;

namespace MGDF.GamesManager.Model
{
  public interface IShortcutManager
  {
    void CreateShortcut(string path, string filename, string args, string icon);
    void DeleteShortcut(string path);
    bool HasShortcut(string path);

  }

  public class ShortcutManager : IShortcutManager
  {
    public static IShortcutManager Current
    {
      get
      {
        return ServiceLocator.Current.Get<IShortcutManager>();
      }
      set
      {
        ServiceLocator.Current.Register(value);
      }
    }

    [DllImport("shell32.dll")]
    private static extern void SHChangeNotify(EventId wEventId,
                                              uFlags uFlags,
                                              IntPtr dwItem1,
                                              IntPtr dwItem2);

    // Nested Types
    [Flags]
    public enum uFlags
    {
      SHCNF_DWORD
    }

    [Flags]
    private enum EventId
    {
      SHCNE_ASSOCCHANGED = 0x8000000
    }

    public static void RefreshDesktop()
    {
      try
      {
        SHChangeNotify(EventId.SHCNE_ASSOCCHANGED, uFlags.SHCNF_DWORD, IntPtr.Zero, IntPtr.Zero);
      }
      catch (Exception)
      {
      }
    }

    public void CreateShortcut(string path, string filename, string args, string icon)
    {
      try
      {
        WshShellClass class2 = new WshShellClass();
        IWshShortcut shortcut = (IWshShortcut)class2.CreateShortcut(path);
        shortcut.TargetPath = filename;
        shortcut.IconLocation = icon;
        shortcut.Arguments = args;
        shortcut.Save();
      }
      catch (Exception ex)
      {
        Logger.Current.Write(ex, "Unable to create shortcut");
      }
    }

    public void DeleteShortcut(string path)
    {
      if (!string.IsNullOrEmpty(path))
      {
        if (File.Exists(path))
        {
          File.Delete(path);
        }
      }
    }

    bool IShortcutManager.HasShortcut(string path)
    {
      return !string.IsNullOrEmpty(path) && File.Exists(path);
    }
  }
}