using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common.Framework;
using MGDF.GamesManager.Model.Contracts.Entities;
using MGDF.GamesManager.Model.Factories;
using MGDF.GamesManager.Tests.Common.Mocks;
using NUnit.Framework;

namespace MGDF.GamesManager.Tests
{
    [TestFixture]
    public class XmlEntityTests: BaseTest
    {
        [Test]
        public void LoadGame()
        {
            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\games")).AddDirectory("console");
            gameDirectory.AddFile("game.xml", ReadTextFile("console.xml"));
            IGame game = EntityFactory.Current.CreateGame(Path.Combine(EnvironmentSettings.Current.AppDirectory,"games\\console\\game.xml"));
            Assert.AreEqual("Console",game.Uid);
            Assert.AreEqual("Lua Console", game.Name);
            Assert.AreEqual("A Lua command console for interacting with the MGDF system", game.Description);
            Assert.AreEqual(new Version(0,1), game.Version);
            Assert.AreEqual("no-8", game.DeveloperUid);
            Assert.AreEqual("http://www.junkship.org", game.Homepage);
            Assert.AreEqual(1, game.InterfaceVersion);
            Assert.AreEqual("no8 interactive",game.DeveloperName);
            Assert.AreEqual(true, game.IsValid);
            Assert.AreEqual("http://games.junkship.org/gamesource.asmx", game.GameSourceService);
            Assert.AreEqual("http://statistics.junkship.org/statisticsservice.asmx", game.StatisticsService);
        }

