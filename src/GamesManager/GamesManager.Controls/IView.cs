using System;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Windows;

namespace MGDF.GamesManager.Controls
{
  public delegate void InvokeDelegate();

  [StructLayout(LayoutKind.Sequential)]
  public struct COPYDATASTRUCT
  {
    public IntPtr dwData;
    public int cbData;
    public IntPtr lpData;
  }

  public interface IView
  {
    event EventHandler Shown;
    event CancelEventHandler Closing;
    event EventHandler Closed;
    event EventHandler<COPYDATASTRUCT> OnWindowCopyData;

    long WindowHandle { get; }

    bool Enabled { get; set; }
    void ShowView(IView parentView);
    void CloseView();
    void Invoke(InvokeDelegate d);
  }

  public static class ControlExtensions
  {
    //allow us to invoke events on the view UI thread
    public static void Invoke(this IView view, InvokeDelegate d)
    {
      view.Invoke(d);
    }
  }
}