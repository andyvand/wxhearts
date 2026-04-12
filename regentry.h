/***************************************************************************/
/**                  Microsoft Windows                                    **/
/**            Copyright(c) Microsoft Corp., 1991-1994                    **/
/***************************************************************************/

/* regentry.h -- wxWidgets port using wxConfig */

#ifndef REGENTRY_INC
#define REGENTRY_INC

#include <wx/config.h>
#include <wx/string.h>

class RegEntry
{
public:
    RegEntry(const wxString& subKey)
        : m_path(subKey)
    {
        m_config = wxConfigBase::Get();
    }

    ~RegEntry() {}

    long SetValue(const wxString& name, const wxString& string)
    {
        if (m_config)
            m_config->Write(m_path + wxT("/") + name, string);
        return 0;
    }

    long SetValue(const wxString& name, long number)
    {
        if (m_config)
            m_config->Write(m_path + wxT("/") + name, number);
        return 0;
    }

    wxString GetString(const wxString& name, const wxString& defaultVal = wxEmptyString)
    {
        wxString result;
        if (m_config)
            m_config->Read(m_path + wxT("/") + name, &result, defaultVal);
        return result;
    }

    /* Overload matching original signature: writes into buffer, returns pointer */
    wxChar* GetString(const wxString& name, wxChar* buffer, size_t /*length*/)
    {
        wxString result = GetString(name);
        if (buffer) {
            wxStrcpy(buffer, result.c_str());
        }
        return buffer;
    }

    long GetNumber(const wxString& name, long defaultVal = 0)
    {
        long result = defaultVal;
        if (m_config)
            m_config->Read(m_path + wxT("/") + name, &result, defaultVal);
        return result;
    }

    long DeleteValue(const wxString& name)
    {
        if (m_config)
            m_config->DeleteEntry(m_path + wxT("/") + name);
        return 0;
    }

    void FlushKey()
    {
        if (m_config)
            m_config->Flush();
    }

private:
    wxString m_path;
    wxConfigBase* m_config;
};

#endif /* REGENTRY_INC */
