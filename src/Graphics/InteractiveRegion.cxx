#include <InteractiveRegion.hxx>

static double 
        _ScrollBarWidth = 40, _ScrollYOffset = 24;
        
static dlib::drectangle _EmptyRect = dlib::drectangle(0,0,0,0);

gautier::graphics::InteractiveRegion::InteractiveRegion() {
        _IsVisualsSet = false;
        _IsDataSet = false;
        _IsMultiline = false;
        _ScrollY = 0;
        _ScrollYText = 0;
        _RegionDimensions = _EmptyRect;
        _ScrollDimensionsV = _EmptyRect;

        return;
}
void gautier::graphics::InteractiveRegion::SetDimensions(double const& left, double const& top, double const& right, double const& bottom) {
        UpdateScrollTrack(top);
        
        _RegionDimensions = dlib::drectangle(left, top, right, bottom);

        double const
              SLeft = right-_ScrollBarWidth;

        _ScrollDimensionsV = dlib::drectangle(SLeft, top, right, bottom);

        
        return;
}
void gautier::graphics::InteractiveRegion::UpdateScrollTrack(const double& top) {
        double 
                DTop = std::abs(_RegionDimensions.top()-top);

        if(_RegionDimensions.left() == _EmptyRect.left() && _RegionDimensions.top() == _EmptyRect.top() && _RegionDimensions.right() == _EmptyRect.right() && _RegionDimensions.bottom() == _EmptyRect.bottom()) {
                _ScrollTrackYTop = top;
                _ScrollTrackYBottom = _ScrollTrackYTop+(_ScrollYOffset*2);
        }
        else if(DTop != 0) {
                DTop  *= (top < _RegionDimensions.top() ? -1 : 1);

                _ScrollTrackYTop = _ScrollTrackYTop+DTop;
                _ScrollTrackYBottom = _ScrollTrackYTop+(_ScrollYOffset*2);
        }

        return;
}
void gautier::graphics::InteractiveRegion::SetFont(ALLEGRO_FONT* font, int const& fontBoxX, int const& fontBoxY, int const& fontBoxW, int const& fontBoxH) {
        if(!font) {
                //std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
                //std::cout << " font not initialized\r\n";
        }
        
        _Font = font;
        _FontBoxX = fontBoxX;
        _FontBoxY = fontBoxY;
        _FontBoxW = fontBoxW;
        _FontBoxH = fontBoxH;

        return;
}
void gautier::graphics::InteractiveRegion::SetIsMultiline(bool const& isMultiline) {
        _IsMultiline = isMultiline;

        return;
}
void gautier::graphics::InteractiveRegion::SetLineColorRGB(double const& r, double const& g, double const& b, double const& a) {
        _LineColor = al_map_rgba(r, g, b, a);

        return;
}
void gautier::graphics::InteractiveRegion::SetLinePointSize(double const& size) {
        _LinePointSize = size;

        return;
}
void gautier::graphics::InteractiveRegion::SetFillColorRGB(double const& r, double const& g, double const& b, double const& a) {
        _FillColor = al_map_rgba(r, g, b, a);

        _IsVisualsSet = true;

        _ScrollBarFillColor = al_map_rgb(233, 198, 175); 
        _ScrollBarTrackColor = al_map_rgb(213, 255, 217);
        _ScrollBarLineColor = al_map_rgb(124, 145, 111);

        return;
}
void gautier::graphics::InteractiveRegion::SetTextColorRGB(double const& r, double const& g, double const& b, double const& a) {
        _TextColor = al_map_rgba(r, g, b, a);

        return;
}
void gautier::graphics::InteractiveRegion::SetInteractiveCallBack(InteractionCallBackType callback) {
        _InteractionCallBack = callback;

        return;
}
void gautier::graphics::InteractiveRegion::SetMouseChange(int const& mouseDirection, dlib::dpoint const& mousePosition) {
        _MouseDirection = mouseDirection;
	_MousePositionLast = _MousePosition;
        _MousePosition = mousePosition;

        if(mouseDirection != 0 && _RegionDimensions.contains(mousePosition)) {
                double const 
                        STop = _ScrollDimensionsV.top(),
                        SBottom = _ScrollDimensionsV.bottom(),
                        SOffset = (_ScrollYOffset),
                        SIncrement = (mouseDirection*_FontBoxH);

                double ScrollY = _ScrollY + SIncrement;

                _ScrollYText += SIncrement;

                /*Visually the scroll track has to stay on screen.*/
                if((ScrollY-SOffset) <= STop) {
                        _ScrollYText = 0;
                        ScrollY = STop+SOffset;
                }
                else if ((ScrollY+SOffset) >= SBottom) {
                        ScrollY = SBottom-SOffset;
                }

                const double WindowItemCount = ((SBottom-STop)/_FontBoxH);
                const double WindowItemCountOffset = (WindowItemCount*_FontBoxH);
                const double ScrollYTextMax = _FontBoxH*_Data.size();

                if((_ScrollYText+WindowItemCountOffset) >= ScrollYTextMax) {
                        _ScrollYText = ScrollYTextMax-WindowItemCountOffset;
                }

                _ScrollY = ScrollY;
                
                _ScrollTrackYTop = _ScrollY-_ScrollYOffset;
                _ScrollTrackYBottom = _ScrollY+_ScrollYOffset;
        }

        return;
}
void gautier::graphics::InteractiveRegion::SetMouseClick(bool const& leftButton, bool const& isDown, bool const& isUp, int const& mouseDirection, dlib::dpoint const& mousePosition) {
        const bool IsMouseInRegion = _RegionDimensions.contains(mousePosition);
        if (IsMouseInRegion) {
                //std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
                //std::cout << "SetMouseClick: " << leftButton << " " << isDown << " " << mouseDirection << " " << mousePosition << "\r\n";

                const bool IsMouseInVScrollLane = _ScrollDimensionsV.contains(mousePosition);
                const bool IsLeftMouseDown = (leftButton && isDown);
                const bool IsScrollWheelTurning = mouseDirection != 0;
                const bool IsLeftMouseUp = (leftButton && isUp);

                if(IsMouseInVScrollLane && IsLeftMouseDown) {
                        double const ScrollY = _MousePosition.y();
                        int MouseDirection = (ScrollY == _ScrollY ? 0 : 1) * (ScrollY < _ScrollY ? -1 : 1);

                        //std::cout << "scroll click " << MouseDirection << "\r\n";

                        SetMouseChange(MouseDirection, mousePosition);
                }
                else if (IsScrollWheelTurning) {
                        //std::cout << "scroll line " << mouseDirection << "\r\n";

                        SetMouseChange(mouseDirection, mousePosition);
                }
                else if (IsLeftMouseUp && !IsMouseInVScrollLane) {
                        //std::cout << "Left Mouse Click \r\n";
                        
                        const double MouseY = (mousePosition.y() + (_VisibleTextIndexStart * _FontBoxH));
                        
                        int TextIndex = 0;
                        
                        double TextTopY = 0;
                        
                        //std::cout << "VisibleTextIndexStart " << _VisibleTextIndexStart << "\r\n";
                        
                        for(auto const& v : _Data) {
		                if(TextIndex < _VisibleTextIndexStart) {
                                        TextIndex++;

		                        continue;
		                }

                                //std::cout << "Looking at TextIndex " << TextIndex << "\r\n";

                                const double TextBottomY = (TextIndex+1) * _FontBoxH;

                                if(MouseY > TextTopY && MouseY < TextBottomY) {
                                        //std::cout << "Mouse Left Clicked on " << v << " line " << TextIndex << "\r\n";
                                        
                                        _InteractionCallBack(TextIndex);

                                        break;
                                }

                                TextTopY = TextBottomY;
                                TextIndex++;
                        }
                }
        }

        return;
}
void gautier::graphics::InteractiveRegion::SetData(std::vector<std::string>& data) {
        //std::cout << "setting data to " << data.size() << " items \r\n";

        _Data = data;

        _IsDataSet = true;

        return;
}
bool gautier::graphics::InteractiveRegion::GetIsDataSet() {
        return _IsDataSet;
}
void gautier::graphics::InteractiveRegion::RenderText() {
	const double VLeft = _RegionDimensions.left(),
	        VTop = _RegionDimensions.top(),
	        VRight = _RegionDimensions.right(),
	        VBottom = _RegionDimensions.bottom();

        const double 
                Width = VRight-VLeft,
                WidthWithMargin = (Width-_ScrollBarWidth);

        double  
                TextTop = VTop,
                LineTextHeight = 0.0;

	std::string LineText;

	al_set_clipping_rectangle(VLeft, VTop, WidthWithMargin, VBottom);

        if(_IsMultiline) {
                if(!_Data.empty()) {
                        const char* TextData = _Data.front().data();

                        al_draw_multiline_text(_Font, _TextColor, VLeft, VTop, WidthWithMargin, _FontBoxH, ALLEGRO_ALIGN_LEFT, TextData);
                }
        }
        else {
                int TextIndex = 0;

                _VisibleTextIndexStart = -1;

                const double 
                        SpaceBetweenCharacters = 5;

                const double  
                        CharacterLimitPad = (_ScrollBarWidth / _FontBoxW),
                        CharacterLimit = Width / (_FontBoxW + SpaceBetweenCharacters);

                //std::cout << "Width " << Width << " Font Box W " << _FontBoxW << " Right " << VRight << " Character Limit " << CharacterLimit << " Character Limit Pad " << CharacterLimitPad << "\r\n";

	        for(auto const& v : _Data) {
	                double TextH = _FontBoxH * TextIndex;

	                if(TextH >= _ScrollYText) {
	                        //std::cout << "text size " << v.size() << "\r\n";
	                        if(v.size() > (CharacterLimit-CharacterLimitPad)) {
		                        LineText = v.substr(0, CharacterLimit-CharacterLimitPad) + " ... ";
	                        }
	                        else {
	                                LineText = v;
	                        }

		                LineTextHeight = TextTop + (_FontBoxH);

		                if(LineTextHeight < VBottom) {
		                        if(_Font) {
			                        al_draw_text(_Font, _TextColor, VLeft, TextTop, ALLEGRO_ALIGN_LEFT, LineText.data());

		                                if(_VisibleTextIndexStart == -1) {
		                                        _VisibleTextIndexStart = TextIndex;
		                                }
			                }
			                else {
                                                //std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
			                        //std::cout << "font invalid\r\n";
			                        break;
			                }
		                }
		                else {
		                        break;
		                }

		                TextTop += _FontBoxH;		        
                        }

		        TextIndex++;
	        }
	}

	al_reset_clipping_rectangle();

	return;
}
void gautier::graphics::InteractiveRegion::RenderSelf() {
        double const 
                VLeft = _RegionDimensions.left(), 
                VTop = _RegionDimensions.top(), 
                VRight = _RegionDimensions.right(), 
                VBottom = _RegionDimensions.bottom();

	al_draw_filled_rectangle(VLeft, VTop, VRight, VBottom, _FillColor);
	al_draw_rectangle(VLeft, VTop, VRight, VBottom, _LineColor, _LinePointSize);

        double const
              SLeft = _ScrollDimensionsV.left(),
              SRight = _ScrollDimensionsV.right(),
              STop = _ScrollDimensionsV.top(),
              SBottom = _ScrollDimensionsV.bottom();

        //std::cout << "scroll lrtb " << SLeft << " " << STop << " " << SRight << " " << SBottom << "\r\n";

	al_draw_filled_rectangle(SLeft, STop, SRight, SBottom, _ScrollBarFillColor);
	al_draw_rectangle(SLeft, STop, SRight, SBottom, _ScrollBarLineColor, _LinePointSize);

        double const
              STTop = _ScrollTrackYTop,
              STBottom = _ScrollTrackYBottom;

	al_draw_filled_rectangle(SLeft, STTop, SRight, STBottom, _ScrollBarTrackColor);
	al_draw_rectangle(SLeft, STTop, SRight, STBottom, _ScrollBarLineColor, _LinePointSize);

        RenderText();

        return;
}
double gautier::graphics::InteractiveRegion::Left() {
        return _RegionDimensions.left();
}
double gautier::graphics::InteractiveRegion::Top() {
        return _RegionDimensions.top();
}
double gautier::graphics::InteractiveRegion::Right() {
        return _RegionDimensions.right();
}
double gautier::graphics::InteractiveRegion::Bottom() {
        return _RegionDimensions.bottom();
}
bool gautier::graphics::InteractiveRegion::GetIsVisualsSet() {
        return _IsVisualsSet;
}