        [Test]
        public void LoadInvalidGameNoUid()
        {
            const string NoUid = @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>0.1</version>
  <interfaceversion>1</interfaceversion>
  <developer>no.8 Interactive</developer>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice> 
</mgdf:game>";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\games")).AddDirectory("console");
            gameDirectory.AddFile("game.xml", NoUid);
            IGame game = EntityFactory.Current.CreateGame(Path.Combine(EnvironmentSettings.Current.AppDirectory, "games\\console\\game.xml"));
            Assert.AreEqual(false, game.IsValid);
            Assert.AreEqual(1, game.ErrorCollection.Count);
        }

        [Test]
        public void LoadInvalidGameInvalidVersion()
        {
            const string InvalidVersion = @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>  
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0 Beta 2</version>
  <interfaceversion>1</interfaceversion>
  <developer>no.8 Interactive</developer>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice> 
</mgdf:game>";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\games")).AddDirectory("console");
            gameDirectory.AddFile("game.xml", InvalidVersion);
            IGame game = EntityFactory.Current.CreateGame(Path.Combine(EnvironmentSettings.Current.AppDirectory, "games\\console\\game.xml"));
            Assert.AreEqual(false, game.IsValid);
            Assert.AreEqual(1, game.ErrorCollection.Count);
        }

        [Test]
        public void LoadInvalidGameInvalidXML()
        {
            const string InvalidXml = @"<?xml version=""1.0"" encoding=""UTF-8""?>
<mgdf:game xmlns:mgdf=""http://schemas.matchstickframework.org/2007/game"" xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"">
  <gameuid>Console</gameuid>  
  <gamename>Lua Console</gamename>
  <description>A Lua command console for interacting with the MGDF system</description>
  <version>1.0</version>
  <interfaceversion>1</interfaceversion>
  <developeruid>no.8 Interactive</developeruid>
  <homepage>http://www.junkship.org</homepage>
  <gamesourceservice>http://games.junkship.org/gamesource.asmx</gamesourceservice>
  <statisticsservice>http://statistics.junkship.org/statisticsservice.asmx</statisticsservice>";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory+"\\games")).AddDirectory("console");
            gameDirectory.AddFile("game.xml", InvalidXml);
            IGame game = EntityFactory.Current.CreateGame(Path.Combine(EnvironmentSettings.Current.AppDirectory, "games\\console\\game.xml"));
            Assert.AreEqual(false, game.IsValid);
            Assert.AreEqual(1, game.ErrorCollection.Count);
        }

        [Test]
        public void LoadPreferences()
        {
            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\games")).AddDirectory("console");
            gameDirectory.AddFile("preferences.xml", ReadTextFile("preferences.xml"));
            IPreferences prefs = EntityFactory.Current.CreatePreferences(Path.Combine(EnvironmentSettings.Current.AppDirectory, "games\\console\\preferences.xml"));

            Assert.AreEqual(true, prefs.IsValid);
            Assert.AreEqual(9, prefs.PreferenceCollection.Count);
            Assert.AreEqual("maxFPS", prefs.PreferenceCollection[0].Name);
            Assert.AreEqual("60", prefs.PreferenceCollection[0].Value); 

            Assert.AreEqual("screenX", prefs.PreferenceCollection[4].Name);
            Assert.AreEqual("800", prefs.PreferenceCollection[4].Value); 
        }

        [Test]
        public void LoadInvalidPreferencesMissingNameElement()
        {
            const string InvalidXml = @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:preferences xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance""
	    xmlns:mgdf=""http://schemas.matchstickframework.org/2007/preferences"">
  <preference>
    <value>60</value>
  </preference>
  <preference>
    <name>volume</name>
    <value>1.0</value>
  </preference>
</mgdf:preferences>";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\games")).AddDirectory("console");
            gameDirectory.AddFile("preferences.xml", InvalidXml);
            IPreferences prefs = EntityFactory.Current.CreatePreferences(Path.Combine(EnvironmentSettings.Current.AppDirectory, "games\\console\\preferences.xml"));

            Assert.AreEqual(false, prefs.IsValid);
            Assert.AreEqual(1, prefs.ErrorCollection.Count);
        }

        [Test]
        public void LoadInvalidPreferencesInvalidXML()
        {
            const string InvalidXml = @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:preferences xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance""
	    xmlns:mgdf=""http://schemas.matchstickframework.org/2007/preferences"">
  <preference>
    <name>volume</name>
    <value>60</value>
  </preference>
  <preference>
    <name>volume</name>
    <value>1.0</value>
</mgdf:preferences>";

            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\games")).AddDirectory("console");
            gameDirectory.AddFile("preferences.xml", InvalidXml);
            IPreferences prefs = EntityFactory.Current.CreatePreferences(Path.Combine(EnvironmentSettings.Current.AppDirectory, "games\\console\\preferences.xml"));

            Assert.AreEqual(false, prefs.IsValid);
            Assert.AreEqual(1, prefs.ErrorCollection.Count);
        }

        [Test] 
        public void LoadAdditionalPreferences()
        {
            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\games")).AddDirectory("console");
            gameDirectory.AddFile("preferences.xml", ReadTextFile("preferences.xml"));
            IPreferences prefs = EntityFactory.Current.CreatePreferences(Path.Combine(EnvironmentSettings.Current.AppDirectory, "games\\console\\preferences.xml"));

            Assert.AreEqual(true, prefs.IsValid);
            Assert.AreEqual(9, prefs.PreferenceCollection.Count);
            Assert.AreEqual("maxFPS", prefs.PreferenceCollection[0].Name);
            Assert.AreEqual("60", prefs.PreferenceCollection[0].Value);

            Assert.AreEqual("screenX", prefs.PreferenceCollection[4].Name);
            Assert.AreEqual("800", prefs.PreferenceCollection[4].Value);

            const string AdditionalPreferences = @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:preferences xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance""
	    xmlns:mgdf=""http://schemas.matchstickframework.org/2007/preferences"">
  <preference>
    <name>maxFPS</name>
    <value>120</value>
  </preference>
  <preference>
    <name>newSetting</name>
    <value>1.0</value>
  </preference>
</mgdf:preferences>";
            var additionalFile = gameDirectory.AddFile("additionalPreferences.xml", AdditionalPreferences);

            prefs.LoadAdditionalPreferences(additionalFile.FullName);

            Assert.AreEqual(true, prefs.IsValid);
            Assert.AreEqual(10, prefs.PreferenceCollection.Count);

            Assert.AreEqual(1,prefs.PreferenceCollection.Where(p => p.Name == "maxFPS").Count());
            Assert.AreEqual("120", prefs.PreferenceCollection.Single(p => p.Name == "maxFPS").Value);
        }

        [Test]
        public void LoadInvalidAdditionalPreferences()
        {
            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\games")).AddDirectory("console");
            gameDirectory.AddFile("preferences.xml", ReadTextFile("preferences.xml"));
            IPreferences prefs = EntityFactory.Current.CreatePreferences(Path.Combine(EnvironmentSettings.Current.AppDirectory, "games\\console\\preferences.xml"));

            Assert.AreEqual(true, prefs.IsValid);
            Assert.AreEqual(9, prefs.PreferenceCollection.Count);

            const string AdditionalPreferences = @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:preferences xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance""
	    xmlns:mgdf=""http://schemas.matchstickframework.org/2007/preferences"">
  <preference>
    <name>maxFPS</name>
    <value>120</value>
  </preference>
  <preference>
    <value>1.0</value>
  </preference>
</mgdf:preferences>";
            var additionalFile = gameDirectory.AddFile("additionalPreferences.xml", AdditionalPreferences);

            prefs.LoadAdditionalPreferences(additionalFile.FullName);

            Assert.AreEqual(false, prefs.IsValid);
            Assert.AreEqual(1, prefs.ErrorCollection.Count);
        }

        [Test]
        public void SavePreferences()
        {
            const string PreferencesXml = @"<?xml version=""1.0"" encoding=""utf-8""?>
<mgdf:preferences xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance""
	    xmlns:mgdf=""http://schemas.matchstickframework.org/2007/preferences"">
  <preference>
    <name>maxFPS</name>
    <value>120</value>
  </preference>
  <preference>
    <name>newSetting</name>
    <value>1.0</value>
  </preference>
</mgdf:preferences>";
            MockDirectory gameDirectory = ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory + "\\games")).AddDirectory("console");
            var preferencesFile = gameDirectory.AddFile("preferences.xml", PreferencesXml);
            IPreferences prefs = EntityFactory.Current.CreatePreferences(Path.Combine(EnvironmentSettings.Current.AppDirectory, "games\\console\\preferences.xml"));

            Assert.AreEqual(true, prefs.IsValid);
            Assert.AreEqual(2, prefs.PreferenceCollection.Count);

            prefs.PreferenceCollection[0].Value = "30";
            prefs.Save(preferencesFile.FullName);

                        const string ExpectedPreferencesXml = @"<?xml version=""1.0"" encoding=""utf-8""?>"+"\r\n"+
