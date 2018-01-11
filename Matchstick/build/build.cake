var target = Argument("target", "Default");
var buildConfiguration = Argument("build_configuration", "Release");
var buildNumber = Argument("build_number", "1.0.0");

Task("BuildX64").Does(() => {
  MSBuild("../Matchstick.sln", new MSBuildSettings{
	Verbosity = Verbosity.Minimal,
	ToolVersion = MSBuildToolVersion.VS2017,
    Configuration = buildConfiguration,
    PlatformTarget = PlatformTarget.x64
  });
});

Task("BuildX86").Does(() => {
  MSBuild("../Matchstick.sln", new MSBuildSettings{
	Verbosity = Verbosity.Minimal,
	ToolVersion = MSBuildToolVersion.VS2017,
    Configuration = buildConfiguration,
    PlatformTarget = PlatformTarget.x86
  });
});

Task("Documentation").Does(() => {
	StartProcess("tools/Doxygen/tools/doxygen.exe", new ProcessSettings() {
		WorkingDirectory = new DirectoryPath("."),
		Arguments = new ProcessArgumentBuilder().AppendQuoted("documentation.Doxyfile")
	});
});

Task("TestGamesManager")
	.IsDependentOn("BuildX86")
	.IsDependentOn("BuildX64")
	.Does(() => {
	NUnit3($@"../tests/GamesManager.Tests/bin/{buildConfiguration}/GamesManager.Tests.dll", new NUnit3Settings() {
		StopOnError = true,
		TeamCity = true,
		NoResults = true
	});
});	

Task("TestCoreX86")
	.IsDependentOn("BuildX86")
	.IsDependentOn("BuildX64")
	.Does(() => {
	int result = StartProcess($@"../bin/Win32/{buildConfiguration}/core.tests.exe", new ProcessSettings() {
		WorkingDirectory = new DirectoryPath($@"../bin/Win32/{buildConfiguration}"),
		Arguments = new ProcessArgumentBuilder().Append("--teamcity")
	});
	if (result != 0) {
		throw new Exception("Failures reported in the x86 unit tests");
	}
});
 
Task("TestCoreX64")
	.IsDependentOn("BuildX86")
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

Task("Dist")
	.IsDependentOn("BuildX86")
	.IsDependentOn("BuildX64")
	.IsDependentOn("TestGamesManager")
	.IsDependentOn("TestCoreX86")
	.IsDependentOn("TestCoreX64")
	.IsDependentOn("Documentation")
	.Does(() => {
		CreateDirectory("../dist");
		CleanDirectory("../dist");
		CreateDirectory("dist/tmp");
		CreateDirectory("../dist/SDK");

		CreateDirectory("../dist/tmp/x86");
		CopyFiles(GetFiles($@"../bin/Win32/{buildConfiguration}/*.dll"), "../dist/tmp/x86");
		CopyFiles(GetFiles($@"../bin/Win32/{buildConfiguration}/*.exe"), "../dist/tmp/x86");
		CopyDirectory($@"../bin/Win32/{buildConfiguration}/dependancies", "../dist/tmp/x86/dependancies");
		CopyDirectory($@"../bin/Win32/{buildConfiguration}/resources", "../dist/tmp/x86/resources");

		CreateDirectory("../dist/tmp/x64");
		CopyFiles(GetFiles($@"../bin/x64/{buildConfiguration}/*.dll"), "../dist/tmp/x64");
		CopyFiles(GetFiles($@"../bin/x64/{buildConfiguration}/*.exe"), "../dist/tmp/x64");
		CopyDirectory($@"../bin/x64/{buildConfiguration}/dependancies", "../dist/tmp/x64/dependancies");
		CopyDirectory($@"../bin/x64/{buildConfiguration}/resources", "../dist/tmp/x64/resources");

		DeleteFiles(GetFiles("../dist/tmp/**/core.tests.exe"));
		DeleteFiles(GetFiles("../dist/tmp/**/*.vshost.exe"));
		Zip("../dist/tmp/x86", $@"../dist/SDK/MGDF_{buildNumber}_x86.zip");
		Zip("../dist/tmp/x64", $@"../dist/SDK/MGDF_{buildNumber}_x64.zip");
		DeleteDirectory("../dist/tmp", new DeleteDirectorySettings() { 
			Force = true,
			Recursive = true
		});

		// TODO copy debug symbols
		// TODO copy MGDF headers
		// TODO generate latest.json
		// TODO copy packagegen
		// TODO copy sdk samples
		// TODO zip up sdk
		
	});
// TODO upload zip file to AWS
		
// TODO need to clean bin & doc folders

Task("Default")
	.IsDependentOn("Dist")
	.Does(() => {
	});

RunTarget(target);
