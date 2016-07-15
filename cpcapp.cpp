/*
 * cpcapp.cpp
 *
 *  Created on: 15 lip 2016
 *      Author: wpk
 */

#include "cpcapp.h"
#include "cpc.h"

wxIMPLEMENT_APP_NO_MAIN(CPCApp);

int main(int argc, char** argv)
{
	// This initializes SDL
	SDL_Init( SDL_INIT_EVERYTHING );
	return wxEntry(argc, argv);
}
CPCApp::CPCApp() {
	// TODO Auto-generated constructor stub

}
bool CPCApp::OnInit() {
	SetExitOnFrameDelete(false);
	emu = new CPC();
	emu->parse_opts(argc, argv);
	emu->initialize();
	CtrlFrame *frame = new CtrlFrame("Zee80 CPC", wxPoint(50, 50), wxSize(450, 340), *this);
	frame->Show( true );
	return true;
}

bool CPCApp::ProcessIdle() {
	emu->run();
	return true;
}
CPCApp::~CPCApp() {
	// TODO Auto-generated destructor stub
}

CtrlFrame::CtrlFrame(const wxString& title, const wxPoint& pos, const wxSize& size, CPCApp& app)
: wxFrame(NULL, wxID_ANY, title, pos, size), app(app)
{
	wxButton* OpenFloppy = new wxButton(this, BUTTON_OPEN_FLOPPY, _T("Open Floppy"),
					    wxDefaultPosition, wxDefaultSize, 0);
	CreateStatusBar();
	SetStatusText("Zee80 CPC ");
}
void CtrlFrame::OnClose(wxCloseEvent& event)
{
	SDL_Quit();
	app.ExitMainLoop();
}

void CtrlFrame::OnOpenFloppy(wxCommandEvent& event)
{
	std::cout << "OnOpenFloppy";
	wxFileDialog
	openFileDialog(this, _("Open DSK file"), "", "",
		       "Disk images (*.dsk)|*.dsk", wxFD_OPEN|wxFD_FILE_MUST_EXIST);
	if (openFileDialog.ShowModal() == wxID_CANCEL)
		return;
	app.emu->fdc.load(openFileDialog.GetPath().ToStdString());
}
