//******************************************************************************
///
/// @file parser/parsertypes.h
///
/// Essential types and forward declarations used throughout the parser.
///
/// @copyright
/// @parblock
///
/// Persistence of Vision Ray Tracer ('POV-Ray') version 3.8.
/// Copyright 1991-2019 Persistence of Vision Raytracer Pty. Ltd.
///
/// POV-Ray is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as
/// published by the Free Software Foundation, either version 3 of the
/// License, or (at your option) any later version.
///
/// POV-Ray is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.
///
/// ----------------------------------------------------------------------------
///
/// POV-Ray is based on the popular DKB raytracer version 2.12.
/// DKBTrace was originally written by David K. Buck.
/// DKBTrace Ver 2.0-2.12 were written by David K. Buck & Aaron A. Collins.
///
/// @endparblock
///
//******************************************************************************

#ifndef POVRAY_PARSER_PARSERTYPES_H
#define POVRAY_PARSER_PARSERTYPES_H

// Module config header file must be the first file included within POV-Ray unit header files
#include "parser/configparser.h"

// C++ variants of C standard header files
//  (none at the moment)

// C++ standard header files
#include <memory>

// Boost header files
//  (none at the moment)

// POV-Ray header files (base module)
#include "base/types.h"
#include "base/messenger.h"

// POV-Ray header files (core module)
#include "core/coretypes.h"

namespace pov_base
{
class IStream;
}

namespace pov_parser
{

using namespace pov_base;

enum TokenId : int;

//------------------------------------------------------------------------------

using StreamPtr = shared_ptr<pov_base::IStream>;
using ConstStreamPtr = shared_ptr<const pov_base::IStream>;

//------------------------------------------------------------------------------

struct LexemePosition : pov::SourcePosition
{
    LexemePosition();
    bool operator==(const LexemePosition& o) const;
    bool operator!=(const LexemePosition& o) const { return !(*this == o); }
    POV_OFF_T operator-(const LexemePosition& o) const;
};

//------------------------------------------------------------------------------

/// Error detected by the _scanner_ or _raw tokenizer_ stage of the parser.
///
/// This type serves as the base class for all exceptions thrown by @ref Scanner
/// or @ref RawTokenizer, indicating a malformed scene file.
///
/// Exceptions of this type carry information about the location of the error
/// (file name, binary offset, line and column), exposed as public data members.
/// For convenience, access to this information is also provided via the
/// @ref MessageContext interface.
///
struct TokenizerException : std::exception, MessageContext
{
    /// Name of the stream in which the error was encountered.
    UCS2String offendingStreamName;

    /// Location at which the error was encountered.
    LexemePosition offendingPosition;

    TokenizerException(const UCS2String& osn, const LexemePosition& op);

    virtual UCS2String GetFileName() const override;
    virtual POV_LONG GetLine() const override;
    virtual POV_LONG GetColumn() const override;
    virtual POV_OFF_T GetOffset() const override;
};

/// Missing end-of-comment marker in block comment.
///
/// This exception is thrown by @ref Scanner (and passed on by
/// @ref RawTokenizer) to indicate that a "C-style" block comment start sequence
/// (`/*`) was encountered without a matching end sequence (`*/`), implying a
/// broken comment or a comment nesting error.
///
struct IncompleteCommentException : TokenizerException
{
    IncompleteCommentException(const UCS2String& osn, const LexemePosition& op);
};

/// Missing end-of-string marker in string literal.
///
/// This exception is thrown by @ref Scanner (and passed on by
/// @ref RawTokenizer) to indicate that an unbalanced double quote (`"`) was
/// encountered, implying a broken string literal.
///
struct IncompleteStringLiteralException : TokenizerException
{
    IncompleteStringLiteralException(const UCS2String& osn, const LexemePosition& op);
};

/// Invalid encoding in input file.
///
/// This exception is thrown by @ref Scanner (and passed on by
/// @ref RawTokenizer) to indicate that an octet or octet sequence encountered
/// in the data stream does not conform to the expected character encoding
/// scheme, implying a broken or malformed file.
///
struct InvalidEncodingException : TokenizerException
{
    /// Descriptive name of the expected encoding scheme.
    const char* encodingName;

