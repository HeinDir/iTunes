//
//  main.cpp
//  test
//
//  Created by Volodymyr Morev on 9/22/13.
//  Copyright (c) 2013 Volodymyr Morev. All rights reserved.
//

#include <string>
#import <Foundation/Foundation.h>
#import <ScriptingBridge/ScriptingBridge.h>
#include <objc/objc.h>
#include <objc/runtime.h>
#include <math.h>
#include <vector>
#include <algorithm>

#import "iTunes.h"

NSString * cFromXML = @"/Users/hein/Developer/temp/from.xml";
//NSString * cToXML = @"/Users/hein/Projects/temp/to.xml";
//NSString * cNewXML = @"/Users/hein/Projects/temp/new.xml";

#define SHOW_LOG 0
#define SHOW_ERROR 0
#define STOP_ON_ERROR 0


struct track_record_t {
	std::string name = "";
	std::string album = "";
	std::string artist = "";
	long size = -1;
	long time = -1;
	long rating = -1;
	long albrtg = -1;
	long is_clc = -1;
};

enum comm_et : int
{
	comm_invalid,
	comm_equal,
	comm_less,
	comm_greater
};

typedef std::vector<track_record_t> track_record_vt;

template <typename T> bool valid(T const & a) {
	bool valid = false;
	return valid;
}

bool valid(std::string const & a) {
	bool valid = !a.empty();
	return valid;
}

bool valid(long const & a) {
	bool valid = a != -1;
	return valid;
}

template <typename T> comm_et comm(T const & l, T const & r) {
	comm_et c = comm_invalid;
	if (valid(l) && valid(r)) {
		if ( !(l == r ) )
			c = (l < r) ? comm_less : comm_greater;
		else
			c = comm_equal;
	}
	return c;
}

int measure(track_record_t const & a, track_record_t const & b)
{
	int likelyhood = 0;
	if (comm(a.artist, b.artist) == comm_equal)  likelyhood += 1;
	if (comm(a.album, b.album) == comm_equal)  likelyhood += 1;
	if (comm(a.name, b.name) == comm_equal)  likelyhood += 1;
	if (comm(a.size, b.size) == comm_equal)  likelyhood += 1;
	if (comm(a.time, b.time) == comm_equal)  likelyhood += 1;
	
	return likelyhood;
}

struct result_t {
	track_record_t const * track = nullptr;
	int likelyhood = 0;
};

bool sort_less (result_t const & a, result_t const & b)
{
	bool res = a.likelyhood < b.likelyhood;
	return res;
}

void find_all(std::vector<result_t> & oResults, track_record_vt::const_iterator first, track_record_vt::const_iterator end, track_record_t const & what)
{
	for (track_record_vt::const_iterator it = first; it != end; ++it)
	{
		track_record_t const & other = *it;
		int likelyhood  = measure(other, what);
		
		if (likelyhood > 0)
		{
			result_t res; res.track = &other; res.likelyhood = likelyhood;
			oResults.push_back(res);
		}
	}
	
	if (oResults.size() > 0)
		std::sort(oResults.begin(), oResults.end(), sort_less);
}

int main(int argc, const char * argv[])
{
	int errorCode = 0;
	
	// get all ratings
	track_record_vt records;
	{
		NSDictionary* dict = [NSDictionary dictionaryWithContentsOfFile: cFromXML];
		NSMutableDictionary* tracks = (NSMutableDictionary*)[dict objectForKey: @"Tracks"];
		if (tracks)
		{
			NSEnumerator *enumerator = [tracks objectEnumerator];
			id value;
			while ((value = [enumerator nextObject]))
			{
				NSDictionary* track = (NSDictionary*)value;
				NSString * name = (NSString *)[track objectForKey: @"Name"];
				NSString * album = (NSString *)[track objectForKey: @"Album"];
				NSString * artist = (NSString *)[track objectForKey: @"Artist"];
				NSNumber * size = (NSNumber *)[track objectForKey: @"Size"];
				NSNumber * time = (NSNumber *)[track objectForKey: @"Total Time"];
				NSNumber * rating = (NSNumber *)[track objectForKey: @"Rating"];
				NSNumber * albrtg = (NSNumber *)[track objectForKey: @"Album Rating"];
				NSNumber * is_clc = (NSNumber *)[track objectForKey: @"Album Rating Computed"];
				if (name || album || artist || size || time || rating || albrtg)
				{
#if SHOW_LOG
					NSLog(@"\"%@\" : rating=%ld, albrtg=%ld, is_clc=%s", name, (rating)?[rating longValue]:-1, (albrtg)?[albrtg longValue]:-1, (is_clc)?(([is_clc boolValue])?"YES":"NO"):"NILL");
#endif
					track_record_t record;
					record.name		= (name)	? [name UTF8String]		: "";
					record.album	= (album)	? [album UTF8String]	: "";
					record.artist	= (artist)	? [artist UTF8String]	: "";
					record.size		= (size)	? [size longValue]		: -1;
					record.time		= (time)	? [time longValue]		: -1;
					record.rating	= (rating)	? [rating longValue]	: -1;
					record.albrtg	= (albrtg)	? [albrtg longValue]	: -1;
					record.is_clc	= (is_clc)	? [is_clc boolValue]	: -1;
					
					records.push_back(record);
				}
			}
		}
	}
	
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
					
					track_record_t key, backup;
					key.name = std::string([track.name UTF8String]);
					key.album = std::string([track.album UTF8String]);
					key.artist = std::string([track.artist UTF8String]);
					key.size = track.size;
					key.time = (long)round((track.finish - track.start) * 1000.);

					std::vector<result_t>  results;
					find_all(results, records.begin(), records.end(), key);
					
					if (!results.empty())
					{
						result_t const & result = results.back();
						if (result.likelyhood > 1)
						{
							track_record_t const * record = result.track;
						
							track.albumRating =	(record->albrtg != -1) ? record->albrtg : 0;
							track.rating =		(record->rating != -1) ? record->rating : 0;
							
							//TODO: handle duplecates!!!
						}
						else
						{
							int a = 5; a++;
						}
					}
					else
					{
						int a = 5; a++;
					}
				}
			}
		}
	}
	
	// quit if we run it
	if (!itunes_runing)
		[iTunes quit];


    return errorCode;
}

