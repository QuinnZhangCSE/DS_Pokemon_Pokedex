// ============================================================================
//
//  IMPORTANT NOTE: DO NOT EDIT THIS FILE
//  (except to add your implementation of Pokemon battle to OutputBattle)
//
// ============================================================================


#include <cassert>
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <cstdlib>
#include <algorithm>
#include <map>
#include <set>
#include <iomanip>


//
// NOTE: This program should be compiled defining the MY_POKEMON_LIST variable:
//
// For example:
//     g++ -DMY_POKEMON_LIST=\"ListXX.txt\" main.cpp pokemon.cpp -o pokemon.out -Wall -g
//
//
// If you are having trouble with this variable, you may uncomment and
// edit this line (replace XX):
//
//#define MY_POKEMON_LIST "List10.txt"
//
// (make sure to recomment this line for submission)
//


// Your Pokemon class implementation should be split into 2 files,
// pokemon.h and pokemon.cpp.  You should not write additional files
#include "pokemon.h"


// helper function prototypes
bool ReadFacts(std::istream &istr, std::map<std::string,std::vector<std::string> >& facts);
Pokemon* CreatePokemon(const std::map<std::string,std::vector<std::string> > &facts);
void OutputByClass(const std::vector<Pokemon*> &pokemons, std::ofstream &ostr);
void OutputBreeding(const std::vector<Pokemon*> &pokemons, std::ofstream &ostr);
void OutputBattle(const std::vector<Pokemon*> &pokemons, std::ifstream &matchup_file, std::ofstream &ostr);



// ============================================================================
// ============================================================================
int main(int argc, char* argv[]) {

  // input parameters
  std::string pokedex_file = "";
  std::string output_file = "";
  bool breeding = false;
  std::string matchup_file = "";

  // parse command line arguments
  for (int i = 1; i < argc; i++) {
    if (argv[i] == std::string("-pokedex")) {
      i++; assert (i < argc);
      pokedex_file = argv[i];
    }
    else if (argv[i] == std::string("-output")) {
      i++; assert (i < argc);
      output_file = argv[i];
    }
    else if (argv[i] == std::string("-breeding")) {
      breeding = true;
    }
    else if (argv[i] == std::string("-matchup")) {
      i++; assert (i < argc);
      matchup_file = argv[i];
    }
    else {
      std::cerr << "ERROR!  Unknown command line argument " << argv[i] << std::endl;
      exit(1);
    }
  }

  // some error checking & open the files
  if (pokedex_file == "") {
    std::cerr << "ERROR: did not specify pokedex file" << std::endl; 
    exit(1);
  }
  if (output_file == "") {
    std::cerr << "ERROR: did not specify output file" << std::endl; 
    exit(1);
  }
  std::ifstream istr(pokedex_file.c_str());
  if (!istr.good()) {
    std::cerr << "ERROR: could not open " << pokedex_file << std::endl; 
    exit(1);
  }    
  std::ofstream ostr(output_file.c_str());
  if (!ostr.good()) {
    std::cerr << "ERROR: could not open " << output_file << std::endl; 
    exit(1);
  }    

  // ----------------------------------------------------
  // create the master container of pokemons
  std::vector<Pokemon*> pokemons;

  // ----------------------------------------------------
  // loop through the input file, reading all characters
  while (1) {
    std::map<std::string,std::vector<std::string> > facts;
    if (!ReadFacts(istr,facts)) break;
    Pokemon* p = CreatePokemon(facts);
    assert (p != NULL);
    // add the new pokemon to the master container
    pokemons.push_back(p);
  }

  // create the output
  OutputByClass(pokemons,ostr);
  if (breeding) { OutputBreeding(pokemons,ostr); }
  if (matchup_file != "") {
    std::ifstream matchups(matchup_file.c_str());
    if (!matchups.good()) {
      std::cerr << "ERROR: could not open " << matchup_file << std::endl;     
      exit(1);
    }
    OutputBattle(pokemons,matchups,ostr); 
  }

  // ----------------------------------------------------  
  // cleanup dynamically allocated memory
  for (unsigned int i = 0; i < pokemons.size(); i++) {
    delete pokemons[i];
  }
  pokemons.clear();
}


// ============================================================================
// ============================================================================
// This helper function parses one character from a Pokedex file. 

bool ReadFacts(std::istream &istr, std::map<std::string,std::vector<std::string> >& facts) {
  std::string token,token2;
  char c;
  bool valid = false;
  facts.clear();
  while (istr >> token) {

    // Each character is delimited by curly braces.
    if (token == "{") {      
      assert (valid == false);
      valid = true;
    } else if (token == "}") {
      assert (valid == true);
      return true;
    }

    else {
      istr >> token2;
      assert (token2 == ":");

      // Some facts are a single string (some can be further parsed
      // into an integer or float)
      if (token == "id" || token == "label" || token == "species" ||
          token == "genderThreshold" || token == "catchRate" ||
          token == "hatchCounter" || token == "height" || token == "weight" ||
          token == "baseExpYield" || token == "baseFriendship" ||
          token == "expGroup" || token == "color") {
        istr >> token2;
        facts[token].push_back(token2);
        assert (facts[token].size() == 1);
      } 

      // Other facts are a vector of 1 or more strings (some can be
      // converted to an integer or float)
      else if (token == "types" || token == "abilities" || token == "eggGroups" ||
               token == "evYield" || token == "baseStats") {
        istr >> token2;
        // We expect square brackets to delimit each vector
        assert (token2 == "[");
        while (istr >> token2) {
          if (token2 == "]") {
            break;
          }
          facts[token].push_back(token2);
        }
        assert (facts[token].size() > 0);
      } 

      // The body style fact is a string with spaces, delimited by double quotes
      else if (token == "bodyStyle") {
        istr >> c;
        std::string t;
        assert (c == '"');
        while (istr >> std::noskipws >> c) {
          if (c == '"') break;
          t.push_back(c);
        }
        facts[token].push_back(t);
        istr >> std::skipws;
      } 

      else {
        std::cerr << "ERROR!  unknown fact type " << token << std::endl;
        exit(1);
      }
    }
  }
  return false;
}



