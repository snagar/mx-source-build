///////////////////////////////////////////////////////////////////////////////
// Tokenizer.h
// ===========
// General purpose string tokenizer (C++ string version)
//
// The default delimiters are space(" "), tab(\t, \v), newline(\n),
// carriage return(\r), and form feed(\f).
// If you want to use different delimiters, then use setDelimiter() to override
// the delimiters. Note that the delimiter string can hold multiple characters.
//
//  AUTHOR: Song Ho Ahn (song.ahn@gmail.com)
// CREATED: 2005-05-25
// UPDATED: 2011-03-08
///////////////////////////////////////////////////////////////////////////////

#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <list>
#include <string>
#include <vector>

namespace missionx
{


  // default delimiter string (space, tab, newline, carriage return, form feed)
  //  const std::string DEFAULT_DELIMITER { " \t\v\n\r\f" }; // v3.303.14 removed

class Tokenizer
{
private:
  static const std::string DEFAULT_DELIMITER; //{ " \t\v\n\r\f" }; // v3.303.14 moved inside class to resolve non-POD
public:
  // ctor/dtor
  Tokenizer();
  Tokenizer(const std::string& str, const std::string& delimiter = Tokenizer::DEFAULT_DELIMITER);
  ~Tokenizer();

  // set string and delimiter
  void set(const std::string& str, const std::string& delimiter = Tokenizer::DEFAULT_DELIMITER);
  void setString(const std::string& str);          // set source string only
  void setDelimiter(const std::string& delimiter); // set delimiter string only

  std::string next(); // return the next token, return "" if it ends

  std::vector<std::string> split();                  // return array of tokens from current cursor
  std::list<std::string>   splitByStringDelimeter(); // return list of tokens from current cursor

protected:
private:
  void skipDelimiter(); // ignore leading delimiters
  // std::list <int> listPos;
  std::list<std::string> listTokens;
  bool                   isDelimiter(char c); // check if the current char is delimiter

  std::string                 buffer;    // input string
  std::string                 token;     // output string
  std::string                 delimiter; // delimiter string
  std::string::const_iterator currPos;   // string iterator pointing the current position
  // bool delimiterIsWhole; // saar: a flag that treats delimiter as a full string and not just a character
};

}

#endif // TOKENIZER_H
