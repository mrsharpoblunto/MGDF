using System;
using MGDF.GamesManager.Common.Framework;
using System.Globalization;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
  public class MockTimeService : ITimeService
  {
    public DateTime Now { get; set; }

    public MockTimeService()
    {
      string nowUtc = "10 Jan 2008 12:00pm";
      Now = DateTime.Parse(nowUtc, CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal).ToUniversalTime();
    }

    public MockTimeService(DateTime now)
    {
      string nowUtc = now.ToString("d MMM yyyy HH:mm:ss", CultureInfo.InvariantCulture);
      Now = DateTime.Parse(nowUtc, CultureInfo.InvariantCulture, DateTimeStyles.AssumeUniversal).ToUniversalTime();
    }
  }
}