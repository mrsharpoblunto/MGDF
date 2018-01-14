using System.Text.RegularExpressions;
#addin nuget:?package=Cake.FileHelpers
#addin nuget:?package=Cake.Json
#addin nuget:?package=Newtonsoft.Json&version=9.0.1

var target = Argument("target", "Default");
var buildConfiguration = Argument("build_configuration", "Release");
var buildNumber = Argument("build_number", "1.0.0");

Task("BuildX64")
  .IsDependentOn("VersionInfo")
  .IsDependentOn("SolutionInfo")
  .Does(() => {
  MSBuild("../Matchstick.sln", new MSBuildSettings{
	Verbosity = Verbosity.Minimal,
	ToolVersion = MSBuildToolVersion.VS2017,
    Configuration = buildConfiguration,
    PlatformTarget = PlatformTarget.x64
  });
});

Task("SolutionInfo").Does(() => {
	string solutionInfo = FileReadText("../src/GamesManager/SolutionInfo.cs");

	Regex productRegex = new Regex(@"\[assembly\:\sAssemblyProduct\("".+""\)\]");
	Regex assemblyVersionRegex = new Regex(@"\[assembly\:\sAssemblyVersion\("".+""\)\]");
	Regex assemblyFileVersionRegex = new Regex(@"\[assembly\:\sAssemblyFileVersion\("".+""\)\]");

	string product = "Matchstick game development framework";
	solutionInfo = productRegex.Replace(solutionInfo,$@"[assembly: AssemblyProduct(""{product}"")]");
	solutionInfo = assemblyVersionRegex.Replace(solutionInfo, $@"[assembly: AssemblyVersion(""{buildNumber}"")]");
	solutionInfo = assemblyFileVersionRegex.Replace(solutionInfo, $@"[assembly: AssemblyFileVersion(""{buildNumber}"")]");

	FileWriteText("../src/GamesManager/SolutionInfo.cs", solutionInfo);
});

Task("VersionInfo").Does(() => {
	string versionInfo = FileReadText("../src/core/common/MGDFVersionInfo.cpp");

	//const std::string MGDFVersionInfo::_mgdfVersion = "1.0.0.0";
    Regex versionRegex = new Regex(@"const\sstd\:\:string\sMGDFVersionInfo\:\:_mgdfVersion\s=\s"".+"";");
	versionInfo = versionRegex.Replace(versionInfo,$@"const std::string MGDFVersionInfo::_mgdfVersion = ""{buildNumber}"";");

	FileWriteText("../src/core/common/MGDFVersionInfo.cpp", versionInfo);
});

Task("Documentation").Does(() => {
	StartProcess("tools/Doxygen/tools/doxygen.exe", new ProcessSettings() {
		WorkingDirectory = new DirectoryPath("."),
		Arguments = new ProcessArgumentBuilder().AppendQuoted("documentation.Doxyfile")
	});
});

Task("TestGamesManager")
	.IsDependentOn("BuildX64")
	.Does(() => {
	NUnit3($@"../tests/GamesManager.Tests/bin/{buildConfiguration}/GamesManager.Tests.dll", new NUnit3Settings() {
		StopOnError = true,
		TeamCity = true,
		NoResults = true
	});
});	

Task("TestCoreX64")
	.IsDependentOn("BuildX64")
	.Does(() => {
	int result = StartProcess($@"../bin/x64/{buildConfiguration}/core.tests.exe", new ProcessSettings() {
		WorkingDirectory = new DirectoryPath($@"../bin/x64/{buildConfiguration}"),
		Arguments = new ProcessArgumentBuilder().Append("--teamcity")
	});
	if (result != 0) {
		throw new Exception("Failures reported in the x64 unit tests");
	}
});

Task("Clean").Does(() => {
	DeleteDirectory("../dist", new DeleteDirectorySettings() {
		Force = true,
		Recursive = true,
	});
	CreateDirectory("../dist");
});

