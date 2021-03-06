// Copyright 2007-2011 Baptiste Lepilleur
// Distributed under MIT license, or public domain if desired and
// recognized in your jurisdiction.
// See file LICENSE for detail or copy at http://jsoncpp.sourceforge.net/LICENSE
/**
 *  @file   json_reader.cpp
 *  @date   Edited:  2015-01-08 Jochen Neubeck
 */
#include <json/assertions.h>
#include <json/reader.h>
#include <json/value.h>
#include "json_tool.h"

#include <utility>
#include <cstdio>
#include <cassert>
#include <cstring>
#include <sys/stat.h>

namespace Json {

// Implementation of class Reader
// ////////////////////////////////

static bool containsNewLine(Reader::Location begin, Reader::Location end) {
  for (; begin < end; ++begin)
    if (*begin == '\n' || *begin == '\r')
      return true;
  return false;
}

// Class Reader
// //////////////////////////////////////////////////////////////////

Reader::Reader(FILE *file, Features features)
  : features_(features)
  , file_(file) {}

bool Reader::parse(const std::string& document, Value& root, bool collectComments) {
  const char *begin = document.c_str();
  const char *end = begin + document.length();
  if (memcmp(begin, "\xEF\xBB\xBF", 3) == 0)
    begin += 3;
  return parse(begin, end, root, collectComments);
}

bool Reader::parse(Value& root, bool collectComments) {
  int desc = _fileno(file_);
  struct stat stat;
  if (fstat(desc, &stat) != 0)
    return false;
  document_.resize(static_cast<std::string::size_type>(stat.st_size));
  size_t size = fread(const_cast<char *>(document_.c_str()), 1, stat.st_size, file_);
  document_.resize(static_cast<std::string::size_type>(size));
  return parse(document_, root, collectComments);
}

bool Reader::parse(const char* beginDoc,
                   const char* endDoc,
                   Value& root,
                   bool collectComments) {
  if (!features_.allowComments_) {
    collectComments = false;
  }

  begin_ = beginDoc;
  end_ = endDoc;
  collectComments_ = collectComments;
  current_ = begin_;
  std::string queuedComments;
  errors_.clear();
  skipCommentTokens(queuedComments);
  if (!queuedComments.empty()) {
    root.setComment(queuedComments.c_str(), commentBefore);
    queuedComments.resize(0);
  }
  if (features_.strictRoot_) {
    if (token_.type_ != tokenArrayBegin && token_.type_ != tokenObjectBegin) {
      addError(
          "A valid JSON document must be either an array or an object value.");
      return false;
    }
  }
  bool successful = readValue(root);
  skipCommentTokens(queuedComments, &root);
  if (!queuedComments.empty()) {
    root.setComment(queuedComments.c_str(), commentAfter);
    queuedComments.resize(0);
  }
  if (token_.type_ != tokenEndOfStream)
    addError("Parsing ended before end of input");
  return successful;
}

bool Reader::readValue(Value& currentValue) {
  bool successful = true;
  switch (token_.type_) {
  case tokenObjectBegin:
    successful = readObject(currentValue);
    break;
  case tokenArrayBegin:
    successful = readArray(currentValue);
    break;
  case tokenNumber:
    successful = decodeNumber(currentValue);
    break;
  case tokenString:
    successful = decodeString(currentValue);
    break;
  case tokenTrue:
    Value(true).swapPayload(currentValue);
    break;
  case tokenFalse:
    Value(false).swapPayload(currentValue);
    break;
  case tokenArraySeparator:
    if (features_.allowDroppedNullPlaceholders_) {
    case tokenNull:
      Value().swapPayload(currentValue);
      break;
    }
    // fall through
  default:
    successful = false;
    addError("Syntax error: value, object or array expected.");
    break;
  }
  return successful;
}

bool Reader::skipCommentTokens(std::string& queuedComments, Value* lastValue) {
  // deal with comment before comma but not on same line as lastValue
  if (lastValue && !queuedComments.empty()) {
    lastValue->setComment(queuedComments.c_str(), commentAfter);
    queuedComments.resize(0);
    lastValue = 0; // don't put comment after comma on same line as lastValue
  }
  bool found = false;
  do {
    if (readToken())
      lastValue = 0;
    if (token_.type_ != tokenComment)
      break;
    found = true;
    if (collectComments_) {
      if (lastValue && !containsNewLine(token_.start_, token_.end_)) {
        std::string inlineComments = lastValue->getComment(commentAfterOnSameLine);
        if (!inlineComments.empty())
          inlineComments.push_back(' ');
        inlineComments.append(token_.start_, token_.end_);
        lastValue->setComment(inlineComments.c_str(), commentAfterOnSameLine);
      } else {
        if (!queuedComments.empty())
          queuedComments.push_back('\n');
        queuedComments.append(token_.start_, token_.end_);
      }
    }
  } while (features_.allowComments_);
  return found;
}

bool Reader::readToken() {
  bool linebreak = false;
  while (current_ != end_) {
    Char c = *current_;
    if (c == '\r' || c == '\n')
      linebreak = true;
    else if (c != ' ' && c != '\t')
      break;
    ++current_;
  }
  token_.type_ = tokenError;
  token_.start_ = current_;
  Char c = getNextChar();
  switch (c) {
  case '{':
    token_.type_ = tokenObjectBegin;
    break;
  case '}':
    token_.type_ = tokenObjectEnd;
    break;
  case '[':
    token_.type_ = tokenArrayBegin;
    break;
  case ']':
    token_.type_ = tokenArrayEnd;
    break;
  case '"':
    token_.type_ = readString();
    break;
  case '/':
    c = getNextChar();
    switch (c) {
    case '*':
      token_.type_ = readCStyleComment();
      break;
    case '/':
      token_.type_ = readCppStyleComment();
      break;
    }
    break;
  case '0':
  case '1':
  case '2':
  case '3':
  case '4':
  case '5':
  case '6':
  case '7':
  case '8':
  case '9':
  case '-':
    token_.type_ = readNumber();
    break;
  case 't':
    token_.type_ = match("true", tokenTrue);
    break;
  case 'f':
    token_.type_ = match("false", tokenFalse);
    break;
  case 'n':
    token_.type_ = match("null", tokenNull);
    break;
  case ',':
    token_.type_ = tokenArraySeparator;
    break;
  case ':':
    token_.type_ = tokenMemberSeparator;
    break;
  case 0:
    token_.type_ = tokenEndOfStream;
    break;
  }
  token_.end_ = current_;
  return linebreak;
}

Reader::TokenType Reader::match(const Char *pattern, TokenType type) {
  int ahead = static_cast<int>(end_ - current_);
  int index = 0;
  while (Char c = *++pattern)
    if (index >= ahead || current_[index++] != c)
      return tokenError;
  current_ += index;
  return type;
}

Reader::TokenType Reader::readCStyleComment() {
  Char b, c = '\0';
  while (current_ != end_) {
    b = c;
    c = *current_++;
    if (b == '*' && c == '/')
      return tokenComment;
  }
  return tokenError;
}

Reader::TokenType Reader::readCppStyleComment() {
  while (current_ != end_) {
    Char c = *current_;
    if (c == '\r' || c == '\n')
      break;
    ++current_;
  }
  return tokenComment;
}

Reader::TokenType Reader::readNumber() {
  const char *p = current_;
  char c = '0'; // stopgap for already consumed character
  // integral part
  while (c >= '0' && c <= '9')
    c = (current_ = p) < end_ ? *p++ : 0;
  // fractional part
  if (c == '.') {
    c = (current_ = p) < end_ ? *p++ : 0;
    while (c >= '0' && c <= '9')
      c = (current_ = p) < end_ ? *p++ : 0;
  }
  // exponential part
  if (c == 'e' || c == 'E') {
    c = (current_ = p) < end_ ? *p++ : 0;
    if (c == '+' || c == '-')
      c = (current_ = p) < end_ ? *p++ : 0;
    while (c >= '0' && c <= '9')
      c = (current_ = p) < end_ ? *p++ : 0;
  }
  return tokenNumber;
}

Reader::TokenType Reader::readString() {
  while (Char c = getNextChar()) {
    if (c == '"')
      return tokenString;
    if (c == '\\')
      getNextChar();
  }
  return tokenError;
}

bool Reader::readObject(Value& currentValue) {
  std::string name;
  Value(objectValue).swapPayload(currentValue);
  std::string queuedComments;
  Value* lastValue = 0;
  bool comment;
  do {
    comment = skipCommentTokens(queuedComments, lastValue);
    if (token_.type_ == tokenObjectEnd)
      if (lastValue == 0 || features_.allowDroppedNullPlaceholders_)
        break; // empty object or trailing comma
    if (token_.type_ == tokenString) {
      if (!decodeString(name))
        return false;
    } else if (token_.type_ == tokenNumber && features_.allowNumericKeys_) {
      Value numberName;
      if (!decodeNumber(numberName))
        return false;
      name = numberName.asString();
    } else {
      addError("Missing '}' or object member name");
      return false;
    }
    skipCommentTokens(queuedComments);
    if (token_.type_ != tokenMemberSeparator) {
      addError("Missing ':' after object member name");
      return false;
    }
    skipCommentTokens(queuedComments);
    Value& value = currentValue[name];
    if (!queuedComments.empty()) {
      value.setComment(queuedComments.c_str(), commentBefore);
      queuedComments.resize(0);
    }
    lastValue = &value;
    if (!readValue(value)) {
      // error already set
      return false;
    }
    comment = token_.type_ != tokenArraySeparator &&
              skipCommentTokens(queuedComments, lastValue);
  } while (token_.type_ == tokenArraySeparator);
  if (token_.type_ != tokenObjectEnd) {
    addError("Missing ',' or '}' in object declaration");
    return false;
  }
  if (comment) {
    if (lastValue == 0) {
      std::string comment = currentValue.getComment(commentBefore);
      if (!comment.empty())
        comment.push_back('\n');
      comment.append(queuedComments);
      currentValue.setComment(comment.c_str(), commentBefore);
    } else if (!queuedComments.empty()) {
      lastValue->setComment(queuedComments.c_str(), commentAfter);
    }
  }
  return true;
}

bool Reader::readArray(Value& currentValue) {
  Value(arrayValue).swapPayload(currentValue);
  int index = 0;
  std::string queuedComments;
  Value* lastValue = 0;
  bool comment;
  do {
    comment = skipCommentTokens(queuedComments, lastValue);
    if (token_.type_ == tokenArrayEnd)
      if (lastValue == 0 || features_.allowDroppedNullPlaceholders_)
        break; // empty array or trailing comma
    Value& value = currentValue[index++];
    if (!queuedComments.empty()) {
      value.setComment(queuedComments.c_str(), commentBefore);
      queuedComments.resize(0);
    }
    lastValue = &value;
    if (!readValue(value)) {
      // error already set
      return false;
    }
    comment = token_.type_ != tokenArraySeparator &&
              skipCommentTokens(queuedComments, lastValue);
  } while (token_.type_ == tokenArraySeparator);
  if (token_.type_ != tokenArrayEnd) {
    addError("Missing ',' or ']' in array declaration");
    return false;
  }
  if (comment) {
    if (lastValue == 0) {
      std::string comment = currentValue.getComment(commentBefore);
      if (!comment.empty())
        comment.push_back('\n');
      comment.append(queuedComments);
      currentValue.setComment(comment.c_str(), commentBefore);
    } else if (!queuedComments.empty()) {
      lastValue->setComment(queuedComments.c_str(), commentAfter);
    }
  }
  return true;
}

bool Reader::decodeNumber(Value& currentValue) {
  Location current = token_.start_;
  bool isNegative = *current == '-';
  if (isNegative)
    ++current;
  Value::LargestUInt value = 0;
  bool isSigned = true;
  bool isUnsigned = true;
  while (current < token_.end_) {
    Char c = *current++;
    if (c < '0' || c > '9') {
      isSigned = false;
      isUnsigned = false;
      break;
    }
    Value::LargestUInt val = value;
    Value::LargestUInt delta = Value::LargestUInt(c - '0');
    for (int i = 0; i < 10; ++i) {
      value += delta;
      if (-Value::LargestInt(value) > -Value::LargestInt(delta))
        isSigned = false;
      if (value < delta)
        isUnsigned = false;
      delta = val;
    }
  }
  if (isSigned && (isNegative || Value::LargestInt(value) >= 0))
    Value(isNegative ? -Value::LargestInt(value) : Value::LargestInt(value)).swapPayload(currentValue);
  else if (isUnsigned && !isNegative)
    Value(value).swapPayload(currentValue);
  else {
    double value = 0;
    const int bufferSize = 64;
    int count;
    int length = token_.length();
    // Avoid using a string constant for the format control string given to
    // sscanf, as this can cause hard to debug crashes on OS X. See here for more
    // info:
    //
    //     http://developer.apple.com/library/mac/#DOCUMENTATION/DeveloperTools/gcc-4.0.1/gcc/Incompatibilities.html
    static const char format[] = "%lf%c";
    if (length <= bufferSize) {
      Char buffer[bufferSize + 1];
      memcpy(buffer, token_.start_, length);
      buffer[length] = 0;
      count = sscanf(buffer, format, &value, &count);
    } else {
      count = sscanf(token_.asString().c_str(), format, &value, &count);
    }
    if (count != 1) {
      addError(("'" + token_.asString() + "' is not a number.").c_str());
      return false;
    }
    Value(value).swapPayload(currentValue);
  }
  return true;
}

bool Reader::decodeString(Value& currentValue) {
  std::string decoded;
  if (!decodeString(decoded))
    return false;
  Value(decoded).swapPayload(currentValue);
  return true;
}

bool Reader::decodeString(std::string& decoded) {
  decoded.resize(0);
  decoded.reserve(token_.length() - 2);
  Location current = token_.start_ + 1; // skip '"'
  Location end = token_.end_ - 1; // do not include '"'
  unsigned surrogate = 0;
  while (current < end) {
    Char c = *current++;
    assert(c != '"');
    if (c == '\\') {
      assert(current < end);
      c = *current++;
      switch (c) {
      case 'b':
        c = '\b';
        break;
      case 'f':
        c = '\f';
        break;
      case 'n':
        c = '\n';
        break;
      case 'r':
        c = '\r';
        break;
      case 't':
        c = '\t';
        break;
      case '"': case '/': case '\\':
        break;
      case 'u':
        if (unsigned codepoint = decodeUnicodeEscapeSequence(current, end)) {
          // Is this a high surrogate?
          if (codepoint >= 0xD800 && codepoint <= 0xDBFF) {
            // Yes, remember for subsequent iteration
            if (surrogate != 0) {
              addError("Misplaced UTF-16 surrogate", current);
              return false;
            }
            surrogate = codepoint;
            continue;
          }
          // Is this a low surrogate?
          if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
            // Yes, combine with high surrogate
            if (surrogate == 0) {
              addError("Misplaced UTF-16 surrogate", current);
              return false;
            }
            codepoint &= 0x3FF;
            codepoint |= (surrogate & 0x3FF) << 10;
            codepoint += 0x10000;
            surrogate = 0;
          }
          decoded += codePointToUTF8(codepoint);
          continue;
        }
        // fall through
      default:
        addError("Bad escape sequence in string", current);
        return false;
      }
    }
    if (surrogate != 0) {
      addError("Misplaced UTF-16 surrogate", current);
      return false;
    }
    decoded.push_back(c);
  }
  return true;
}

