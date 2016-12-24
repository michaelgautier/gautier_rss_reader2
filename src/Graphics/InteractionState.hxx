#ifndef __gautier_graphics_InteractionState__
#define __gautier_graphics_InteractionState__

#include <dlib/geometry.h>
/*
	Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.
*/
namespace gautier {
        namespace graphics {
struct InteractionState {
	public:
        bool 
                IsWindowOpen, 
                IsWindowResized, 
                IsMouseDown, 
                IsMouseUp,
                IsVisualModelChanged;

        int 
                MonitorWidth,
                MonitorHeight,
                WindowWidth,
                WindowHeight,
                MouseButton,
                MouseDirection;

        dlib::drectangle 
                WindowDimensions;

        dlib::dpoint 
                MousePosition;

        dlib::dpoint 
                WindowPosition;
};
        }
}
#endif
