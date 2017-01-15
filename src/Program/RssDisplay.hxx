#ifndef __gautier_program_RssDisplay__
#define __gautier_program_RssDisplay__

#include <InteractionState.hxx>
#include <InteractiveRegion.hxx>
#include <InteractiveDisplay.hxx>
#include <PrimaryDisplaySurfaceWindow.hxx>
#include <gautier_rss_model.hxx>

/*
	Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.
*/
namespace gautier {
        namespace program {
class RssDisplay : public gautier::graphics::InteractiveDisplay {
	public:
	RssDisplay();
	~RssDisplay();
        void BuildVisualModel(gautier::graphics::InteractionState const& interactionState, gautier::graphics::PrimaryDisplaySurfaceWindow& disp);
        void ProcessInteractions(gautier::graphics::InteractionState const& interactionState, std::vector<InteractiveDisplay*>& displays);
        void UpdateVisualOutput();
        void CallBackFeedDetails(int lineNumber);
        void CallBackFeedHeadlines(int lineNumber);
        void CallBackFeeds(int lineNumber);

        private:
        void InitializeData();
        void ProcessData();

        ALLEGRO_FONT* 
                _Font = nullptr;

        bool
                _DataInitialized = false;

        int 
                _FontBoxX, _FontBoxY, _FontBoxW, _FontBoxH;

        gautier::graphics::InteractionState 
                _InteractionState;

        std::vector<std::string> 
                _Feeds, _FeedsUrls,
                _FeedHeadlines, _FeedHeadlineUrls,
                _FeedDetails;

        std::map<std::string, gautier::rss_model::unit_type_rss_source> 
                _rss_feed_sources;

        std::map<std::string, std::vector<gautier::rss_model::unit_type_rss_item>> 
                _rss_feed_items;

        gautier::rss_model 
                _RssDataModel;

        std::string 
                _CurrentFeedSourceName;

        static constexpr double 
                _RegionProportion = 4.0;

        std::vector<gautier::graphics::InteractiveRegion>
                _Regions;


};
        }
}
#endif
