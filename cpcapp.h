/*
 * cpcapp.h
 *
 *  Created on: 15 lip 2016
 *      Author: wpk
 */

#ifndef CPCAPP_H_
#define CPCAPP_H_

#include <wx/wx.h>
#include "cpc.h"
class CtrlFrame;

class CPCApp : public wxApp {
public:
	CPCApp();
	virtual ~CPCApp();
	virtual bool OnInit();
	virtual bool ProcessIdle();
	CPC* emu;
	CtrlFrame* ctrlframe;
	uint32_t cycle;
};

class CtrlFrame: public wxFrame
{
public:
	CtrlFrame(const wxString& title, const wxPoint& pos, const wxSize& size, CPCApp& app);
private:
	void OnClose(wxCloseEvent& event);
	void OnOpenFloppy(wxCommandEvent& event);
	CPCApp& app;
	wxDECLARE_EVENT_TABLE();
};
enum {
	BUTTON_OPEN_FLOPPY = wxID_HIGHEST + 1
};

wxBEGIN_EVENT_TABLE(CtrlFrame, wxFrame)
EVT_BUTTON(BUTTON_OPEN_FLOPPY, CtrlFrame::OnOpenFloppy)
EVT_CLOSE(CtrlFrame::OnClose)
wxEND_EVENT_TABLE()

#endif /* CPCAPP_H_ */
