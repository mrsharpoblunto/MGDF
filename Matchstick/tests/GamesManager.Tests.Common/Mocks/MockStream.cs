using System;
using System.IO;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
  public class IOEventArgs : EventArgs
  {
    public byte[] Data { get; set; }
    public int Offset { get; set; }
    public int Length { get; set; }
  }

  public class MockStream : Stream, IDisposable
  {
    public event EventHandler OnRead;
    public event EventHandler OnWrite;

    private MemoryStream _stream;
    private byte[] _data;

    public MockStream()
    {
    }

    public MockStream(byte[] bytes)
    {
      _data = bytes;
    }

    public void Open()
    {
      _stream = new MemoryStream();
      if (_data != null) _stream.Write(_data, 0, _data.Length);
    }

    public override void Write(byte[] data, int offset, int length)
    {
      if (OnWrite != null)
      {
        OnWrite(this, new IOEventArgs { Data = data, Offset = offset, Length = length });
      }
      _stream.Write(data, offset, length);
      _data = _stream.ToArray();
    }

    public override void Flush()
    {
      _data = _stream.ToArray();
      _stream.Flush();
    }

    public override void Close()
    {
      _data = _stream.ToArray();
      _stream.Close();
    }

    protected override void Dispose(bool disposing)
    {
      if (disposing)
        _stream.Dispose();

      base.Dispose(disposing);
    }

    public override bool CanRead
    {
      get { return _stream.CanRead; }
    }

    public override bool CanSeek
    {
      get { return _stream.CanSeek; }
    }

    public override bool CanWrite
    {
      get { return _stream.CanWrite; }
    }

    public override long Length
    {
      get { return _data.Length; }
    }

    public override long Position
    {
      get { return _stream.Position; }
      set { _stream.Position = value; }
    }

    public override int Read(byte[] buffer, int offset, int count)
    {
      if (OnRead != null)
      {
        OnRead(this, new IOEventArgs { Data = buffer, Offset = offset, Length = count });
      }
      return _stream.Read(buffer, offset, count);
    }

    public override long Seek(long offset, SeekOrigin origin)
    {
      return _stream.Seek(offset, origin);
    }

    public override void SetLength(long value)
    {
      _stream.SetLength(value);
    }
  }
}