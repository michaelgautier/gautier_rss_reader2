#ifndef __gautier_graphics_InteractiveRegion__
#define __gautier_graphics_InteractiveRegion__
#include <dlib/geometry.h>

#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

/*
	Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.
*/
namespace gautier {
        namespace graphics {
class InteractiveRegion {
        public:
        InteractiveRegion();

	using InteractionCallBackType = void(*)(int lineNumber);        
        
        void SetInteractiveCallBack(InteractionCallBackType);
        
        void SetDimensions(double const& left, double const& top, double const& right, double const& bottom);
        void SetFont(ALLEGRO_FONT* font, int const& fontBoxX, int const& fontBoxY, int const& fontBoxW, int const& fontBoxH);

        void SetLineColorRGB(double const& r, double const& g, double const& b, double const& a);
        void SetLinePointSize(double const& size);
        void SetFillColorRGB(double const& r, double const& g, double const& b, double const& a);
        void SetTextColorRGB(double const& r, double const& g, double const& b, double const& a);

        void SetMouseChange(int const& mouseDirection, dlib::dpoint const& mousePosition);
        void SetMouseClick(bool const& leftButton, bool const& isDown, bool const& isUp, int const& mouseDirection, dlib::dpoint const& mousePosition);

        void SetData(std::vector<std::string>& data);
        void SetIsMultiline(bool const& isMultiline);

        bool GetIsVisualsSet();
        bool GetIsDataSet();

        double Left();
        double Top();
        double Right();
        double Bottom();

        void RenderSelf();

        private:
        void UpdateScrollTrack(const double& top);
        void RenderText();

        InteractionCallBackType 
                _InteractionCallBack;
        
        dlib::drectangle 
                _RegionDimensions, _ScrollDimensionsV;

        dlib::dpoint 
                _MousePosition, _MousePositionLast;

        ALLEGRO_COLOR 
                _LineColor, _FillColor, _TextColor, 
                _ScrollBarFillColor, _ScrollBarTrackColor, _ScrollBarLineColor;

        ALLEGRO_FONT const* 
                _Font;
        
        int 
                _FontBoxX, _FontBoxY, _FontBoxW, _FontBoxH,
                _VisibleTextIndexStart;

        double 
                _LinePointSize, _ScrollTrackYTop, _ScrollTrackYBottom,
                _ScrollY, _ScrollYText;
        int 
                _MouseDirection;
                
        bool 
                _IsVisualsSet, _IsDataSet, _IsMultiline;

        std::vector<std::string> 
                _Data;
};
        }
}
#endif
