using System;
using System.Drawing;
using System.IO;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace MGDF.Tools
{
  public class ToolForm : Form
  {
    protected readonly LinkLabel Body;
    protected readonly ProgressBar Progress;
    protected readonly Button Primary;
    protected readonly Button Secondary;
    private readonly PictureBox _picture;
    private readonly Label _heading;
    private readonly FlowLayoutPanel _buttons;
    private readonly TextBox _error;
    private readonly Button _copy;

    [DllImport("user32.dll")]
    private static extern bool SetProcessDpiAwarenessContext(IntPtr context);

    public static void EnableStyles()
    {
      try { SetProcessDpiAwarenessContext(new IntPtr(-4)); }
      catch (EntryPointNotFoundException) { }
      Application.EnableVisualStyles();
      Application.SetCompatibleTextRenderingDefault(false);
    }

    public ToolForm(string gameName, string iconPath, string heading, string body, string primary, string secondary, string errorText = null)
    {
      Text = gameName;
      Font = new Font("Segoe UI", 9F);
      AutoScaleDimensions = new SizeF(96, 96);
      AutoScaleMode = AutoScaleMode.Dpi;
      FormBorderStyle = FormBorderStyle.FixedDialog;
      MaximizeBox = false;
      MinimizeBox = false;
      StartPosition = FormStartPosition.CenterScreen;
      ClientSize = new Size(460, 100);
      _picture = new PictureBox { SizeMode = PictureBoxSizeMode.Zoom };
      try
      {
        if (File.Exists(iconPath))
          using (var image = Image.FromFile(iconPath)) _picture.Image = new Bitmap(image);
      }
      catch (Exception ex) when (ex is IOException || ex is ArgumentException || ex is UnauthorizedAccessException) { }
      _heading = new Label { Text = heading, Font = new Font("Segoe UI", 12F), AutoSize = false };
      Body = new LinkLabel { Text = body, LinkArea = new LinkArea(0, 0), AutoSize = false, UseCompatibleTextRendering = true };
      Progress = new ProgressBar { Visible = false };
      Primary = new Button { Text = primary, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, MinimumSize = new Size(108, 28) };
      Secondary = new Button { Text = secondary, AutoSize = true, AutoSizeMode = AutoSizeMode.GrowAndShrink, MinimumSize = new Size(108, 28) };
      _buttons = new FlowLayoutPanel { FlowDirection = FlowDirection.RightToLeft, AutoSize = true, WrapContents = false };
      Primary.Margin = new Padding(0, 0, 8, 0);
      Secondary.Margin = Padding.Empty;
      _buttons.Controls.AddRange(new Control[] { Secondary, Primary });
      if (!string.IsNullOrEmpty(errorText))
      {
        _error = new TextBox { Text = errorText, Multiline = true, ReadOnly = true,
          WordWrap = true, ScrollBars = ScrollBars.Vertical };
        _copy = new Button { Text = "Copy", AutoSize = true };
        _copy.Click += (sender, e) => {
          try { Clipboard.SetText(_error.Text); }
          catch (ExternalException) { _copy.Text = "Try again"; }
        };
        Controls.AddRange(new Control[] { _error, _copy });
      }
      AcceptButton = Primary;
      CancelButton = Secondary;
      Controls.AddRange(new Control[] { _picture, _heading, Body, Progress, _buttons });
      ActiveControl = Primary;
      Body.TextChanged += (sender, e) => PerformLayout();
      Progress.VisibleChanged += (sender, e) => PerformLayout();
    }

    protected override void OnLayout(LayoutEventArgs e)
    {
      base.OnLayout(e);
      if (Body == null) return;
      float scale = Math.Max(DeviceDpi / 96F, Font.Size / 9F);
      int margin = (int)Math.Round(16 * scale);
      int width = ClientSize.Width - margin * 2;
      int icon = (int)Math.Round(48 * scale);
      _picture.SetBounds(margin, margin, icon, icon);
      int titleWidth = width - icon - margin;
      int titleHeight = _heading.GetPreferredSize(new Size(titleWidth, 0)).Height;
      int rowHeight = Math.Max(icon, titleHeight);
      _heading.SetBounds(margin + icon + margin, margin + (rowHeight - titleHeight) / 2, titleWidth, titleHeight);
      int top = margin + rowHeight + margin / 2;
      int bodyHeight = Body.GetPreferredSize(new Size(width, 0)).Height;
      Body.SetBounds(margin, top, width, bodyHeight);
      top += bodyHeight + margin;
      if (_error != null)
      {
        int height = Font.Height * 4 + (int)Math.Round(8 * scale);
        int gap = margin / 2;
        var copy = _copy.GetPreferredSize(Size.Empty);
        _error.SetBounds(margin, top, width - copy.Width - gap, height);
        _copy.SetBounds(margin + width - copy.Width, top, copy.Width, copy.Height);
        top += height + margin;
      }
      if (Progress.Visible)
      {
        int height = (int)Math.Round(8 * scale);
        Progress.SetBounds(margin, top, width, height);
        top += height + margin;
      }
      var buttons = _buttons.GetPreferredSize(new Size(width, 0));
      _buttons.SetBounds(margin, top, width, buttons.Height);
      int heightNeeded = top + buttons.Height + margin;
      if (ClientSize.Height != heightNeeded) ClientSize = new Size(ClientSize.Width, heightNeeded);
    }

    protected override void Dispose(bool disposing)
    {
      if (disposing) _picture.Image?.Dispose();
      base.Dispose(disposing);
    }
  }
}
