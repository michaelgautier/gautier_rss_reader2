#include <RssDisplay.hxx>
/*
	Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.
*/
static gautier::program::RssDisplay* 
        _Self;
static constexpr double 
        _RegionProportion = 4.0;

gautier::program::RssDisplay::RssDisplay() {
        _Self = this;

        _Regions = {
               gautier::graphics::InteractiveRegion(), 
               gautier::graphics::InteractiveRegion(), 
               gautier::graphics::InteractiveRegion()
        };

        return;
}
gautier::program::RssDisplay::~RssDisplay() {
        _Self = nullptr;

        return;
}
void gautier::program::RssDisplay::BuildVisualModel(gautier::graphics::InteractionState const& interactionState, gautier::graphics::PrimaryDisplaySurfaceWindow& disp) {
        if(!_Font) {
                disp.GetFont(_Font, _FontBoxX, _FontBoxY, _FontBoxW, _FontBoxH);
        }

        dlib::drectangle WindowDimensions = interactionState.WindowDimensions;

        const double VLeft = WindowDimensions.left(), VTop = WindowDimensions.top(), 
        VRight = WindowDimensions.right(), VBottom = WindowDimensions.bottom();

        short RegionNumber = 0;

        //std::cout << "win l,t,r,b " << VLeft << " " << VTop << " " << VRight << " " << VBottom << "\r\n";

        for(auto& r : _Regions) {
                double VL0 = VLeft, VT0 = VTop, VR0 = VRight, VB0 = VBottom;
                double CFR, CFG, CFB, CFA = 255;
                double CLR, CLG, CLB, CLA = CFA;
                double CTR, CTG, CTB, CTA = CFA;

                const bool IsVisualsSet = r.GetIsVisualsSet();

                switch(RegionNumber) {
                        case 0:
                                VR0 = VR0/_RegionProportion;
                                CFR = 255; CFG = 153; CFB = 85;
                                CLR = 160; CLG = 44; CLB = 44;
                                CTR = 11; CTG = 40; CTB = 23;
                                break;
                        case 1:
	                        VL0 = _Regions[0].Right();
	                        VB0 = VB0/_RegionProportion;
                                CFR = 255; CFG = 221; CFB = 85;
                                CTR = 11; CTG = 40; CTB = 23;
                                break;
                        case 2:
	                        VL0 = _Regions[0].Right();
	                        VT0 = _Regions[1].Bottom();
                                CFR = 249; CFG = 249; CFB = 249;
                                CTR = 11; CTG = 40; CTB = 23;
                                break;
                        default:
                                //std::cout << __FILE__ " " << __func__ << " " << "(" << __LINE__ << ") ";
                                //std::cout << "region not mapped " << RegionNumber << "\r\n";
                                break;
                }

                r.SetDimensions(VL0, VT0, VR0, VB0);
                
                if(!IsVisualsSet) {
                        r.SetFont(_Font, _FontBoxX, _FontBoxY, _FontBoxW, _FontBoxH);                
                        r.SetLineColorRGB(CLR, CLG, CLB, CLA);
                        r.SetLinePointSize(1);
                        r.SetFillColorRGB(CFR, CFG, CFB, CFA);
                        r.SetTextColorRGB(CTR, CTG, CTB, CTA);
                }

                //std::cout << "region pass 1 l,t,r,b " << r.Left() << " " << r.Top() << " " << r.Right() << " " << r.Bottom() << "\r\n";

                RegionNumber++;
        }

        return;
}
void gautier::program::RssDisplay::ProcessInteractions(gautier::graphics::InteractionState const& interactionState, std::vector<InteractiveDisplay*>& displays) {
        int 
                MouseDirection = interactionState.MouseDirection,
                MouseButton = interactionState.MouseButton;

        const bool 
                IsMouseUp = (interactionState.IsMouseUp && _InteractionState.IsMouseUp == false), 
                IsMouseDown = interactionState.IsMouseDown,
                LeftMouseButton = (MouseButton == 1);

        dlib::dpoint 
                MousePosition = interactionState.MousePosition;

        for(auto& r : _Regions) {
                r.SetMouseClick(LeftMouseButton, IsMouseDown, IsMouseUp, MouseDirection, MousePosition);
        }

        ProcessData();

        return;
}
void FeedsCallBack(int lineNumber) {
        _Self->CallBackFeeds(lineNumber);

        return;
}
void FeedHeadlinesCallBack(int lineNumber) {
        _Self->CallBackFeedHeadlines(lineNumber);

        return;
}
void FeedDetailsCallBack(int lineNumber) {
        _Self->CallBackFeedDetails(lineNumber);

        return;
}
void gautier::program::RssDisplay::CallBackFeeds(int lineNumber) {
        std::string FeedSourceName = _Feeds[lineNumber];
        
        //std::cout << "Feed Source Name " << FeedSourceName << "\r\n";

	std::string feed_source_url = std::string(_rss_feed_sources[FeedSourceName].url);

	//std::cout << FeedSourceName << " @ " << feed_source_url << "\r\n";
	
	std::vector<gautier::rss_model::unit_type_rss_item> feed_items = _rss_feed_items[FeedSourceName];
	
	//std::cout << "feed item count: " << feed_items.size() << "\r\n";
	
	_FeedHeadlines.clear();
	
	for(auto& feed_item : feed_items)
	{
		const char* rss_headline = feed_item.title.data();

		_FeedHeadlines.push_back(rss_headline);
	}

        (&_Regions[1])->SetData(_FeedHeadlines);

        _CurrentFeedSourceName = FeedSourceName;

        return;
}
void gautier::program::RssDisplay::CallBackFeedHeadlines(int lineNumber) {
        std::string FeedHeadline = _FeedHeadlines[lineNumber];

	std::vector<gautier::rss_model::unit_type_rss_item> feed_items = _rss_feed_items[_CurrentFeedSourceName];

        _FeedDetails.clear();

	for(auto& feed_item : feed_items)
	{
		if(feed_item.title == FeedHeadline)
		{
			const char* rss_details = feed_item.description.data();

                        _FeedDetails.push_back(rss_details);

                        (&_Regions[2])->SetData(_FeedDetails);

			break;
		}
	}

        return;
}
void gautier::program::RssDisplay::CallBackFeedDetails(int lineNumber) {
        //std::cout << "CallBackFeedDetails line " << lineNumber << "\r\n";

        return;
}
void gautier::program::RssDisplay::UpdateVisualOutput() {
        for(auto& r : _Regions) {
                r.RenderSelf();
        }

        return;
}
void gautier::program::RssDisplay::InitializeData() {
        //std::cout << "Loading data\r\n";

	std::string rss_feeds_sources_file_name = "FeedsList.txt";

	_RssDataModel.load_feeds_source_list(rss_feeds_sources_file_name, _rss_feed_sources);

	for(const auto& rss_feed_source : _rss_feed_sources)
	{
		const char* feed_source_name = rss_feed_source.first.data();

		_Feeds.push_back(feed_source_name);
	}

	if(!_rss_feed_sources.empty())
	{
		_RssDataModel.collect_feeds(_rss_feed_sources);
		
		_RssDataModel.load_feeds(_rss_feed_items);
	}

        //std::cout << "initialized data to " << _Feeds.size() << " feeds, " << _FeedHeadlines.size() << " headlines, " << _FeedDetails.size() << " details\r\n";

        _DataInitialized = true;

        return;
}
void gautier::program::RssDisplay::ProcessData() {
        short RegionNumber = 0;

        _RssDataModel = gautier::rss_model();

        if(!_DataInitialized) {
                InitializeData();
        }

        for(auto& r : _Regions) {
                const bool IsDataSet = r.GetIsDataSet();

                if(!IsDataSet) {
                        switch(RegionNumber) {
                                case 0:
                                        //std::cout << "data item count " << _Feeds.size() << "\r\n";
                                        r.SetInteractiveCallBack(FeedsCallBack);
                                        r.SetData(_Feeds);
                                        break;
                                case 1:
                                        //std::cout << "data item count " << _FeedHeadlines.size() << "\r\n";
                                        r.SetInteractiveCallBack(FeedHeadlinesCallBack);
                                        r.SetData(_FeedHeadlines);
                                        break;
                                case 2:
                                        //std::cout << "data item count " << _FeedDetails.size() << "\r\n";
                                        r.SetInteractiveCallBack(FeedDetailsCallBack);
                                        r.SetData(_FeedDetails);
                                        r.SetIsMultiline(true);
                                        break;
                        }
                }
                
                RegionNumber++;
        }

        return;
}