Task("Dist")
	.IsDependentOn("Clean")
	.IsDependentOn("BuildX64")
	.IsDependentOn("TestGamesManager")
	.IsDependentOn("TestCoreX64")
	.IsDependentOn("Documentation")
	.Does(() => {
		// zip up the SDK binaries
		CreateDirectory("../dist/tmp");
		CreateDirectory("../dist/SDK");
		CreateDirectory("../dist/tmp/x64");
		CopyFiles(GetFiles($@"../bin/x64/{buildConfiguration}/*.dll"), "../dist/tmp/x64");
		CopyFiles(GetFiles($@"../bin/x64/{buildConfiguration}/*.exe"), "../dist/tmp/x64");
		CopyDirectory($@"../dependancies/x64", "../dist/tmp/x64/dependancies");
		CopyDirectory($@"../content/resources", "../dist/tmp/x64/resources");
		DeleteFiles(GetFiles("../dist/tmp/**/core.tests.exe"));
		DeleteFiles(GetFiles("../dist/tmp/**/*.vshost.exe"));
		Zip("../dist/tmp/x64", $@"../dist/SDK/MGDF_{buildNumber}_x64.zip");
		DeleteDirectory("../dist/tmp", new DeleteDirectorySettings() { 
			Force = true,
			Recursive = true
		});

		// generate the latest.json file for use by other games
		SerializeJsonToPrettyFile("../dist/latest.json", new {
			Framework = new {
				Version = buildNumber,
				Url = $@"https://s3.matchstickframework.org/MGDF_{buildNumber}_x64.zip",
				MD5 = CalculateFileHash($@"../dist/SDK/MGDF_{buildNumber}_x64.zip", HashAlgorithm.MD5).ToHex(),
			}
		});

		// copy debug symbols
		CreateDirectory("../dist/Symbols");
		CreateDirectory("../dist/Symbols/x64");
		CopyFiles(GetFiles($@"../bin/x64/{buildConfiguration}/*.dll"), "../dist/Symbols/x64");
		CopyFiles(GetFiles($@"../bin/x64/{buildConfiguration}/*.exe"), "../dist/Symbols/x64");
		CopyFiles(GetFiles($@"../bin/x64/{buildConfiguration}/*.pdb"), "../dist/Symbols/x64");
		CopyFiles(GetFiles($@"../vendor/lib/x64/{buildConfiguration}/*.dll"), "../dist/Symbols/x64");
		CopyFiles(GetFiles($@"../vendor/lib/x64/{buildConfiguration}/*.exe"), "../dist/Symbols/x64");
		CopyFiles(GetFiles($@"../vendor/lib/x64/{buildConfiguration}/*.pdb"), "../dist/Symbols/x64");

		// copy MGDF headers
		CopyDirectory("../include", "../dist/SDK/Include");
    
		// copy packagegen to SDK bin dir
		CreateDirectory("../dist/SDK/bin");
		CopyFile($@"../src/GamesManager/GamesManager.PackageGen/bin/{buildConfiguration}/GamesManager.PackageGen.exe", "../dist/SDK/bin/PackageGen.exe");
		CopyFile($@"../src/GamesManager/GamesManager.PackageGen/bin/{buildConfiguration}/GamesManager.PackageGen.exe.config", "../dist/SDK/bin/PackageGen.exe.config");

		// copy SDK samples
		CreateDirectory("../dist/SDK/src/samples");
		CopyDirectory("../src/samples", "../dist/SDK/src/samples");
		DeleteFiles(GetFiles("../dist/SDK/src/**/*.sdf"));
		DeleteFiles(GetFiles("../dist/SDK/src/**/*.suo"));
		DeleteFiles(GetFiles("../dist/SDK/src/**/*.user"));
		DeleteDirectories(new DirectoryPath[] {
			Directory("../dist/SDK/src/samples/EmptyGame/user"),
			Directory("../dist/SDK/src/samples/EmptyGame/bin"),
			Directory("../dist/SDK/src/samples/EmptyGame/game/bin"),
		}, new DeleteDirectorySettings() { 
			Force = true,
			Recursive = true
		});

		// zip up sdk
		Zip("../dist/SDK", $@"../dist/MGDF_{buildNumber}_SDK.zip");
		DeleteDirectory("../dist/SDK", new DeleteDirectorySettings() { 
			Force = true,
			Recursive = true
		});
	});

Task("Default")
	.IsDependentOn("Dist")
	.Does(() => {
	});

// TODO need a publish task that uploads the latest dist to S3 and updates the links & docs on the github website

RunTarget(target);
