#ifndef __gautier_program_RssReader__
#define __gautier_program_RssReader__

#include <PrimaryDisplaySurfaceWindow.hxx>
#include <InteractionState.hxx>
#include <InteractiveRegion.hxx>
#include <InteractiveDisplay.hxx>
#include <RssDisplay.hxx>

#include <gautier_rss_model.hxx>

/*
	Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.
*/
namespace gautier {
        namespace program {
class RssReader {
	public:
	RssReader();
	~RssReader();

	void Start();
        void ProcessUpdates(const gautier::graphics::InteractionState& interactionState);
	
	private:
        ALLEGRO_FONT* 
                _Font = nullptr;

        int 
                _FontBoxX, _FontBoxY, _FontBoxW, _FontBoxH;

        gautier::graphics::InteractiveDisplay* 
                _InteractiveDisplay;

        gautier::graphics::InteractionState 
                _InteractionState;
};
        }
}
#endif