    /// Brief description on the nature of the encoding scheme violation.
    /// May be `nullptr` to indicate that no further information is available
    /// or necessary.
    const char* details;

    InvalidEncodingException(const UCS2String& osn, const LexemePosition& op, const char* encodingName, const char* details = nullptr);
};

/// Invalid character in input file.
///
/// This exception is thrown by @ref Scanner (and passed on by
/// @ref RawTokenizer) to indicate that an unexpected ASCII control character
/// or non-ASCII character was encountered outside a string literal or comment.
///
struct InvalidCharacterException : TokenizerException
{
    /// UCS code point corresponding to the unexpected character.
    UCS4 offendingCharacter;

    InvalidCharacterException(const UCS2String& osn, const LexemePosition& op, UCS4 oc);
};

/// Invalid escape sequence in string literal.
///
/// This exception is thrown by @ref AmbiguousStringValue (as created by
/// @ref RawTokenizer) to indicate that an unexpected sequence of characters was
/// encountered after a string literal escape character (`\`) while trying to
/// evaluate the literal in a non-filename context, implying a broken string
/// literal, malformed escape sequence or failure to properly escape a literal
/// backslash character.
///
struct InvalidEscapeSequenceException : TokenizerException
{
    /// Offending escape sequence, including leading escape character.
    UTF8String offendingText;

    InvalidEscapeSequenceException(const UCS2String& osn, const LexemePosition& op, const UTF8String& ot);
    InvalidEscapeSequenceException(const UCS2String& osn, const LexemePosition& op,
                                   const UTF8String::const_iterator& otb, const UTF8String::const_iterator& ote);
};

//------------------------------------------------------------------------------

/// Base class for miscellaneous things that can be assigned to a symbol.
struct Assignable
{
    virtual ~Assignable() {}
    virtual Assignable* Clone() const = 0;
};

//------------------------------------------------------------------------------

struct ParserOptions
{
    bool    useClock;
    DBL     clock;
    size_t  randomSeed;
    ParserOptions(bool uc, DBL c, size_t rs) : useClock(uc), clock(c), randomSeed(rs) {}
};

//------------------------------------------------------------------------------

/// Value identifying a character encoding scheme.
///
/// Each value of this type represents a particular scheme for encoding
/// sequences of characters as sequences of octets.
///
enum class CharacterEncodingID
{
    kAutoDetect,    ///< Auto-detect (UTF-8 or Windows-1252 or compatible).

    kASCII,         ///< Strict ASCII.
    kLatin1,        ///< Strict ISO-8859-1 aka Latin-1.
    kMacOSRoman,    ///< Mac OS Roman (as used on classic Mac OS).
    kWindows1252,   ///< Windows code page 1252 aka [incorrectly] ANSI.

    kUTF8,          ///< Strict UTF-8.
};

//------------------------------------------------------------------------------

class FontReference;
using FontReferencePtr = std::shared_ptr<FontReference>;

enum class FontStyle
{
    kRegular    = 0x00,
    kBold       = 0x01,
    kItalic     = 0x02,
    kBoldItalic = 0x03
};

inline FontStyle operator|(FontStyle a, FontStyle b) { return EnumOr(a, b); }
inline FontStyle operator&(FontStyle a, FontStyle b) { return EnumAnd(a, b); }
inline FontStyle operator~(FontStyle a) { return EnumNot(a); }
inline FontStyle& operator|=(FontStyle& a, FontStyle b) { a = a | b; return a; }
inline FontStyle& operator&=(FontStyle& a, FontStyle b) { a = a & b; return a; }

class FontResolver
{
public:
    virtual ~FontResolver();
    virtual FontReferencePtr GetFont(const UCS2String& name, FontStyle style) = 0;
};

//------------------------------------------------------------------------------

} // end of namespace

#endif // POVRAY_PARSER_PARSERTYPES_H
