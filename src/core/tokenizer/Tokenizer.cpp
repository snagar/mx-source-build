///////////////////////////////////////////////////////////////////////////////
// Tokenizer.cpp
// =============
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

#include "Tokenizer.h"

namespace missionx
{

const std::string Tokenizer::DEFAULT_DELIMITER { " \t\v\n\r\f" };

///////////////////////////////////////////////////////////////////////////////
// constructor
///////////////////////////////////////////////////////////////////////////////
Tokenizer::Tokenizer()
  : buffer("")
  , token("")
  , delimiter(Tokenizer::DEFAULT_DELIMITER)
{
  currPos = buffer.begin();
}

Tokenizer::Tokenizer(const std::string& str, const std::string& delimiter)
  : buffer(str)
  , token("")
  , delimiter(delimiter)
{
  currPos = buffer.begin();
}



///////////////////////////////////////////////////////////////////////////////
// destructor
///////////////////////////////////////////////////////////////////////////////
Tokenizer::~Tokenizer() {}



///////////////////////////////////////////////////////////////////////////////
// reset string buffer, delimiter and the currsor position
///////////////////////////////////////////////////////////////////////////////
void
Tokenizer::set(const std::string& str, const std::string& delimiter)
{
  this->buffer    = str;
  this->delimiter = delimiter;
  this->currPos   = buffer.begin();
}

void
Tokenizer::setString(const std::string& str)
{
  this->buffer  = str;
  this->currPos = buffer.begin();
}

void
Tokenizer::setDelimiter(const std::string& delimiter)
{
  this->delimiter = delimiter;
  this->currPos   = buffer.begin();
}



///////////////////////////////////////////////////////////////////////////////
// return the next token
// If cannot find a token anymore, return "".
///////////////////////////////////////////////////////////////////////////////
std::string
Tokenizer::next()
{
  if (buffer.size() <= 0)
    return ""; // skip if buffer is empty

  token.clear(); // reset token string

  this->skipDelimiter(); // skip leading delimiters

  // append each char to token string until it meets delimiter
  while (currPos != buffer.end() && !isDelimiter(*currPos))
  {
    token += *currPos;
    ++currPos;
  }
  return token;
}



///////////////////////////////////////////////////////////////////////////////
// skip any leading delimiters
///////////////////////////////////////////////////////////////////////////////
void
Tokenizer::skipDelimiter()
{
  while (currPos != buffer.end() && isDelimiter(*currPos))
    ++currPos;
}



///////////////////////////////////////////////////////////////////////////////
// return true if the current character is delimiter
///////////////////////////////////////////////////////////////////////////////
bool
Tokenizer::isDelimiter(char c)
{
  return (delimiter.find(c) != std::string::npos);
}



///////////////////////////////////////////////////////////////////////////////
// split the input string into multiple tokens
// This function scans tokens from the current cursor position.
///////////////////////////////////////////////////////////////////////////////
std::vector<std::string>
Tokenizer::split()
{
  std::vector<std::string> tokens;
  std::string              token;
  while ((token = this->next()) != "")
  {
    tokens.push_back(token);
  }

  return tokens;
}


///////////////////////////////////////////////////////////////////////////////
// split the input string into multiple tokens using a string delimiter and not char delimiter
///////////////////////////////////////////////////////////////////////////////
std::list<std::string>
Tokenizer::splitByStringDelimeter()
{

  static int pos      = 0;
  static int prev_pos = 0;
  static int counter  = 0;

  if (buffer.size() <= 0)
  {
    listTokens.clear(); // skip if buffer is empty
    return listTokens;
  }

  token.clear();

  prev_pos = pos = 0;
  counter        = 0;
  // listPos.clear();
  // split string based on delimiter string (whole string)
  while (pos != (int)std::string::npos && counter < 1000) // counter is there so we won't have endless loop
  {
    token.clear();

    pos = (int)buffer.find(this->delimiter, (size_t)pos);
    if (pos != (int)std::string::npos)
    {
      token = buffer.substr((size_t)prev_pos, (size_t)(pos - prev_pos));

      pos      = pos + (int)this->delimiter.length();
      prev_pos = pos;
    }
    else
    {
      if (!buffer.empty())
      {
        if (prev_pos < (int)buffer.length())
        {
          token = buffer.substr(prev_pos);
        }
      }
    }

    if (!token.empty())
      listTokens.push_back(token);

    ++counter;
  } // end while loop


  return this->listTokens;
}


/***
http://www.songho.ca/misc/tokenizer/tokenizer.html

#include "Tokenizer.h"
#include <iostream>
#include <string>
using namespace std;

int main(int argc, char* argv[])
{
// instanciate Tokenizer class
Tokenizer str("This is a very long string.");
string token;

// Tokenizer::next() returns a next available token from source string
// If it reaches EOS, it returns zero-length string, "".
while((token = str.next()) != "")
{
cout << token << endl;
}
return 0;
}


The output should be like this:
===============================
This
is
a
very
long
string.


***/


}
