/*
 * cpcapp.cpp
 *
 *  Created on: 15 lip 2016
 *      Author: wpk
 */

#include "cpcapp.h"
#include "cpc.h"

wxIMPLEMENT_APP_NO_MAIN(CPCApp);

int main(int argc, char **argv) {
  // This initializes SDL
  SDL_Init(SDL_INIT_EVERYTHING);
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
  ctrlframe =
      new CtrlFrame("Zee80 CPC", wxPoint(50, 50), wxSize(450, 340), *this);
  ctrlframe->Show(true);
  return true;
}

bool CPCApp::ProcessIdle() {
  for (int i = 0; i < 10;
       i++) // TODO do it dynamically? Two event loops horror :/
    emu->run();
  if (cycle++ % 100000 == 0) {
    wxString x;
    x.Printf("Zee80 CPC load %.2f", emu->getLoad());
    ctrlframe->SetStatusText(x);
  }

  return true;
}
CPCApp::~CPCApp() {
  // TODO Auto-generated destructor stub
}

CtrlFrame::CtrlFrame(const wxString &title, const wxPoint &pos,
                     const wxSize &size, CPCApp &app)
    : wxFrame(NULL, wxID_ANY, title, pos, size), app(app) {
  wxPanel *panel = new wxPanel(this, -1);
  wxBoxSizer *vbox = new wxBoxSizer(wxVERTICAL);

  wxButton *reset = new wxButton(this, BTN_RESET, _T("Reset"),
                                 wxDefaultPosition, wxDefaultSize, 0);
  wxButton *openfloppy = new wxButton(this, BTN_OPEN_FLOPPY, _T("Open Floppy"),
                                      wxDefaultPosition, wxDefaultSize, 0);
  vbox->Add(reset, 0, wxEXPAND);
  vbox->Add(openfloppy, 0, wxEXPAND);
  panel->SetSizer(vbox);

  CreateStatusBar();
  Centre();
}
void CtrlFrame::OnClose(wxCloseEvent &event) {
  SDL_Quit();
  app.ExitMainLoop();
}

void CtrlFrame::OnOpenFloppy(wxCommandEvent &event) {
  wxFileDialog openFileDialog(this, _("Open DSK file"), "", "",
                              "Disk images (*.dsk)|*.dsk",
                              wxFD_OPEN | wxFD_FILE_MUST_EXIST);
  if (openFileDialog.ShowModal() == wxID_CANCEL)
    return;
  app.emu->LoadFloppy(openFileDialog.GetPath().ToStdString());
}

void CtrlFrame::OnReset(wxCommandEvent &event) { app.emu->reset(); }
