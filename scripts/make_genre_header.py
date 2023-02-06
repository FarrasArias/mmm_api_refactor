import csv

ABZ_FOLDER = "/VOLUMES/SCRATCH/GENRE/train/"
GENRE_SOURCES = ["discogs", "tagtraum", "lastfm"]

def make_string_map(d, name):
  template = """
std::map<std::string,int> {name} = {{
\t{inner}
}};"""
  inner = ",\n\t".join(['{{"{}",{}}}'.format(k,v) for k,v in d.items()])
  return template.format(inner=inner, name=name)

with open("../src/mmm_api/enum/genres.h", "w") as out:

  out.write("""#pragma once

#include <map>
#include <string>

// START OF NAMESPACE
namespace mmm {

""")

  for src in GENRE_SOURCES:
    genres = set()
    with open(ABZ_FOLDER + "acousticbrainz-mediaeval2017-{}-train.stats.csv".format(src),"r") as f:
      for row in csv.DictReader(f,delimiter="\t"):
        genres.add( row["genre/subgenre"].split("---")[0] )

    genres = ["NONE"] + sorted(list(genres))
    genres = {g:i for i,g in enumerate(genres)}
    raw = make_string_map(genres, "GENRE_MAP_" + src.upper())

    out.write(raw)
  
  out.write("""

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
""")