@"<mgdf:preferences xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance"" xmlns:mgdf=""http://schemas.matchstickframework.org/2007/preferences"">"+"\r\n"+
@"    <preference>"+"\r\n"+
@"        <name>maxFPS</name>"+"\r\n"+
@"        <value>30</value>"+"\r\n"+
@"    </preference>"+"\r\n"+
@"    <preference>"+"\r\n"+
@"        <name>newSetting</name>"+"\r\n"+
@"        <value>1.0</value>"+"\r\n"+
@"    </preference>"+"\r\n"+
@"</mgdf:preferences>";

            string savedFile = string.Empty;
            using (var stream = preferencesFile.OpenStream(FileMode.Open))
            {
                using (StreamReader reader = new StreamReader(stream))
                {
                    savedFile = reader.ReadToEnd().Replace("\t", "    ");
                }
            }

            Assert.AreEqual(ExpectedPreferencesXml, savedFile);
        }

//        [Test]
//        public void LoadPreferenceTemplates()
//        {
//            CrossThreadTestRunner runner = new CrossThreadTestRunner();
//            runner.RunInSTA(() =>
//                                {

//                                    PreferenceTemplateFactory.Current = new WPFPreferenceTemplateFactory();

//                                    MockDirectory gameDirectory =
//                                        ((MockDirectory)MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory)).
//                                            AddDirectory("games").AddDirectory("console");
//                                    gameDirectory.AddFile("preferenceTemplates.xml", ReadTextFile("preferenceTemplates.xml"));
//                                    IPreferenceTemplates prefTemplates =
//                                        ModelFactory.Current.CreatePreferenceTemplates(
//                                            Path.Combine(EnvironmentSettings.Current.AppDirectory,
//                                                         "games\\console\\preferenceTemplates.xml"));
//                                    Assert.IsTrue(prefTemplates.IsValid,
//                                                  prefTemplates.ErrorCollection.Count == 0
//                                                      ? string.Empty
//                                                      : prefTemplates.ErrorCollection[0]);

//                                    const string AdditionalPreferencesXml =
//                                        @"<?xml version=""1.0"" encoding=""utf-8""?>
//<mgdf:preferencetemplates  xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance""
//	    	    xmlns:mgdf=""http://www.mgdf.org/2007/preferenceTemplates"">
//  <category name=""graphics"">
//
//    <preference name=""randomPreference"" type=""none"" label="""">
//      <selector>
//        <none/>
//      </selector>
//    </preference>
//  </category>
//</mgdf:preferencetemplates>";

//                                    gameDirectory.AddFile("additionalPreferences.xml", AdditionalPreferencesXml);
//                                    prefTemplates.LoadAdditionalTemplates(Path.Combine(EnvironmentSettings.Current.AppDirectory,
//                                                                                       "games\\console\\additionalPreferences.xml"));
//                                    Assert.IsTrue(prefTemplates.IsValid);

