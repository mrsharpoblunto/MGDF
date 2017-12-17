var target = Argument("target", "Default");

Task("Default").Does(() => {
  MSBuild("../Matchstick.sln", new MSBuildSettings{
	Verbosity = Verbosity.Minimal,
	ToolVersion = MSBuildToolVersion.VS2017,
    Configuration = "Release",
    PlatformTarget = PlatformTarget.x64
  });
});

RunTarget(target);
