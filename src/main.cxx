#include <allegro5/allegro.h>
#include <RssReader.hxx>
/*
	Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0 . Software distributed under the License is distributed on an "AS IS" BASIS, NO WARRANTIES OR CONDITIONS OF ANY KIND, explicit or implicit. See the License for details on permissions and limitations.
*/
static gautier::program::RssReader 
        _RssReader;

int main(int argc, char **argv) {
        if(argc > 0 && argv) {
                std::cout << "args " << argc << "\r\n";
        }

	_RssReader.Start();
	
	return 0;
}

