using System.Text.RegularExpressions;
#addin nuget:?package=Cake.FileHelpers
#addin nuget:?package=Cake.Json
#addin nuget:?package=Cake.AWS.S3
#addin nuget:?package=Newtonsoft.Json&version=9.0.1

var target = Argument("target", "Default");
var buildConfiguration = Argument("configuration", "Release");
var buildNumber = Argument("buildnumber", "1.0.0");

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
	DeleteDirectory($@"../bin/x64/{buildConfiguration}", new DeleteDirectorySettings() {
		Force = true,
		Recursive = true,
	});
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
		CopyDirectory($@"../dependencies/x64", "../dist/tmp/x64/dependencies");
		CopyDirectory($@"../content/resources", "../dist/tmp/x64/resources");
		DeleteFiles(GetFiles("../dist/tmp/**/core.tests.exe"));
		DeleteFiles(GetFiles("../dist/tmp/**/*.vshost.exe"));
		Zip("../dist/tmp/x64", $@"../dist/SDK/MGDF_{buildNumber}_x64.zip");
		CopyFile($@"../dist/SDK/MGDF_{buildNumber}_x64.zip", $@"../dist/MGDF_{buildNumber}_x64.zip");
		DeleteDirectory("../dist/tmp", new DeleteDirectorySettings() { 
			Force = true,
			Recursive = true
		});

		// generate the latest.json file for use by other games
		SerializeJsonToPrettyFile("../dist/latest.json", new {
			framework = new {
				version = buildNumber,
				url = $@"https://s3.matchstickframework.org/MGDF_{buildNumber}_x64.zip",
				md5 = CalculateFileHash($@"../dist/SDK/MGDF_{buildNumber}_x64.zip", HashAlgorithm.MD5).ToHex(),
			}
		});

		// copy debug symbols
		CreateDirectory("../dist/symbols");
		CreateDirectory("../dist/symbols/x64");
		CopyFiles(GetFiles($@"../bin/x64/{buildConfiguration}/*.dll"), "../dist/symbols/x64");
		CopyFiles(GetFiles($@"../bin/x64/{buildConfiguration}/*.exe"), "../dist/symbols/x64");
		CopyFiles(GetFiles($@"../bin/x64/{buildConfiguration}/*.pdb"), "../dist/symbols/x64");
		CopyFiles(GetFiles($@"../vendor/lib/x64/{buildConfiguration}/*.dll"), "../dist/symbols/x64");
		CopyFiles(GetFiles($@"../vendor/lib/x64/{buildConfiguration}/*.exe"), "../dist/symbols/x64");
		CopyFiles(GetFiles($@"../vendor/lib/x64/{buildConfiguration}/*.pdb"), "../dist/symbols/x64");

		// copy MGDF headers
		CopyDirectory("../include", "../dist/SDK/include");
    
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

Task("UploadDist").Does(() => {
	var s3AccessKey = Argument<string>("s3accesskey");
	var s3SecretKey = Argument<string>("s3secretkey");
	var dist = Argument<string>("dist");

	string distVersion = null;
	var distFiles = GetFiles($@"{dist}/MGDF_*.zip");
	Regex SDKMatcher = new Regex("^.*MGDF_(([0-9]\\.?)*)_(SDK|x64).zip$");
	foreach (var file in distFiles) {
		Match m = SDKMatcher.Match(file.FullPath);
		if (!m.Success) {
			throw new Exception($@"{file.FullPath} to publish doesn't appear to be a valid MGDF SDK or redistributable");
		}
		
		string dv = m.Groups[1].Captures[0].ToString();
		if (distVersion == null) {
			distVersion = dv;
			Information($@"MGDF SDK is version {distVersion}");
		} else if (dv != distVersion) {
			throw new Exception($@"The specified SDK & redistributable files in {dist} don't have matching version numbers.");
		}
	}

	if (distVersion == null) {
		throw new Exception($@"The specified dist folder {dist} doesn't appear to be valid, no SDK or redistributable found.");
	}

	foreach (var file in distFiles) {
		S3Upload(file, file.GetFilename().ToString(), new UploadSettings() {
			AccessKey = s3AccessKey,
			SecretKey = s3SecretKey,
			BucketName = "s3.matchstickframework.org",
			CannedACL = S3CannedACL.PublicRead,
		});
	}
});

Task("UpdateWebsite").Does(() => {
});

Task("Publish")
	.IsDependentOn("UploadDist")
	.IsDependentOn("UpdateWebsite")
	.Does(() => {
	});

Task("Default")
	.IsDependentOn("Dist")
	.Does(() => {
	});


// TODO need a publish task that uploads the latest dist to S3 and updates the links & docs on the github website

RunTarget(target);
