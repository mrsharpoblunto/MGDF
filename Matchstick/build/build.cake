using System.Text.RegularExpressions;
#addin nuget:?package=Cake.FileHelpers&version=6.1.3
#addin nuget:?package=Cake.Json&version=7.0.1
#addin nuget:?package=Cake.AWS.S3&version=1.0.0&loaddependencies=true
#addin nuget:?package=Cake.Git&version=3.0.0
#addin nuget:?package=Newtonsoft.Json&version=9.0.1
#tool nuget:?package=NUnit.ConsoleRunner&version=3.16.3

var target = Argument("target", "Default");
var buildConfiguration = Argument("configuration", "Release");
var buildNumber = (target == "Dist" || target == "Default") ? Argument<string>("buildnumber") : null;


Task("BuildX64")
	.IsDependentOn("VersionInfo")
	.IsDependentOn("SolutionInfo")
	.Does(() => {
		MSBuild("../Matchstick.sln", (new MSBuildSettings{
			Verbosity = Verbosity.Minimal,
			ToolVersion = MSBuildToolVersion.VS2022,
			Configuration = buildConfiguration,
			PlatformTarget = PlatformTarget.x64
		}).WithTarget("Clean").WithTarget("Build"));
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

	//constexpr char _mgdfVersion[] = "1.0.0.0";
  Regex versionRegex = new Regex(@"constexpr\schar\s_mgdfVersion\[\]\s=\s"".+"";");
	versionInfo = versionRegex.Replace(versionInfo,$@"constexpr char _mgdfVersion[] = ""{buildNumber}"";");

	FileWriteText("../src/core/common/MGDFVersionInfo.cpp", versionInfo);
});

Task("Documentation")
  .IsDependentOn("BuildX64")
  .Does(() => {
    // Generate docs
    CreateDirectory("../dist/documentation");
    StartProcess($@"../src/Docgen/MatchstickFramework.Docgen/bin/{buildConfiguration}/MatchstickFramework.Docgen.exe", new ProcessSettings() {
      WorkingDirectory = new DirectoryPath(".."),
      Arguments = new ProcessArgumentBuilder()
        .AppendQuoted("src/core/core.api/MGDF.idl")
        .AppendQuoted("dist/documentation/api.json")
    });
  });

Task("TestGamesManager")
	.IsDependentOn("BuildX64")
	.Does(() => {
	NUnit($@"../tests/GamesManager.Tests/bin/{buildConfiguration}/GamesManager.Tests.dll", new NUnitSettings() {
		ToolPath = "./tools/NUnit.ConsoleRunner.3.16.3/tools/nunit3-console.exe",
		StopOnError = true,
		NoResults = true
	});
});	

Task("TestCoreX64")
	.IsDependentOn("BuildX64")
	.Does(() => {
	int result = StartProcess($@"../bin/x64/{buildConfiguration}/core.tests.exe", new ProcessSettings() {
		WorkingDirectory = new DirectoryPath($@"../bin/x64/{buildConfiguration}"),
	});
	if (result != 0) {
		throw new Exception("Failures reported in the x64 unit tests");
	}
});

Task("Clean").Does(() => {
	if (DirectoryExists("../dist")) {
		DeleteDirectory("../dist", new DeleteDirectorySettings() {
		  Force = true,
		  Recursive = true,
		});
	}
	CreateDirectory("../dist");
	if (DirectoryExists("../bin/x64/{buildConfiguration}")) {
    DeleteDirectory($@"../bin/x64/{buildConfiguration}", new DeleteDirectorySettings() {
      Force = true,
      Recursive = true,
    });
	}
});

