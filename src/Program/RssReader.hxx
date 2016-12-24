#ifndef __gautier_program_RssReader__
#define __gautier_program_RssReader__

#include <dlib/geometry.h>
#include <PrimaryDisplaySurfaceWindow.hxx>
#include <InteractionState.hxx>
#include <InteractiveRegion.hxx>

#include <gautier_rss_model.hxx>

/*
	Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.
*/
namespace gautier {
        namespace program {
class RssReader {
	public:
	RssReader();
	void Start();
        void ProcessUpdates(const gautier::graphics::InteractionState&);

        void CallBackFeeds(int lineNumber);
        void CallBackFeedHeadlines(int lineNumber);
        void CallBackFeedDetails(int lineNumber);
	
	private:
        void InitializeData();
        void BuildVisualModel(const gautier::graphics::InteractionState&);
        void ProcessInteractions(const gautier::graphics::InteractionState&);
        void ProcessData();
        void UpdateVisualOutput();
        void LoadFont();
        
        bool
                _DataInitialized = false;

        gautier::graphics::InteractionState 
                _InteractionState;

        std::vector<std::string> 
                _Feeds, _FeedsUrls,
                _FeedHeadlines, _FeedHeadlineUrls,
                _FeedDetails;

        ALLEGRO_FONT* 
                _Font = nullptr;

        int 
                _FontBoxX, _FontBoxY, _FontBoxW, _FontBoxH;

        std::map<std::string, gautier::rss_model::unit_type_rss_source> 
                _rss_feed_sources;

        std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>> 
                _rss_feed_items;

        gautier::rss_model 
                _RssDataModel;

        std::string 
                _CurrentFeedSourceName;
};
        }
}
#endif
