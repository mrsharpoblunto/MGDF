/*
 * Copyright 1999-2001,2004 The Apache Software Foundation.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * $Id: XMLDOMProcessingInstruction.cpp 176026 2004-09-08 13:57:07Z peiyongz $
 */

#include "stdafx.h"
#include "xml4com.h"
#include "XMLDOMProcessingInstruction.h"

// IXMLDOMProcessingInstruction methods
STDMETHODIMP CXMLDOMProcessingInstruction::get_target(BSTR  *pVal)
{
	ATLTRACE(_T("CXMLDOMProcessingInstruction::get_target\n"));

	if (NULL == pVal)
		return E_POINTER;

	*pVal = NULL;

	try
	{
		*pVal = SysAllocString(processingInstruction->getTarget());
	}
	catch(DOMException& ex)
	{
		return MakeHRESULT(ex);
	}

	catch(...)
	{
		return E_FAIL;
	}

	return S_OK;
}

STDMETHODIMP CXMLDOMProcessingInstruction::get_data(BSTR  *pVal)
{
	ATLTRACE(_T("CXMLDOMProcessingInstruction::get_data\n"));

	if (NULL == pVal)
		return E_POINTER;

	*pVal = NULL;

	try
	{
		*pVal = SysAllocString(processingInstruction->getData());
	}
	catch(DOMException& ex)
	{
		return MakeHRESULT(ex);
	}
	catch(...)
	{
		return E_FAIL;
	}

	return S_OK;

}

STDMETHODIMP CXMLDOMProcessingInstruction::put_data(BSTR newVal)
{
	ATLTRACE(_T("CXMLDOMProcessingInstruction::put_data\n"));

	try
	{
		processingInstruction->setData(newVal);
	}
	catch(DOMException& ex)
	{
		return MakeHRESULT(ex);
	}
	catch(...)
	{
		return E_FAIL;
	}
	
	return S_OK;
}
