/*
 Copyright (C) 2010-2012 Kristian Duske
 
 This file is part of TrenchBroom.
 
 TrenchBroom is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 TrenchBroom is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with TrenchBroom.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "TrenchBroomApp.h"

#include <wx/config.h>
#include <wx/docview.h>

#include "Utility/DocManager.h"
#include "View/MenuCommandIds.h"

#include <clocale>

IMPLEMENT_APP(TrenchBroomApp)

BEGIN_EVENT_TABLE(TrenchBroomApp, AbstractApp)
EVT_MENU(wxID_EXIT, TrenchBroomApp::OnFileExit)
EVT_UPDATE_UI(wxID_UNDO, TrenchBroomApp::OnUpdateMenuItem)
EVT_UPDATE_UI(wxID_REDO, TrenchBroomApp::OnUpdateMenuItem)
EVT_UPDATE_UI_RANGE(TrenchBroom::View::MenuCommandIds::tbID_MENU_LOWEST, TrenchBroom::View::MenuCommandIds::tbID_MENU_HIGHEST, TrenchBroomApp::OnUpdateMenuItem)
END_EVENT_TABLE()

wxMenu* TrenchBroomApp::CreateFileMenu() {
    wxMenu* fileMenu = AbstractApp::CreateFileMenu();

    // these won't show up in the app menu if we don't add them here
    fileMenu->Append(wxID_ABOUT, wxT("About"));
    fileMenu->Append(wxID_PREFERENCES, wxT("Preferences...\tCtrl-,"));
    fileMenu->Append(wxID_EXIT, wxT("Exit"));
    
    return fileMenu;
}

bool TrenchBroomApp::OnInit() {
    // set the locale to US so that we can parse floats property
    std::setlocale(LC_ALL, "en_US");
    
    if (AbstractApp::OnInit()) {
        SetExitOnFrameDelete(false);

        m_docManager->SetUseSDI(false);
        m_docManager->FileHistoryLoad(*wxConfig::Get());
        
        wxMenuBar* menuBar = CreateMenuBar(this);
        wxMenuBar::MacSetCommonMenuBar(menuBar);
        
        return true;
    }
    
    return false;
}


void TrenchBroomApp::OnFileExit(wxCommandEvent& event) {
    Exit();
}

void TrenchBroomApp::OnUpdateMenuItem(wxUpdateUIEvent& event) {
    event.Enable(false); // disable everything (except maybe help?)
}
