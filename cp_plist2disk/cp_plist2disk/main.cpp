//
//  main.cpp
//  cp_plist2disk
//
//  Created by Volodymyr Morev on 3/8/15.
//  Copyright (c) 2015 Volodymyr Morev. All rights reserved.
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
#include <map>

#import "iTunes.h"

const std::string cPlaylist = "playlist";
const std::string cFolder = "folder";
static std::map<std::string, id> args = { { cPlaylist, nil }, { cFolder, nil } };

id getValueForKey(std::string const & str, std::string const & key)
{
	id result = nil;
	std::string::size_type pos = str.find(key);
	pos += key.size();
	pos += 1; // size of "=" symbol
	
	std::string value = str.substr(pos); // until end of string
	
	NSString* nsstr = nil;
	if (!value.empty()) @autoreleasepool
	{
		nsstr = [NSString stringWithCString:value.c_str() encoding:NSUTF8StringEncoding];
		
		if ([nsstr length] > 0)
		{
			if (key == cPlaylist)
			{
				result = nsstr;
			}
			else
			if (key == cFolder)
			{
				NSFileManager* fileMgr = [NSFileManager defaultManager];

				if (fileMgr != nil)
				{
					BOOL isFolder = NO;
					BOOL exists = [fileMgr fileExistsAtPath: nsstr isDirectory: &isFolder];
					BOOL writeAccess = [fileMgr isWritableFileAtPath: nsstr];
					
					if (exists && isFolder && writeAccess)
					{
						result = [NSURL fileURLWithPath: nsstr isDirectory: isFolder];
					}
				}
			}
		}
	}
	return result;
}

int main(int argc, const char * argv[])
{
	bool gotAllRequiredArgs = false;
	
	if (argc >= 3)
	{
		for (int i(0); i < argc; ++i)
		{
			const char * arg = argv[i];
			std::for_each(args.begin(), args.end(), [arg](decltype(args)::value_type & pair)
			  {
				  std::string const & key = pair.first;
				  //id* value = &pair.second;
				  
				  if (pair.second == nil)
				  {
					  std::string str(arg);
					  
					  std::string::size_type pos = str.find(key);
					  if (pos != std::string::npos)
					  {
						  pair.second = getValueForKey(str, key);
					  }
				  }
				  
				  printf ("%p\n", pair.second);
			  });
			
		}
		
		bool anyNilValueLeft = false;
		std::for_each(args.begin(), args.end(), [&anyNilValueLeft](decltype(args)::value_type const & pair)
		  {
			  id value = pair.second;
			  if (value == nil)
			  {
				  anyNilValueLeft = true;
			  };
		  });
		
		gotAllRequiredArgs = !anyNilValueLeft;
	}
	
	
	if (gotAllRequiredArgs) @autoreleasepool
	{
		
		NSString* choosenPlaylist = args[cPlaylist];
		NSURL* destinationFolder = args[cFolder];
		NSFileManager* fileMgr = [NSFileManager defaultManager];
		
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

		iTunesUserPlaylist* playlist = nullptr;
		
		if (itunes_sources)
		{
			SBElementArray* play_lists = [itunes_sources userPlaylists];//libraryPlaylists];
			size = [play_lists count];
			
			for (uint64_t i = 0; i < size; i++)
			{
				id obj = [play_lists objectAtIndex: i ];
				std::string class_name(class_getName([obj class]));
				if (class_name == "ITunesUserPlaylist")
				{
					iTunesUserPlaylist* plist = (iTunesUserPlaylist*)obj;
					NSString* name = [plist name];
					if ( name != nil && [name isEqualToString: choosenPlaylist] )
					{
						playlist = plist;
						break;
					}
				}
			}
		}

		if (playlist)
		{
			SBElementArray * tracks = [playlist fileTracks];
			size = [tracks count];
				
			for (uint64_t i = 0; i < size; i++)
			{
				iTunesFileTrack * track = nullptr;
				id obj = [tracks objectAtIndex: i ];
				std::string class_name(class_getName([obj class]));
					
				if (class_name == "ITunesFileTrack")
				{
					track = (iTunesFileTrack*)obj;
					NSURL* loc = [track location];
					
					if (loc != nil )
					{
						BOOL a_file = loc.fileURL;
						NSError * error = nil;
						BOOL file_reachable = [loc checkResourceIsReachableAndReturnError: &error];
						
						if (a_file && file_reachable)
						{
							NSError * error2 = nil;
							NSString* lastPiece = loc.lastPathComponent;
							NSURL* target = nil;
							if (lastPiece)
							{
								target = [destinationFolder URLByAppendingPathComponent: lastPiece];
								//target = [NSURL URLWithString: lastPiece relativeToURL: destinationFolder];
							}
							
							if (target)
							{
								BOOL done = [fileMgr copyItemAtURL:loc toURL:target error:&error2];
								if (done)
								{
									NSLog(@"Copy '%s' to '%s'", [loc fileSystemRepresentation], [target fileSystemRepresentation]);
								}
							}
						}
					}
				}
			}
		 }

		// quit if we run it
		if (!itunes_runing)
			[iTunes quit];
		
	}
	return errno;
}