//                                    List<IPreferenceTemplate> graphicsTemplates = null;
//                                    List<IPreferenceTemplate> soundTemplates = null;

//                                    foreach (var template in prefTemplates)
//                                    {
//                                        if (template.Key == "graphics")
//                                        {
//                                            graphicsTemplates = template.Value;
//                                        }
//                                        else if (template.Key == "sound")
//                                        {
//                                            soundTemplates = template.Value;
//                                        }
//                                    }

//                                    Assert.IsNotNull(graphicsTemplates);
//                                    Assert.IsNotNull(soundTemplates);

//                                    Assert.AreEqual(7, graphicsTemplates.Count);
//                                    Assert.AreEqual(1, soundTemplates.Count);
//                                });
//        }

//        [Test]
//        public void LoadInvalidPreferenceTemplatesMissingRequiredElement()
//        {
//            CrossThreadTestRunner runner = new CrossThreadTestRunner();
//            runner.RunInSTA(() =>
//                                {
//                                    PreferenceTemplateFactory.Current = new WPFPreferenceTemplateFactory();

//                                    MockDirectory gameDirectory =
//                                        ((MockDirectory)
//                                         MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory)).
//                                            AddDirectory("games").AddDirectory("console");

//                                    const string InvalidPreferencesXml =
//                                        @"<?xml version=""1.0"" encoding=""utf-8""?>
//<mgdf:preferencetemplates  xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance""
//	    	    xmlns:mgdf=""http://www.mgdf.org/2007/preferenceTemplates"">
//  <category name=""sound"">
//   <preference name=""volume"" type=""slider"" label=""Volume"">
//      <selector>
//        <slider>
//          <minvalue>0</minvalue>
//          <maxvalue>100</maxvalue>
//          <increment>10</increment>
//          <multiplier>0.01</multiplier>
//        </slider>
//      </selector>
//    </preference>
//  </category>
//</mgdf:preferencetemplates>";

//                                    gameDirectory.AddFile("preferenceTemplates.xml", InvalidPreferencesXml);
//                                    IPreferenceTemplates prefTemplates =
//                                        ModelFactory.Current.CreatePreferenceTemplates(
//                                            Path.Combine(EnvironmentSettings.Current.AppDirectory,
//                                                         "games\\console\\preferenceTemplates.xml"));
//                                    Assert.IsFalse(prefTemplates.IsValid);
//                                    Assert.AreEqual(1, prefTemplates.ErrorCollection.Count);
//                                });
//        }

//        [Test]
//        public void LoadInvalidPreferenceTemplatesInvalidXml()
//        {
//                        CrossThreadTestRunner runner = new CrossThreadTestRunner();
//            runner.RunInSTA(() =>
//                                {

//                                    PreferenceTemplateFactory.Current = new WPFPreferenceTemplateFactory();

//                                    MockDirectory gameDirectory =
//                                        ((MockDirectory)
//                                         MockFileSystem.GetDirectory(EnvironmentSettings.Current.AppDirectory)).
//                                            AddDirectory("games").AddDirectory("console");

//                                    const string InvalidPreferencesXml =
//                                        @"<?xml version=""1.0"" encoding=""utf-8""?>
//<mgdf:preferencetemplates  xmlns:xsi=""http://www.w3.org/2001/XMLSchema-instance""
//	    	    xmlns:mgdf=""http://www.mgdf.org/2007/preferenceTemplates"">
//  <category name=""sound"">
//   <preference name=""volume"" type=""slider"" label=""Volume"">
//      <selector>
//        <slider>
//          <minvalue>0</minvalue>
//          <maxvalue>100</maxvalue>
//          <increment>10</increment>
//          <multiplier>0.01</multiplier>
//          <shownumber>false</shownumber>
//        </slider>
//      </selector>
//    </preference>
//</mgdf:preferencetemplates>";

//                                    gameDirectory.AddFile("preferenceTemplates.xml", InvalidPreferencesXml);
//                                    IPreferenceTemplates prefTemplates =
//                                        ModelFactory.Current.CreatePreferenceTemplates(
//                                            Path.Combine(EnvironmentSettings.Current.AppDirectory,
//                                                         "games\\console\\preferenceTemplates.xml"));
//                                    Assert.IsFalse(prefTemplates.IsValid);
//                                    Assert.AreEqual(1, prefTemplates.ErrorCollection.Count);
//                                });
//        }
    }
}
