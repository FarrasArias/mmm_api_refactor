#pragma once

#include <map>
#include <string>

// START OF NAMESPACE
namespace mmm {


std::map<std::string,int> GENRE_MAP_DISCOGS = {
	{"NONE",0},
	{"blues",1},
	{"brass & military",2},
	{"children's",3},
	{"classical",4},
	{"electronic",5},
	{"folk, world, & country",6},
	{"funk / soul",7},
	{"hip hop",8},
	{"jazz",9},
	{"latin",10},
	{"non-music",11},
	{"pop",12},
	{"reggae",13},
	{"rock",14},
	{"stage & screen",15}
};
std::map<std::string,int> GENRE_MAP_TAGTRAUM = {
	{"NONE",0},
	{"anime",1},
	{"blues",2},
	{"celtic",3},
	{"childrens",4},
	{"christmas/holiday",5},
	{"classical",6},
	{"comedy",7},
	{"country",8},
	{"dance",9},
	{"drumnbass",10},
	{"dubstep",11},
	{"easylistening",12},
	{"electronic",13},
	{"folk",14},
	{"funk",15},
	{"game",16},
	{"gospel",17},
	{"hiphop",18},
	{"industrial",19},
	{"jazz",20},
	{"latin",21},
	{"newage",22},
	{"reggae",23},
	{"reggaeton",24},
	{"rnb",25},
	{"rock/pop",26},
	{"soundtrack",27},
	{"spoken",28},
	{"techno",29},
	{"trance",30},
	{"world",31}
};
std::map<std::string,int> GENRE_MAP_LASTFM = {
	{"NONE",0},
	{"blues",1},
	{"brazilian",2},
	{"chillout",3},
	{"christian",4},
	{"christmas/holiday",5},
	{"classical",6},
	{"country",7},
	{"dance",8},
	{"electronic",9},
	{"experimental",10},
	{"folk",11},
	{"gospel",12},
	{"gothic",13},
	{"hiphop",14},
	{"instrumental",15},
	{"jazz",16},
	{"jpop/jrock",17},
	{"latin",18},
	{"metal",19},
	{"neofolk",20},
	{"newage",21},
	{"oldie",22},
	{"pop",23},
	{"posthardcore",24},
	{"postrock",25},
	{"reggae",26},
	{"rock",27},
	{"soul",28},
	{"soundtrack",29},
	{"world",30}
};

std::map<std::string,int> get_genres(std::string taxonomy) {
	if (taxonomy == "lastfm") {
		return GENRE_MAP_LASTFM;
	}
	else if (taxonomy == "tagtraum") {
		return GENRE_MAP_TAGTRAUM;
	}
	else if (taxonomy == "discogs") {
		return GENRE_MAP_DISCOGS;
	}
	else {
		throw std::runtime_error("INVALID TAXONOMY");
	}
}

}
// END OF NAMESPACE