Task("Sign")
	.IsDependentOn("BuildX64")
	.Does(() => {
    var files = GetFiles($@"../bin/x64/{buildConfiguration}/*.exe");
    files.Add(GetFiles($@"../vendor/lib/x64/{buildConfiguration}/*.exe"));
    files.Add(GetFiles($@"../src/GamesManager/GamesManager.PackageGen/bin/{buildConfiguration}/*.exe"));
		var signSecret = Argument<string>("signsecret");
    Sign(files, new SignToolSignSettings {
			TimeStampUri = new Uri("http://timestamp.digicert.com"),
			DigestAlgorithm = SignToolDigestAlgorithm.Sha256,
			CertPath = "../straylight.cer",
			ArgumentCustomization = args => {
				args.Clear();
				args.Append("sign")
				.Append("/td")
        .Append("sha256")
        .Append("/fd")
        .Append("sha256")
        .Append("/tr")
        .Append("http://timestamp.digicert.com")
				.Append("/f")
				.AppendQuoted("../straylight.cer")
        .Append("/csp")
				.AppendQuoted("eToken Base Cryptographic Provider")
				.Append("/kc")
				.AppendQuoted(signSecret);
				foreach (var file in files) {
					args.AppendQuoted(file.FullPath);
				}
				return args;
       }
    });
});

Task("Dist")
	.IsDependentOn("Clean")
	.IsDependentOn("Sign")	
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
		CopyFiles(GetFiles($@"../bin/x64/{buildConfiguration}/*.exe.config"), "../dist/tmp/x64");
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
		CreateDirectory("../dist/SDK/include/MGDF");
		CopyFiles(GetFiles("../include/MGDF/*.h*"), "../dist/SDK/include/MGDF");
    
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

Task("Publish").Does(async () => {
	var s3AccessKey = Argument<string>("s3accesskey");
	var s3SecretKey = Argument<string>("s3secretkey");
	var dist = Argument<string>("dist");
	var backup = Argument<string>("backup");

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

	CopyDirectory(dist, $@"{backup}/{distVersion}");

	foreach (var file in distFiles) {
		Information("Uploading " + file.FullPath);
		await S3Upload(file, file.GetFilename().ToString(), new UploadSettings() {
			AccessKey = s3AccessKey,
			SecretKey = s3SecretKey,
			Region = RegionEndpoint.USEast1,
			BucketName = "s3.matchstickframework.org",
			CannedACL = S3CannedACL.PublicRead,
		});
	}

	if (DirectoryExists("gh-pages")) {
		DeleteDirectory("gh-pages", new DeleteDirectorySettings() { 
			Force = true,
			Recursive = true
		});
	}

	StartProcess("git", new ProcessSettings() {
		WorkingDirectory = new DirectoryPath("."),
		Arguments = new ProcessArgumentBuilder()
			.Append("clone")
			.Append("-b")
			.Append("gh-pages")
			.Append("--single-branch")
			.Append("git@github.com:mrsharpoblunto/MGDF.git")
			.Append("gh-pages")
	});

	// Copy docs
  CopyFile($@"../dist/documentation/api.json", $@"gh-pages/_data/api.json");

	// Update the website download link
	string configYaml = FileReadText("gh-pages/_config.yml");
	var versionRegex = new Regex("sdkversion:\\s\"(.*?)\"");
	configYaml = versionRegex.Replace(configYaml, "sdkversion: \"" + distVersion + "\"");
	var urlRegex = new Regex("sdkdownloadurl:\\s\"https://s3.matchstickframework.org/MGDF_(.*?)_SDK.zip");
	configYaml = urlRegex.Replace(configYaml, "sdkdownloadurl: \"https://s3.matchstickframework.org/MGDF_" + distVersion + "_SDK.zip");
	FileWriteText("gh-pages/_config.yml", configYaml);

	// commit the website changes
	StartProcess("git", new ProcessSettings() {
		WorkingDirectory = new DirectoryPath("gh-pages"),
		Arguments = new ProcessArgumentBuilder()
			.Append("add")
			.Append("-A")
	});
	if (StartProcess("git", new ProcessSettings() {
		WorkingDirectory = new DirectoryPath("gh-pages"),
		Arguments = new ProcessArgumentBuilder()
			.Append("commit")
			.Append("-m")
			.AppendQuoted($@"Publishing release {distVersion}")
	}) == 0) {
		// only push if changes were committed
		StartProcess("git", new ProcessSettings() {
			WorkingDirectory = new DirectoryPath("gh-pages"),
			Arguments = new ProcessArgumentBuilder()
				.Append("push")
				.Append("origin")
				.Append("gh-pages")
		});
	}
	DeleteDirectory("gh-pages", new DeleteDirectorySettings() { 
		Force = true,
		Recursive = true
	});
});

Task("Default")
	.IsDependentOn("Dist")
	.Does(() => {
	});

RunTarget(target);