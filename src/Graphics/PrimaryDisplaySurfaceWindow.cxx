#include <string>
#include <iostream>
#include <vector>
#include <cmath>

#include <allegro5/allegro.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#include <PrimaryDisplaySurfaceWindow.hxx>
#include <InteractionState.hxx>
#include <dlib/geometry.h>

/*
	Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.
*/
static std::string 
        _DefaultWindowTitle = "Gautier Frame";

static int _EventIter = 0;
	
static constexpr double 
        _AvgPhysicalScreenSize = 13.667, _PrintPointSize = 72.0;

static double 
        _ScreenDpiLast = 96;
gautier::graphics::PrimaryDisplaySurfaceWindow::PrimaryDisplaySurfaceWindow() {
        Initialize();
        
        return;
}
gautier::graphics::PrimaryDisplaySurfaceWindow::~PrimaryDisplaySurfaceWindow() {
        Release();
        
        return;
}
void gautier::graphics::PrimaryDisplaySurfaceWindow::Initialize() {
        _InteractionState.WindowPosition = dlib::dpoint(0, 0);
        _InteractionState.WindowDimensions = dlib::drectangle(0, 0, 0, 0);

	_IsAllegroInitialized = al_init();

	if(_IsAllegroInitialized) {
		al_init_font_addon();
		al_init_ttf_addon();
	
		al_set_new_display_flags(ALLEGRO_MAXIMIZED | ALLEGRO_RESIZABLE | ALLEGRO_GENERATE_EXPOSE_EVENTS | ALLEGRO_OPENGL);
		al_set_new_display_option(ALLEGRO_RENDER_METHOD, true, ALLEGRO_SUGGEST);
		al_set_new_display_option(ALLEGRO_FLOAT_COLOR, true, ALLEGRO_SUGGEST);
		
		bool IsMonitorInfoAvailable = al_get_monitor_info(0, &_WinScreenInfo);

		if(IsMonitorInfoAvailable) {
			_InteractionState.MonitorWidth = _WinScreenInfo.x2-_WinScreenInfo.x1;
			_InteractionState.MonitorHeight = _WinScreenInfo.y2-_WinScreenInfo.y1;

			/*
				At the point I implement resolution change detection code, 
				can set _dpi_info_cached = false to allow the dpi to pickup;
			*/
			double ScreenDpi;
			GetScreenDpi(ScreenDpi);
			
			std::cout << "estimated dpi: " << ScreenDpi << "\r\n";

			_WinCtx = al_create_display(_InteractionState.MonitorWidth, _InteractionState.MonitorHeight);

                        if(_WinCtx) {
			        _InteractionState.WindowWidth = al_get_display_width(_WinCtx);		
			        _InteractionState.WindowHeight = al_get_display_height(_WinCtx);

                                _InteractionState.WindowDimensions = dlib::drectangle(0,0,_InteractionState.WindowWidth, _InteractionState.WindowHeight);

			        al_set_window_title(_WinCtx, _DefaultWindowTitle.data());
			        al_set_window_position(_WinCtx, _InteractionState.WindowPosition.x(), _InteractionState.WindowPosition.y());

			        _WinMsgEvtSrc = al_get_display_event_source(_WinCtx);
			        _WinMsgEvtQueue = al_create_event_queue();

			        al_register_event_source(_WinMsgEvtQueue, _WinMsgEvtSrc);

		                if(!al_is_mouse_installed()) {
			                al_install_mouse();
			                
			                _MouseEvtSrc = al_get_mouse_event_source();
		
			                al_register_event_source(_WinMsgEvtQueue, _MouseEvtSrc);
		                }
                        }
                        else {
                                std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
                                std::cout << "window could not be created.\r\n";
                        }
		}
		
		if(_WinCtx) {
			_InteractionState.IsWindowOpen = true;
			_InteractionState.IsVisualModelChanged = true;
			_InteractionStateLast = _InteractionState;
		}
	}

	return;
}
void gautier::graphics::PrimaryDisplaySurfaceWindow::Activate(InteractionCallBackType interactionCallBack) {
        _InteractionCallBack = interactionCallBack;
	while(_InteractionState.IsWindowOpen && _WinMsgEvtQueue) {
                _InteractionStateLast = _InteractionState;
	        //std::cout << "Event Iter " << _EventIter << "\r\n";
	        _EventIter++;

		ALLEGRO_EVENT_TYPE window_event_type = _winmsg_event.type;

	        //std::cout << "window event" << _winmsg_event.type << "\r\n";
		
		/*
		        Right before the display closes, you may see switch in, switch out, then close.
		*/
		switch(window_event_type) {
			case ALLEGRO_EVENT_DISPLAY_SWITCH_IN:
			        //std::cout << "Switching in display - The window is active\r\n";
			        break;
			case ALLEGRO_EVENT_DISPLAY_SWITCH_OUT:
			        //std::cout << "Switching out display - The window is inactive\r\n";
			        break;
			case ALLEGRO_EVENT_DISPLAY_CLOSE:
			        //std::cout << "window close\r\n";
				_InteractionState.IsWindowOpen = false;
			        break;
			case ALLEGRO_EVENT_DISPLAY_RESIZE:
			        //std::cout << "resize\r\n";
				al_acknowledge_resize(_WinCtx);
				
				_InteractionState.WindowWidth = _winmsg_event.display.width;
				_InteractionState.WindowHeight = _winmsg_event.display.height;
                                _InteractionState.WindowDimensions = dlib::drectangle(0,0,_InteractionState.WindowWidth, _InteractionState.WindowHeight);

				_InteractionState.IsWindowResized = true;	
			        //std::cout << _InteractionState.WindowWidth << "/" << _InteractionState.WindowHeight << "\r\n";
			        break;
	                case ALLEGRO_EVENT_MOUSE_BUTTON_UP:
	                        //std::cout << "evt mouse up\r\n";
	                        _InteractionState.IsMouseUp = true;
	                        _InteractionState.IsMouseDown = false;
		                break;
	                case ALLEGRO_EVENT_MOUSE_BUTTON_DOWN:
	                        //std::cout << "evt mouse up\r\n";
	                        _InteractionState.IsMouseUp = false;
	                        _InteractionState.IsMouseDown = true;
	                        break;
	                case ALLEGRO_EVENT_MOUSE_AXES:
	                        _InteractionState.MouseDirection = _winmsg_event.mouse.dz;
	                        //std::cout << "evt mouse direction " << _InteractionState.MouseDirection << "\r\n";
	                        break;
                }

                _InteractionState.MousePosition = dlib::dpoint(_winmsg_event.mouse.x, _winmsg_event.mouse.y);
                _InteractionState.MouseButton = _winmsg_event.mouse.button;

                //std::cout << "evt mouse direction " << _InteractionState.MouseDirection << "\r\n";

                if(_InteractionState.IsWindowOpen) {
                        if(_WinCtx) {
                                al_flush_event_queue(_WinMsgEvtQueue);

                                bool IsVisualModelChanged = GetIsVisualModelChanged(_InteractionStateLast, _InteractionState);

                                if(IsVisualModelChanged) {
                                        StartRenderGraphics();

                                        //std::cout << "visual callback: changed " << IsVisualModelChanged << "\r\n";

                                        interactionCallBack(_InteractionState);

                                        EndRenderGraphics();
                                }
	                }

                        _InteractionStateLast = _InteractionState;

                        al_wait_for_event_timed(_WinMsgEvtQueue, &_winmsg_event, 0.1);
                }
                else {
		        Release();
                }
	}
	
	return;
}
void gautier::graphics::PrimaryDisplaySurfaceWindow::Release() {
        if(!_IsAllegroUnInitialized) {
                if(_WinMsgEvtQueue) {
                        if(_MouseEvtSrc) {
	                        al_unregister_event_source(_WinMsgEvtQueue, _MouseEvtSrc);
	                }

                        if(_WinMsgEvtSrc) {
	                        al_unregister_event_source(_WinMsgEvtQueue, _WinMsgEvtSrc);
	                }

	                al_destroy_event_queue(_WinMsgEvtQueue);
	        }

                if(_WinCtx) {
	                al_destroy_display(_WinCtx);
	        }

	        _IsAllegroUnInitialized = true;
	        _IsAllegroInitialized = false;
	}
	
	return;
}
void gautier::graphics::PrimaryDisplaySurfaceWindow::GetScreenDpi(double& screenDpi) {
	double scrdpi = 0;
	/*
	 * 	Calculate true screen resolution, PPI  (see wikipedia  Pixel Density
	 * 
	 * 		diagonal resolution in pixels   =   square root of ( (screen_w)^2  +  (screen_h)^2  )
	 * 
	 * 		diag_res / diagonal screen size in inches (the average of (11, 12, 13.3, 14, 15.6, and 17) )
	 */		
	if(_IsScreenDPICached) {
		scrdpi = _ScreenDpiLast;
	}
	else {
		double diagres = std::hypot(_InteractionState.MonitorWidth, _InteractionState.MonitorHeight);
		double scrtmp = diagres/_AvgPhysicalScreenSize; 

		scrdpi = _ScreenDpiLast = scrtmp;

		_IsScreenDPICached = true;
	}
	
	screenDpi = scrdpi;
	
	return;
}
bool gautier::graphics::PrimaryDisplaySurfaceWindow::GetIsVisualModelChanged(gautier::graphics::InteractionState const& old, gautier::graphics::InteractionState const& now) {
        bool IsChanged = (now.IsVisualModelChanged || now.IsWindowResized || now.IsMouseDown || now.IsMouseUp);

        if(!IsChanged) {
                IsChanged = (old.WindowWidth != now.WindowWidth || old.WindowHeight != now.WindowHeight 
                 || old.MouseButton != now.MouseButton || old.MouseDirection != now.MouseDirection 
                 || old.WindowDimensions != now.WindowDimensions || old.MousePosition != now.MousePosition);
        }
        
        return IsChanged;
}
void gautier::graphics::PrimaryDisplaySurfaceWindow::StartRenderGraphics() {
        al_flip_display();
        al_clear_to_color(al_map_rgb(255, 255, 255));

        return;
}
void gautier::graphics::PrimaryDisplaySurfaceWindow::EndRenderGraphics() {
        al_flip_display();

        return;
}
bool gautier::graphics::PrimaryDisplaySurfaceWindow::GetIsFontLoaded() {
        return _Font != nullptr;
}
void gautier::graphics::PrimaryDisplaySurfaceWindow::SetFontParameters(const char* fontPath, double const& fontSize) {
        _FontPath = fontPath;
        _FontSize = fontSize;

        LoadFont();

        return;
}
void gautier::graphics::PrimaryDisplaySurfaceWindow::GetFont(ALLEGRO_FONT*& font, int& fontBoxX, int& fontBoxY, int& fontBoxW, int& fontBoxH) {
        font = _Font;
        fontBoxX = _FontBoxX;
        fontBoxY = _FontBoxY;
        fontBoxW = _FontBoxW;
        fontBoxH = _FontBoxH;

        return;
}
void gautier::graphics::PrimaryDisplaySurfaceWindow::LoadFont() {
        if(!_Font) {
                /*
                        Font scaling based on Paragraph #4 in the Article DPI and Device-Independent Pixels at: 
                        https://msdn.microsoft.com/en-us/library/windows/desktop/ff684173(v=vs.85).aspx
                */
                double ScreenDpi;

                GetScreenDpi(ScreenDpi);

                const double ScaledFontSizeD = (_FontSize/_PrintPointSize) * ScreenDpi;

                _Font = al_load_font(_FontPath, ScaledFontSizeD, 0);

                if(_Font) {
                        std::vector<char> 
                                TextToMeasure = {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','z','y','z','A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','Z','Y','Z','1','2','3','4','5','6','8','9','0','~','!','@','#','$','%','^','&','*','(',')','_','+','-','=','{','}','[',']','|','\\',':',';','<','>','?',',','.','/','\0'}; 

                        const char* 
                                SampleText = TextToMeasure.data();

                        MeasureLineHeight(SampleText);

                        if(TextToMeasure.size() > 0) {
                                int FontBoxX = 0, FontBoxY = 0, FontBoxW = 0, FontBoxH = 0;

                                std::vector<double>
                                        FontBoxWs;

                                double FontBoxWm = 0;

                                for(auto txt : TextToMeasure) {
                                        char Letter[] = {txt, '\0'};

                                        al_get_text_dimensions(_Font, Letter, &FontBoxX, &FontBoxY, &FontBoxW, &FontBoxH);

                                        FontBoxWm += FontBoxW;

                                        FontBoxWs.push_back(FontBoxW);
                                }

                                _FontBoxW = FontBoxWm/FontBoxWs.size();
                        }
                }
                else {
                        std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
                        std::cout << "could not load font\r\n";
                }
        }

        return;
}
void gautier::graphics::PrimaryDisplaySurfaceWindow::MeasureLineHeight(const char* str) {
        if(_Font) {
                int FontBoxX = 0, FontBoxY = 0, FontBoxW = 0, FontBoxH = 0;
                
                al_get_text_dimensions(_Font, str, &FontBoxX, &FontBoxY, &FontBoxW, &FontBoxH);

                _FontBoxX = FontBoxX; _FontBoxW = FontBoxW;
                _FontBoxY = FontBoxY; _FontBoxH = FontBoxH;
        }

        return;
}

