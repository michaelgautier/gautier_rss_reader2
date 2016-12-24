#ifndef __gautier_graphics_PrimaryDisplaySurfaceWindow__
#define __gautier_graphics_PrimaryDisplaySurfaceWindow__

#include <allegro5/allegro.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>

#include <dlib/geometry.h>

#include <InteractionState.hxx>
/*
	Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.
*/
namespace gautier {
        namespace graphics {
class PrimaryDisplaySurfaceWindow {
	public:
	using InteractionCallBackType = void(*)(InteractionState);
	PrimaryDisplaySurfaceWindow();
	~PrimaryDisplaySurfaceWindow();
	void Activate(InteractionCallBackType);
        void StartRenderGraphics();
        void EndRenderGraphics();
        void GetScreenDpi(double& screenDpi);

        bool GetIsVisualModelChanged(InteractionState const& old, InteractionState const& now);
        bool GetIsFontLoaded();
        void SetFontParameters(const char* fontPath, double const& fontSize);
        void GetFont(ALLEGRO_FONT*& font, int& fontBoxX, int& fontBoxY, int& fontBoxW, int& fontBoxH);

	private:
	void Initialize();
	void Release();
	void UpdateDisplay(gautier::graphics::PrimaryDisplaySurfaceWindow& primaryDisplaySurface);

        void LoadFont();
        void MeasureLineHeight(const char* str);

        ALLEGRO_DISPLAY* 
                _WinCtx;

        ALLEGRO_MONITOR_INFO 
                _WinScreenInfo;

        ALLEGRO_EVENT_SOURCE* 
                _WinMsgEvtSrc;

        ALLEGRO_EVENT_SOURCE* 
                _MouseEvtSrc;

        ALLEGRO_EVENT_QUEUE* 
                _WinMsgEvtQueue;

        ALLEGRO_EVENT 
                _winmsg_event;

        ALLEGRO_FONT* _Font;

        gautier::graphics::InteractionState 
                _InteractionState, _InteractionStateLast;

        gautier::graphics::PrimaryDisplaySurfaceWindow::InteractionCallBackType 
                _InteractionCallBack;

        bool 
                _IsAllegroInitialized, _IsAllegroUnInitialized,
                _IsScreenDPICached;
                
        int 
                _FontBoxX, _FontBoxW, _FontBoxY, _FontBoxH;

        double _FontSize, _ScreenDPI;
                
        const char* _FontPath;

};
        }
}
#endif
