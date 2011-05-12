/***************************************************************************
 *   Copyright (C) 2004 - 2006 Paul D Turner & The CEGUI Development Team
 *
 *   Permission is hereby granted, free of charge, to any person obtaining
 *   a copy of this software and associated documentation files (the
 *   "Software"), to deal in the Software without restriction, including
 *   without limitation the rights to use, copy, modify, merge, publish,
 *   distribute, sublicense, and/or sell copies of the Software, and to
 *   permit persons to whom the Software is furnished to do so, subject to
 *   the following conditions:
 *
 *   The above copyright notice and this permission notice shall be
 *   included in all copies or substantial portions of the Software.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 *   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 *   OTHER DEALINGS IN THE SOFTWARE.
 ***************************************************************************/
/*************************************************************************
	
	The code in this file is taken from article 1.3 in the the book:
	Game Programming Gems from Charles River Media

*************************************************************************/

template <typename T> class Singleton
{
protected:
    static T* _inst;

public:
    Singleton( void )
    {
        assert( !_inst );
        _inst = static_cast<T*>(this);
    }
   ~Singleton( void )
        {  assert( _inst );  _inst = NULL;  }
    static T& Instance( void )
        {  assert( _inst );  return ( *_inst );  }
    static T* InstancePtr( void )
        {  return ( _inst );  }
};

#define DECLARE_SINGLETON(className) public:\
	static className &Instance(void);		\
	static className *InstancePtr(void)

#define DEFINE_SINGLETON(className)	template<> className *Singleton<##className##>::_inst	= NULL;	\
	className &##className##::Instance(void){ return Singleton<##className##>::Instance();}##className *##className##::InstancePtr(void) { return Singleton<##className##>::InstancePtr();}

