using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.GameSource.Model;

namespace MGDF.GamesManager.GameSource.Tests.Mocks
{
    class MockGameSourceRepository: MockRepositoryBase
    {
        public MockGameSourceRepository() : base(new[]{
                                                          typeof(PendingDelete),
                                                          typeof (Game), 
                                                          typeof (Developer), 
                                                          typeof (User), 
                                                          typeof (UserGame), 
                                                          typeof (GameFragment),
                                                          typeof (GameVersion),
                                                          typeof(DefaultFileServerGameData),
                                                          typeof(CNonce)})
        {
        }
    }
}