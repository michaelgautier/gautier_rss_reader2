#include <RssReader.hxx>
/*
	Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.
*/
static gautier::program::RssReader* 
        _Self = nullptr;

static gautier::graphics::PrimaryDisplaySurfaceWindow 
        _GraphicsSurface;

std::vector<gautier::graphics::InteractiveDisplay*> 
        _IDisplays;

void UpdateDisplay(gautier::graphics::InteractionState interactionState);

gautier::program::RssReader::RssReader() {
        _Self = this;
        _IDisplays.reserve(200);

        return;
}
gautier::program::RssReader::~RssReader() {
        return;
}
void gautier::program::RssReader::Start() {
        gautier::program::RssDisplay* FirstDisplay = new gautier::program::RssDisplay();

         _IDisplays.push_back(FirstDisplay);

        _GraphicsSurface.Activate(UpdateDisplay);

        return;
}
void UpdateDisplay(gautier::graphics::InteractionState interactionState) {
        _Self->ProcessUpdates(interactionState);

	return;
}
void gautier::program::RssReader::ProcessUpdates(const gautier::graphics::InteractionState& interactionState) {
        bool IsVisualModelChanged = _GraphicsSurface.GetIsVisualModelChanged(_InteractionState, interactionState);

        if(IsVisualModelChanged) {
                if(!_Font) {
                        if(!_GraphicsSurface.GetIsFontLoaded()) {
                                _GraphicsSurface.SetFontParameters("NotoSans-Regular.ttf", 10);
                        }
                }

                gautier::graphics::InteractiveDisplay*
                        IDisplay = _IDisplays.front();

                if(IDisplay) {
                        IDisplay->BuildVisualModel(interactionState, _GraphicsSurface);

                        IDisplay->ProcessInteractions(interactionState, _IDisplays);

                        if(IDisplay != _IDisplays.front() && _IDisplays.front()) {
                                IDisplay = _IDisplays.front();
                        }

	                IDisplay->UpdateVisualOutput();
                }
	}

        _InteractionState = interactionState;

        return;
}

