#ifndef __gautier_graphics_InteractiveDisplay__
#define __gautier_graphics_InteractiveDisplay__

#include <InteractionState.hxx>
#include <PrimaryDisplaySurfaceWindow.hxx>
/*
	Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.
*/
namespace gautier {
        namespace graphics {
class InteractiveDisplay {
	public:
        virtual void BuildVisualModel(gautier::graphics::InteractionState const& interactionState, gautier::graphics::PrimaryDisplaySurfaceWindow& disp){};
        virtual void ProcessInteractions(gautier::graphics::InteractionState const& interactionState, std::vector<InteractiveDisplay*>& displays){};
        virtual void UpdateVisualOutput(){};
};
        }
}
#endif
