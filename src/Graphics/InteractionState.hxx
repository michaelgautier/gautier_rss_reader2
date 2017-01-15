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
inline bool operator ==(const InteractionState& lhs, const InteractionState& rhs) {
        bool BoolsEqual = (lhs.IsWindowOpen == rhs.IsWindowOpen && lhs.IsWindowResized == rhs.IsWindowResized && lhs.IsMouseDown == rhs.IsMouseDown && lhs.IsMouseUp == rhs.IsMouseUp && lhs.IsVisualModelChanged == rhs.IsVisualModelChanged);
        bool IntsEqual = (lhs.MonitorWidth == rhs.MonitorWidth && lhs.MonitorHeight == rhs.MonitorHeight && lhs.WindowWidth == rhs.WindowWidth && lhs.WindowHeight == rhs.WindowHeight && lhs.MouseButton == rhs.MouseButton && lhs.MouseDirection == rhs.MouseDirection);
        bool WindowDimensionsEqual = (lhs.WindowDimensions.left() == rhs.WindowDimensions.left() && lhs.WindowDimensions.top() == rhs.WindowDimensions.top() && lhs.WindowDimensions.right() == rhs.WindowDimensions.right() && lhs.WindowDimensions.bottom() == rhs.WindowDimensions.bottom());
        bool MousePositionEqual = (lhs.MousePosition.x() == rhs.MousePosition.x() && lhs.MousePosition.y() == rhs.MousePosition.y());
        bool WindowPositionEqual = (lhs.WindowPosition.x() == rhs.WindowPosition.x() && lhs.WindowPosition.y() == rhs.WindowPosition.y());

        return (BoolsEqual && IntsEqual && WindowDimensionsEqual && MousePositionEqual && WindowPositionEqual);
}
inline bool operator !=(const InteractionState& lhs, const InteractionState& rhs) {
        return !(lhs == rhs);
}

        }
}
#endif
