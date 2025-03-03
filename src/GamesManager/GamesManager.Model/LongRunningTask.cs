using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace MGDF.GamesManager.Model
{
  public enum LongRunningTaskResult
  {
    Completed,
    Cancelled,
    Error
  }

  public abstract class LongRunningTask
  {
    public uint Progress { get; protected set; }
    public uint Total { get; protected set; }

    public abstract LongRunningTaskResult Start();
  }
}