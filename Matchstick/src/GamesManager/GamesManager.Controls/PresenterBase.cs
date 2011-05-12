using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using MGDF.GamesManager.Common;

namespace MGDF.GamesManager.Controls
{
    public class PresenterBase<T> : IPresenter where T : IView
    {
        private readonly T _view;

        public PresenterBase()
        {
            _view = ViewFactory.Current.CreateView<T>();
        }

        public PresenterBase(T view)
        {
            _view = view;
        }

        public T View
        {
            get { return _view; }
        }

        public void ShowView(IView owner)
        {
            _view.ShowView(owner);
        }

        public void ShowView()
        {
            ShowView(null);
        }

        public void CloseView()
        {
            _view.CloseView();
        }

        public static IMessage Message
        {
            get { return ViewFactory.Current.CreateView<IMessage>(); }
        }
    }
}
