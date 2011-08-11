using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using Microsoft.Win32;

namespace MGDF.GamesManager.Common.Framework
{
    public enum BaseRegistryKey : uint
	{
		ClassesRoot = 0x80000000,
		CurrentUser = 0x80000001,
		LocalMachine = 0x80000002,
		Users = 0x80000003
	}

    public interface IRegistry
	{
		IRegistryKey CreateSubKey(BaseRegistryKey baseKey, string name);
		IRegistryKey OpenSubKey(BaseRegistryKey baseKey, string name);
		void DeleteKey(BaseRegistryKey baseKey, string name);
	}

	public interface IRegistryKey
	{
	    void CreateDWordValue(string name, int value);
		void CreateValue(string name, string value);
		void CreateValues(string name, List<string> values);
		string GetValue(string name);
		string TryGetValue(string name);
		uint GetDwordValue(string name);
		List<string> GetValues(string name);
		void DeleteValue(string name);
		int TryGetDwordValue(string name, out uint value);
	}

    public class RegistryKey : IRegistryKey
	{

		[DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegSetValueEx")]
		static extern int RegSetValueEx(
				UIntPtr hKey,
				string lpValueName,
				int Reserved,
				RegistryValueKind dwType,
				IntPtr lpData,
				int cbData
				);


		[DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegQueryValueEx")]
		static extern int RegQueryValueEx(
				UIntPtr hKey,
				string lpValueName,
				int lpReserved,
				ref uint lpType,
				char[] lpData,
				ref uint lpcbData);

		[DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegCloseKey")]
		private static extern int RegCloseKey(
				UIntPtr hKey
				);

		[DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegDeleteValue")]
		private static extern int RegDeleteValue(
				UIntPtr hKey,
				string lpValueName
				);

		private UIntPtr _handle;

		public RegistryKey(UIntPtr handle)
		{
			_handle = handle;
		}

		~RegistryKey()
		{
			RegCloseKey(Handle);
		}

        public void CreateDWordValue(string name, int value)
        {
            int size = 4;
            IntPtr ptr = Marshal.AllocHGlobal(size);
            Marshal.WriteInt32(ptr, 0, value);

            int retValue = RegSetValueEx(Handle, name, 0, RegistryValueKind.DWord, ptr, size);
            if (retValue != 0)
            {
                throw new Exception("Unable to write value. Error code:" + retValue);
            }
        }

		public void CreateValue(string name, string value)
		{
			int size = (value.Length + 1) * Marshal.SystemDefaultCharSize;
			IntPtr ptr = Marshal.StringToHGlobalAuto(value);

			int retValue = RegSetValueEx(Handle, name, 0, RegistryValueKind.String, ptr, size);
			if (retValue != 0)
			{
				throw new Exception("Unable to write value. Error code:" + retValue);
			}
		}

		public void CreateValues(string name, List<string> values)
		{
			string value = string.Empty;
			int size = 0;
			foreach (var v in values)
			{
				size += (v.Length + 1) * Marshal.SystemDefaultCharSize;
				value += v + '\0';
			}
			IntPtr ptr = Marshal.StringToHGlobalAuto(value);

			int retValue = RegSetValueEx(Handle, name, 0, RegistryValueKind.MultiString, ptr, size);
			if (retValue != 0)
			{
				throw new Exception("Unable to write values. Error code:" + retValue);
			}
		}

		public string GetValue(string name)
		{
			uint keyType=0;
			uint length = 0;
			int retValue = RegQueryValueEx(Handle, name, 0, ref keyType, null, ref length);

			char[] pvData = new char[(int)length / Marshal.SystemDefaultCharSize];
			retValue = RegQueryValueEx(Handle, name, 0, ref keyType, pvData, ref length);

			if (retValue != 0)
			{
				throw new Exception("Unable to read value for '" + name + "'. Error code:" + retValue);
			}
			return new string(pvData, 0, pvData.Length-1);
		}

		public string TryGetValue(string name)
		{
			uint keyType = 0;
			uint length = 0;
			RegQueryValueEx(Handle, name, 0, ref keyType, null, ref length);

			char[] pvData = new char[(int)length / Marshal.SystemDefaultCharSize];
			int retValue = RegQueryValueEx(Handle, name, 0, ref keyType, pvData, ref length);

			return retValue != 0 ? null : new string(pvData, 0, pvData.Length - 1);
		}

		public uint GetDwordValue(string name)
		{
			uint keyType = 4; //DWORD
			uint length = sizeof(int);

			char[] pvData = new char[length];
			int retValue = RegQueryValueEx(Handle, name, 0, ref keyType, pvData, ref length);

			if (retValue != 0)
			{
				throw new Exception("Unable to read value for '" + name + "'. Error code:" + retValue);
			}

			byte[] bytes = new byte[4];
			for (int i = 0; i < 4; i++)
				bytes[i] = (byte)pvData[i];

			return BitConverter.ToUInt32(bytes, 0);
		}

		public int TryGetDwordValue(string name, out uint value)
		{
			uint keyType = 4; //DWORD
			uint length = sizeof(int);

			char[] pvData = new char[length];
			int retValue = RegQueryValueEx(Handle, name, 0, ref keyType, pvData, ref length);

			if (retValue == 0)
			{
				byte[] bytes = new byte[4];
				for(int i = 0; i < 4; i++)
					bytes[i] = (byte) pvData[i];

				value = BitConverter.ToUInt32(bytes, 0);
			}
			else
			{
				value = 0;
			}

			return retValue;
		}

		public List<string> GetValues(string name)
		{
			uint keyType=0;
			uint length = 0;
			int retValue = RegQueryValueEx(Handle, name, 0, ref keyType, null, ref length);

			char[] pvData = new char[(int)length / Marshal.SystemDefaultCharSize];
			retValue = RegQueryValueEx(Handle, name, 0, ref keyType, pvData, ref length);

			if (retValue != 0)
			{
				throw new Exception("Unable to read value for '"+name+"'. Error code:" + retValue);
			}

			List<string> results = new List<string>();
			int index = 0;
			foreach (char c in pvData)
			{
				if (index == results.Count)
				{
					results.Add(string.Empty);
				}

				if (c != '\0')
				{
					results[index] += c;
				}
				else
				{
					++index;
				}

			}

			return results;
		}

		public void DeleteValue(string name)
		{
			int retValue = RegDeleteValue(_handle, name);
			if (retValue != 0)
			{
				throw new Exception("Unable to delete value for '" + name + "'. Error code:" + retValue);
			}
		}

		internal UIntPtr Handle
		{
			get { return _handle; }
		}
	}

    public class Registry : IRegistry
    {
        public static IRegistry Current
        {
            get { return ServiceLocator.Current.Get<IRegistry>(); }
            set { ServiceLocator.Current.Register(value); }
        }

        private enum RegWow64Options
        {
            None = 0,
            KeyWow6464Key = 0x0100,
            KeyWow6432Key = 0x0200,
        }

        private enum RegistryRights
        {
            ReadKey = 0x20019,
            WriteKey = 0x20006
        }

        public enum RegistryKeyAction : uint
        {
            CreatedNewKey = 0x00000001,
            OpenedExisting = 0x00000002
        }

        const int MAX_PATH = 260;

        [DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegCreateKeyEx")]
        private static extern int RegCreateKeyEx(
                UIntPtr hKey,
                string lpSubKey,
                uint reserved,
                string lpClass,
                uint dwOptions,
                uint samDesired,
                uint lpSecurityAttributes,
                out UIntPtr phkResult,
                out uint lpdwDisposition
                );

        [DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegOpenKeyEx")]
        static extern int RegOpenKeyEx(
                UIntPtr hKey,
                string subKey,
                uint options,
                int sam,
                out UIntPtr phkResult);

        [DllImport("advapi32.dll", EntryPoint = "RegDeleteKeyEx", SetLastError = true)]
        public static extern int RegDeleteKeyEx(
                UIntPtr hKey,
                string subKey,
                uint samDesired,
                uint Reserved);

        [DllImport("advapi32.dll", EntryPoint = "RegDeleteKey", SetLastError = true)]
        public static extern int RegDeleteKey(
                UIntPtr hKey,
                string subKey);

        [DllImport("advapi32.dll", CharSet = CharSet.Unicode, EntryPoint = "RegCloseKey")]
        private static extern int RegCloseKey(
                UIntPtr hKey
                );

        [DllImport("advapi32.dll", EntryPoint = "RegEnumKeyEx")]
        extern private static int RegEnumKeyEx(UIntPtr hkey,
            uint index,
            StringBuilder lpName,
            ref uint lpcbName,
            IntPtr reserved,
            IntPtr lpClass,
            IntPtr lpcbClass,
            out long lpftLastWriteTime);

        public IRegistryKey CreateSubKey(BaseRegistryKey baseKey, string name)
        {
            uint action;
            UIntPtr subKey;
            int retValue = RegCreateKeyEx(new UIntPtr((uint)baseKey), name, 0, null, 0,
                                                                        (int)RegWow64Options.KeyWow6464Key | (int)RegistryRights.WriteKey | (int)RegistryRights.ReadKey, 0,
                                                                        out subKey, out action);

            if (retValue == 0)
            {
                return new RegistryKey(subKey);
            }
            return null;
        }

        public IRegistryKey OpenSubKey(BaseRegistryKey baseKey, string name)
        {
            UIntPtr subKey;
            int retValue = RegOpenKeyEx(
                    new UIntPtr((uint)baseKey),
                    name,
                    0,
                    (int)RegWow64Options.KeyWow6464Key | (int)RegistryRights.ReadKey | (int)RegistryRights.WriteKey,
                    out subKey);

            if (retValue == 0)
            {
                return new RegistryKey(subKey);
            }
            return null;
        }

        public void DeleteKey(BaseRegistryKey baseKey, string name)
        {
            DeleteKeyRecursive(new UIntPtr((uint)baseKey), name);
        }

        private static void DeleteKeyRecursive(UIntPtr baseKey, string name)
        {
            //try to delete the key
            int retVal;
            if (EnvironmentSettings.Current.OSArchitecture == 32 && EnvironmentSettings.Current.OSVersion.Major == 5)
            {
                retVal = RegDeleteKey(new UIntPtr((uint)baseKey), name);
            }
            else
            {
                retVal = RegDeleteKeyEx(new UIntPtr((uint) baseKey), name, (uint) RegWow64Options.KeyWow6464Key, 0);
            }
            if (retVal == 0) return;

            //if it fails, open up the key
            UIntPtr subKey;
            retVal = RegOpenKeyEx(
                new UIntPtr((uint)baseKey),
                name,
                0,
                (int)RegWow64Options.KeyWow6464Key | (int)RegistryRights.ReadKey,
                out subKey);

            if (retVal != 0)
            {
                throw new Exception("Unable to open key '" + name + "'. Error code:" + retVal);
            }

            //loop through the keys children recursively and delete all of them
            while (retVal == 0)
            {
                uint dwSize = MAX_PATH;
                long lastWriteTime;
                StringBuilder subName = new StringBuilder(MAX_PATH);
                retVal = RegEnumKeyEx(subKey, 0, subName, ref dwSize, IntPtr.Zero, IntPtr.Zero, IntPtr.Zero, out lastWriteTime);

                if (retVal == 0)
                {
                    DeleteKeyRecursive(baseKey, Path.Combine(name, subName.ToString()));
                }
            }

            //close the parent key, then delete it now that its empty
            RegCloseKey(subKey);

            if (EnvironmentSettings.Current.OSArchitecture == 32 && EnvironmentSettings.Current.OSVersion.Major == 5)
            {
                retVal = RegDeleteKey(new UIntPtr((uint)baseKey), name);
            }
            else
            {
                retVal = RegDeleteKeyEx(new UIntPtr((uint)baseKey), name, (uint)RegWow64Options.KeyWow6464Key, 0);
            }
            if (retVal != 0)
            {
                throw new Exception("Unable to delete key '" + name + "'. Error code:" + retVal);
            }
        }
    }
}