unsigned Reader::decodeUnicodeEscapeSequence(Location& current, Location end) {
  if (end - current < 4)
    return 0;
  unsigned unicode = 0;
  for (int index = 0; index < 4; ++index) {
    Char c = *current++;
    unicode *= 16;
    if (c >= '0' && c <= '9')
      unicode += c - '0';
    else if (c >= 'a' && c <= 'f')
      unicode += c - 'a' + 10;
    else if (c >= 'A' && c <= 'F')
      unicode += c - 'A' + 10;
    else
      return 0;
  }
  return unicode;
}

void Reader::addError(const char* message, Location extra) {
  ErrorInfo info;
  info.token_ = token_;
  info.message_ = message;
  info.extra_ = extra;
  errors_.push_back(info);
}

Reader::Char Reader::getNextChar() {
  if (current_ == end_)
    return 0;
  return *current_++;
}

void Reader::getLocationLineAndColumn(Location location,
                                      int& line,
                                      int& column) const {
  Location current = begin_;
  Location lastLineStart = current;
  line = 0;
  while (current < location && current != end_) {
    Char c = *current++;
    if (c == '\r') {
      if (*current == '\n')
        ++current;
      lastLineStart = current;
      ++line;
    } else if (c == '\n') {
      lastLineStart = current;
      ++line;
    }
  }
  // column & line start at 1
  column = static_cast<int>(location - lastLineStart) + 1;
  ++line;
}

std::string Reader::getLocationLineAndColumn(Location location) const {
  int line, column;
  getLocationLineAndColumn(location, line, column);
  char buffer[18 + 16 + 16 + 1];
  sprintf(buffer, "Line %d, Column %d", line, column);
  return buffer;
}

std::string Reader::getFormattedErrorMessages() const {
  std::string formattedMessage;
  for (Errors::const_iterator itError = errors_.begin();
       itError != errors_.end();
       ++itError) {
    const ErrorInfo& error = *itError;
    formattedMessage +=
        "* " + getLocationLineAndColumn(error.token_.start_) + "\n";
    formattedMessage += "  " + error.message_ + "\n";
    if (error.extra_)
      formattedMessage +=
          "See " + getLocationLineAndColumn(error.extra_) + " for detail.\n";
  }
  return formattedMessage;
}

bool Reader::good() const {
  return !errors_.size();
}

} // namespace Json
