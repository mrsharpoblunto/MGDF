using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;

namespace MGDF.GamesManager.Tests.Common.Mocks
{
  public class MockRegistryKey : IRegistryKey
  {
    private readonly Dictionary<string, List<string>> _keyValues = new Dictionary<string, List<string>>();
    private readonly string _name;

    public MockRegistryKey(string name)
    {
      _name = name;
    }

    public string Name
    {
      get { return _name; }
    }

    public void CreateDWordValue(string name, int value)
    {
      CreateValue(name, value.ToString());
    }

    public void CreateValue(string name, string value)
    {
      if (!_keyValues.ContainsKey(name))
      {
        _keyValues.Add(name, new List<string> { value });
      }
      else
      {
        _keyValues[name] = new List<string> { value };
      }
    }

    public void CreateValues(string name, List<string> values)
    {
      if (!_keyValues.ContainsKey(name))
      {
        _keyValues.Add(name, values);
      }
      else
      {
        _keyValues[name] = values;
      }
    }

    public string GetValue(string name)
    {
      return _keyValues[name][0];
    }

    public string TryGetValue(string name)
    {
      return !_keyValues.ContainsKey(name) ? null : _keyValues[name][0];
    }

    public uint GetDwordValue(string name)
    {
      return UInt32.Parse(_keyValues[name][0]);
    }

    public int TryGetDwordValue(string name, out uint value)
    {
      value = 0;
      if (!_keyValues.ContainsKey(name)) return 2;
      value = UInt32.Parse(_keyValues[name][0]);
      return 0;
    }

    public List<string> GetValues(string name)
    {
      return _keyValues[name];
    }

    public void DeleteValue(string name)
    {
      if (_keyValues.ContainsKey(name))
      {
        _keyValues.Remove(name);
      }
    }
  }

  public class MockRegistry : IRegistry
  {
    private readonly Dictionary<BaseRegistryKey, List<MockRegistryKey>> _keyMap =
        new Dictionary<BaseRegistryKey, List<MockRegistryKey>>();

    public IRegistryKey CreateSubKey(BaseRegistryKey baseKey, string name)
    {
      if (!_keyMap.ContainsKey(baseKey))
      {
        _keyMap.Add(baseKey, new List<MockRegistryKey>());
      }

      foreach (MockRegistryKey key in _keyMap[baseKey])
      {
        if (key.Name.Equals(name, StringComparison.InvariantCultureIgnoreCase))
        {
          return key;
        }
      }

      MockRegistryKey newKey = new MockRegistryKey(name);
      _keyMap[baseKey].Add(newKey);
      return newKey;
    }

    public IRegistryKey OpenSubKey(BaseRegistryKey baseKey, string name)
    {
      foreach (MockRegistryKey key in _keyMap[baseKey])
      {
        if (key.Name.Equals(name, StringComparison.InvariantCultureIgnoreCase))
        {
          return key;
        }
      }

      return null;
    }

    public void DeleteKey(BaseRegistryKey baseKey, string name)
    {
      if (_keyMap.ContainsKey(baseKey))
      {
        _keyMap[baseKey].RemoveAll(m => m.Name == name);
      }
    }

    public void AddFakeKey(BaseRegistryKey baseKey, MockRegistryKey fakeKey)
    {
      if (!_keyMap.ContainsKey(baseKey))
      {
        _keyMap.Add(baseKey, new List<MockRegistryKey>());
      }
      _keyMap[baseKey].Add(fakeKey);
    }
  }
}