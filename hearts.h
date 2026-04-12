#ifndef HEARTS_INC
#define HEARTS_INC

#include <wx/wx.h>
#include <wx/config.h>

class HeartsApp : public wxApp
{
public:
    virtual bool OnInit();
};

wxDECLARE_APP(HeartsApp);

#endif
