//
//  main.cpp
//  list_gen
//
//  Created by Volodymyr Morev on 12/14/14.
//  Copyright (c) 2014 Volodymyr Morev. All rights reserved.
//

#include <string>
#import <Foundation/Foundation.h>
#import <ScriptingBridge/ScriptingBridge.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include <set>
#include <stdio.h>

#import "iTunes.h"

#define SHOW_LOG

int main(int argc, const char * argv[])
{
	std::set<std::string> ArtistNames;
	
	// Scripting iTunes application
	iTunesApplication *iTunes = [SBApplication applicationWithBundleIdentifier:@"com.apple.iTunes"];
	
	// run it if not runing
	bool itunes_runing = [iTunes isRunning];
	if (!itunes_runing)
		[iTunes run];
	
	SBElementArray* sources = [iTunes sources];
	
	iTunesSource* itunes_sources = nullptr;
	uint64_t size = [sources count];
	for (uint64_t i = 0; i < size; i++)
	{
		id obj = [sources objectAtIndex: i ];
		std::string class_name(class_getName([obj class]));
		if (class_name == "ITunesSource")
		{
			itunes_sources = (iTunesSource*)obj;
			break;
		}
	}
	
	if (itunes_sources)
	{
		SBElementArray* play_lists = [itunes_sources libraryPlaylists];
		size = [play_lists count];
		iTunesLibraryPlaylist* library_playlist = nullptr;
		
		for (uint64_t i = 0; i < size; i++)
		{
			id obj = [play_lists objectAtIndex: i ];
			std::string class_name(class_getName([obj class]));
			if (class_name == "ITunesLibraryPlaylist")
			{
				library_playlist = (iTunesLibraryPlaylist*)obj;
				break;
			}
		}
		
		if (library_playlist)
		{
			SBElementArray * file_tracks = [library_playlist fileTracks];
			size = [file_tracks count];
			
			for (uint64_t i = 0; i < size; i++)
			{
				iTunesFileTrack * track = nullptr;
				id obj = [file_tracks objectAtIndex: i ];
				std::string class_name(class_getName([obj class]));
				
				if (class_name == "ITunesFileTrack")
				{
					track = (iTunesFileTrack*)obj;
					
//					key.name = std::string([track.name UTF8String]);
//					key.album = std::string([track.album UTF8String]);
//					key.artist = std::string([track.artist UTF8String]);
//					key.size = track.size;
//					key.time = (long)round((track.finish - track.start) * 1000.);
					
					std::string artist = std::string([track.artist UTF8String]);
					
					ArtistNames.insert(artist);
				}
			}
		}
	}
	
	// quit if we run it
	if (!itunes_runing)
		[iTunes quit];
	
	
	if (ArtistNames.size())
	{
		FILE* file = fopen("/Users/hein/Desktop/ArtistList.txt", "w");
		
		if (file)
		{
		
			for (auto it = ArtistNames.begin(); it != ArtistNames.end(); ++it)
			{
				std::string s = *it;
				s += '\n';
#ifdef SHOW_LOG
				printf("%s  ", s.c_str());
#endif
				int res = fputs(s.c_str(), file);
				
				if (res == EOF || res < 0)
					break;
			}
			
			fclose(file);
		}
	}
	
    return errno;
}