// ============================================================================
// ============================================================================
// This function determines the most specific type of Pokemon that can
// be created from the collection of facts.  It does this by process
// of elimination.  Note that the order in which it attempts to create
// the shapes is important.
//
// NOTE: C++ macros are used to insert your specific Pokemon classes.
// The function call-like macros are actually replaced using
// substitution by the preprocessor before the code is given to the
// compiler.
//
// (You are not required to understand the details of the macros.  You
// do not need to edit this code.)
//

Pokemon* CreatePokemon(const std::map<std::string,std::vector<std::string> > &facts) {

  Pokemon *answer = NULL;
  
  //
  // We need to go through all of the characters in MY_POKEMON LIST
  // and try to create each class with these facts.  If a constructor
  // fails, try the next.  When a constructor succeeds, stop going
  // through the list.
  //
  // So, if the MY_POKEMON_LIST file contains:
  //   POKENAME(Pikachu)
  //   POKENAME(Pichu)
  //   POKENAME(Field)
  //
  // This is the code we would need to run:
  //
  //   try { 
  //     answer = new Pikachu(facts); 
  //   } 
  //   catch (int) {
  //     try { 
  //       answer = new Pichu(facts); 
  //     } 
  //     catch (int) {
  //       try { 
  //         answer = new Field(facts); 
  //       } 
  //       catch (int) {
  //         
  //         answer = new Pokemon(facts); 
  // 
  //       }
  //     }
  //   }
  // 
  // The C++ preprocessor macro below does this expansion for your
  // specific list of Pokemon.
  //
  // We have a try block for each specific Pokemon constructor.  The
  // placeholder "type" is substituted with each of your assigned
  // classes.  Note, we have an extra open { for the catch block, so
  // these try blocks are nested!

#define POKENAME(type)  try { answer = new type(facts); } catch (int) {
#include MY_POKEMON_LIST
#undef POKENAME
  
  // The innermost catch block....  If all of the above constructors
  // failed, then we will make a base type Pokemon.  (This constructor
  // should not fail!)
  answer = new Pokemon(facts);
  
  // Close each of the nested catch blocks
# define POKENAME(type)  }
# include MY_POKEMON_LIST
# undef POKENAME
  
  assert (answer != NULL);
  return answer;
}


// ============================================================================
// This function prints the output.  As above, C++ preprocessor macros
// are used to substitute your specific Pokemon classes and abbreviate
// some repetitive code.

void OutputByClass(const std::vector<Pokemon*> &pokemons, std::ofstream &ostr) {

  // create a top level map of the names
  std::map<std::string,std::set<std::string> > data;

  // loop over all Pokemon and count & record the names of shapes in each class
  for (std::vector<Pokemon*>::const_iterator i = pokemons.begin(); i!=pokemons.end(); ++i) {

    // attempt to cast each character to each class.  If the cast
    // succeeds, then add the character to that collection.
#   define POKENAME(type) if (dynamic_cast<type*> (*i)) { data[#type].insert((*i)->getLabel()); }

    // loop over your specific Pokemon
#   include MY_POKEMON_LIST

    // loop over the Egg Groups (& base class Pokemon)
#   include "EggGroupList.txt"

#   undef POKENAME
  }   
    
  // output data for each category, sorted alphabetically by the characters label
  for (std::map<std::string,std::set<std::string> >::iterator itr = data.begin(); itr != data.end(); itr++) {
    ostr << std::setw(3) << std::right << itr->second.size() << " " << std::left << std::setw(15) << itr->first+"(s):";
    if (itr->second.size() <= 10) {
      for (std::set<std::string>::iterator itr2 = itr->second.begin(); itr2 != itr->second.end(); itr2++) {
        ostr << " " << *itr2;
      }
    }
    ostr << std::endl;
  }
}


// ============================================================================
void OutputBreeding(const std::vector<Pokemon*> &pokemons, std::ofstream &ostr) {
  // a doubly-nested loop over all characters
  for (unsigned int i = 0; i < pokemons.size(); i++) {
    ostr << pokemons[i]->getLabel() << " can breed with: ";
    for (unsigned int j = 0; j < pokemons.size(); j++) {
      // skip yourself
      if (i == j) continue;
      // Two pokemon can breed if they share an Egg Group
      if (pokemons[i]->SharesEggGroup(pokemons[j])) {
        ostr << " " << pokemons[j]->getLabel();
      }
    }
    ostr << std::endl;
  }
}


// ============================================================================
void OutputBattle(const std::vector<Pokemon*> &pokemons, std::ifstream &matchup_file, std::ofstream &ostr) {

  //
  // EXTRA CREDIT: Use the information in the matchups file to output
  // information about Pokemon Training/Battle.
  //

  // This is the only section of the file you should edit.

}

// ============================================================================
