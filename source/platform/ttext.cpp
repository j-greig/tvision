#define Uses_TText
#include <tvision/tv.h>

#include <internal/codepage.h>
#include <internal/platform.h>
#include <internal/unixcon.h>
#include <internal/linuxcon.h>
#include <internal/win32con.h>
#include <internal/winwidth.h>
#include <internal/utf8.h>
#include <wchar.h>
#include <cstdio>
#include <cstdlib>

namespace ttext
{

// Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
// See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

enum { UTF8_ACCEPT = 0, UTF8_REJECT = 12 };

static const uint8_t utf8d[] =
{
    // The first part of the table maps bytes to character classes that
    // to reduce the size of the transition table and create bitmasks.
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,  0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,  9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,
     7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,  7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
     8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,  2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
    10,3,3,3,3,3,3,3,3,3,3,3,3,4,3,3, 11,6,6,6,5,8,8,8,8,8,8,8,8,8,8,8,

    // The second part is a transition table that maps a combination
    // of a state of the automaton and a character class to a state.
     0,12,24,36,60,96,84,12,12,12,48,72, 12,12,12,12,12,12,12,12,12,12,12,12,
    12, 0,12,12,12,12,12, 0,12, 0,12,12, 12,24,12,12,12,12,12,24,12,24,12,12,
    12,12,12,12,12,12,12,24,12,12,12,12, 12,24,12,12,12,12,12,12,12,24,12,12,
    12,12,12,12,12,12,12,36,12,36,12,12, 12,36,12,12,12,12,12,36,12,36,12,12,
    12,36,12,12,12,12,12,12,12,12,12,12,
};

static inline
uint32_t decode_utf8(uint32_t* state, uint32_t* codep, uint8_t byte) noexcept
{
    uint32_t type = utf8d[byte];

    *codep = (*state != UTF8_ACCEPT) ?
        (byte & 0x3F) | (*codep << 6) :
        (0xFF >> type) & (byte);

    *state = utf8d[256 + *state + type];
    return *state;
}

static inline
uint32_t decode_utf8(uint32_t* state, uint8_t byte) noexcept
{
    uint32_t type = utf8d[byte];
    *state = utf8d[256 + *state + type];
    return *state;
}

static int mbtowc(uint32_t &wc, TStringView text) noexcept
// Pre: text.size() > 0.
// Returns n >= 1 if 'text' begins with a UTF-8 multibyte character that's
// 'n' bytes long, -1 otherwise.
{
    uint32_t state = 0;
    uint32_t codep = 0;
    for (size_t i = 0; i < text.size(); ++i)
        switch (decode_utf8(&state, &codep, text[i]))
        {
            case UTF8_ACCEPT:
                return (wc = codep), i + 1;
            case UTF8_REJECT:
                return -1;
            default:
                break;
        }
    return -1;
}

static int mblen(TStringView text) noexcept
// Pre: text.size() > 0.
// Returns n >= 1 if 'text' begins with a UTF-8 multibyte character that's
// 'n' bytes long, -1 otherwise.
{
    uint32_t state = 0;
    for (size_t i = 0; i < text.size(); ++i)
        switch (decode_utf8(&state, text[i]))
        {
            case UTF8_ACCEPT:
                return i + 1;
            case UTF8_REJECT:
                return -1;
            default:
                break;
        }
    return -1;
}

struct mbstat_r { int length; int width; };

static mbstat_r mbstat(TStringView text) noexcept
// Pre: 'text.size() > 0'.
{
    using namespace tvision;
    uint32_t wc;
    int length = mbtowc(wc, text);
    int width = 1;
    if (length > 1)
        width = Platform::charWidth(wc);
    return {length, width};
}

} // namespace ttext

// ---- Emoji/Grapheme cluster (MVP) ----------------------------------------
namespace ttext {

static inline bool inRange(uint32_t cp, uint32_t a, uint32_t b) noexcept
{ return cp >= a && cp <= b; }

static inline bool isVS16(uint32_t cp) noexcept
{ return cp == 0xFE0F; }

static inline bool isZWJ(uint32_t cp) noexcept
{ return cp == 0x200D; }

static inline bool isEmojiModifier(uint32_t cp) noexcept
{ return inRange(cp, 0x1F3FB, 0x1F3FF); }

static inline bool isRegionalIndicator(uint32_t cp) noexcept
{ return inRange(cp, 0x1F1E6, 0x1F1FF); }

static inline bool isKeycapBase(uint32_t cp) noexcept
{ return cp == '#' || cp == '*' || inRange(cp, '0', '9'); }

// Simplified Extended_Pictographic heuristic: cover common emoji blocks.
static inline bool isExtendedPictographic(uint32_t cp) noexcept
{
    return inRange(cp, 0x1F300, 0x1FAFF) ||
           inRange(cp, 0x2600, 0x26FF)   ||
           inRange(cp, 0x2700, 0x27BF)   ||
           inRange(cp, 0x1F900, 0x1F9FF);
}

// Returns {lengthBytes, widthColumns}. lengthBytes=0 if not a recognized emoji cluster.
static inline mbstat_r emojiClusterStat(TStringView text) noexcept
{
    if (text.size() == 0)
        return {0, 0};
    uint32_t wc = 0; int baseLen = mbtowc(wc, text);
    if (baseLen <= 0)
        return {0, 0};
    size_t i = baseLen;

    // Regional indicator pair (flag): two RIs => width 2 cluster.
    if (isRegionalIndicator(wc)) {
        uint32_t wc2 = 0; int len2 = (i < text.size()) ? mbtowc(wc2, text.substr(i)) : -1;
        if (len2 > 0 && isRegionalIndicator(wc2))
            return { int(baseLen + len2), 2 };
        return {0, 0};
    }

    // Keycap sequence: [#*0-9] (VS16)? U+20E3
    if (isKeycapBase(wc)) {
        uint32_t next = 0; int nlen = (i < text.size()) ? mbtowc(next, text.substr(i)) : -1;
        if (nlen > 0 && isVS16(next)) { i += nlen; nlen = (i < text.size()) ? mbtowc(next, text.substr(i)) : -1; }
        if (nlen > 0 && next == 0x20E3) // COMBINING ENCLOSING KEYCAP
            return { int(i + nlen), 2 };
        return {0, 0};
    }

    // Extended pictographic clusters with optional VS16/modifier and ZWJ joins.
    if (isExtendedPictographic(wc)) {
        uint32_t cp = 0; int len = (i < text.size()) ? mbtowc(cp, text.substr(i)) : -1;
        if (len > 0 && isVS16(cp)) { i += len; len = (i < text.size()) ? mbtowc(cp, text.substr(i)) : -1; }
        if (len > 0 && isEmojiModifier(cp)) { i += len; len = (i < text.size()) ? mbtowc(cp, text.substr(i)) : -1; }
        while (len > 0 && isZWJ(cp)) {
            i += len; // consume ZWJ
            uint32_t nextBase = 0; int blen = (i < text.size()) ? mbtowc(nextBase, text.substr(i)) : -1;
            if (blen <= 0 || !isExtendedPictographic(nextBase))
                break;
            i += blen;
            uint32_t t = 0; int tlen = (i < text.size()) ? mbtowc(t, text.substr(i)) : -1;
            if (tlen > 0 && isVS16(t)) { i += tlen; tlen = (i < text.size()) ? mbtowc(t, text.substr(i)) : -1; }
            if (tlen > 0 && isEmojiModifier(t)) { i += tlen; }
            len = (i < text.size()) ? mbtowc(cp, text.substr(i)) : -1;
        }
        return { int(i), 2 };
    }

    return {0, 0};
}

} // namespace ttext

// Policy: TV_EMOJI_WIDTH = off | auto | force2 (default: auto)
static int emojiPolicy() noexcept
{
    enum { OFF, AUTO, FORCE2 };
    static int policy = []{
        const char *p = std::getenv("TV_EMOJI_WIDTH");
        if (!p) return AUTO;
        if (!std::strcmp(p, "off") || !std::strcmp(p, "OFF")) return OFF;
        if (!std::strcmp(p, "force2") || !std::strcmp(p, "FORCE2") || !std::strcmp(p, "2")) return FORCE2;
        return AUTO;
    }();
    return policy;
}

namespace tvision
{

#ifdef _TV_UNIX
int UnixConsoleAdapter::charWidth(uint32_t wc) noexcept
{
    return wcwidth(wc);
}
#endif // _TV_UNIX

#ifdef __linux__
int LinuxConsoleAdapter::charWidth(uint32_t wc) noexcept
{
    // The Linux Console does not support zero-width characters. It assumes
    // all characters are either single or double-width. Additionally, the
    // double-width characters are the same as in the wcwidth() implementation by
    // Markus Kuhn from 2007-05-26 (https://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c).
    return 1 +
        (wc >= 0x1100 &&
         (wc <= 0x115f ||
          wc == 0x2329 || wc == 0x232a ||
          (wc >= 0x2e80 && wc <= 0xa4cf &&
           wc != 0x303f) ||
          (wc >= 0xac00 && wc <= 0xd7a3) ||
          (wc >= 0xf900 && wc <= 0xfaff) ||
          (wc >= 0xfe10 && wc <= 0xfe19) ||
          (wc >= 0xfe30 && wc <= 0xfe6f) ||
          (wc >= 0xff00 && wc <= 0xff60) ||
          (wc >= 0xffe0 && wc <= 0xffe6) ||
          (wc >= 0x20000 && wc <= 0x2fffd) ||
          (wc >= 0x30000 && wc <= 0x3fffd)));
}
#endif // __linux__

#ifdef _WIN32
int Win32ConsoleAdapter::charWidth(uint32_t wc) noexcept
{
    return WinWidth::width(wc);
}
#endif // _WIN32

} // namespace tvision

size_t TText::width(TStringView text) noexcept
{
    size_t i = 0, width = 0;
    while (TText::next(text, i, width));
    return width;
}

TTextMetrics TText::measure(TStringView text) noexcept
{
    TTextMetrics metrics {};
    size_t i = 0;
    while (true)
    {
        size_t width = 0;
        if (!TText::next(text, i, width))
            break;
        metrics.width += width;
        metrics.characterCount += 1;
        metrics.graphemeCount += (width > 0);
    }
    return metrics;
}

size_t TText::next(TStringView text) noexcept
{
    if (text.size())
        return max(ttext::mblen(text), 1);
    return 0;
}

TText::Lw TText::nextImpl(TStringView text) noexcept
{
    if (text.size())
    {
        int policy = emojiPolicy();
        if (policy != 0) {
            if (auto em = ttext::emojiClusterStat(text); em.length > 0)
                return { size_t(em.length), size_t(em.width) };
            if (policy == 2) {
                uint32_t wc = 0; int baseLen = ttext::mblen(text);
                if (baseLen > 0 && ttext::mbtowc(wc, text) > 0 && ttext::isExtendedPictographic(wc))
                    return { size_t(baseLen), 2 };
            }
        }
        auto mb = ttext::mbstat(text);
        if (mb.length <= 1)
            return {1, 1};
        return {
            size_t(mb.length),
            size_t(mb.width ? max(mb.width, 1) : 0),
        };
    }
    return {0, 0};
}

TText::Lw TText::nextImpl(TSpan<const uint32_t> text) noexcept
{
    using namespace tvision;
    if (text.size())
    {
        int width = Platform::charWidth(text[0]);
        return {
            1,
            size_t(width ? max(width, 1) : 0)
        };
    }
    return {0, 0};
}

size_t TText::prev(TStringView text, size_t index) noexcept
{
    if (index)
    {
        // Try reading backwards character by character, until a valid
        // character is found. This tolerates invalid characters.
        size_t lead = min<size_t>(index, 4);
        for (size_t i = 1; i <= lead; ++i)
        {
            int len = ttext::mblen({&text[index - i], i});
            if (len > 0)
                return size_t(len) == i ? i : 1;
        }
        return 1;
    }
    return 0;
}

char TText::toCodePage(TStringView text) noexcept
{
    using namespace tvision;
    size_t length = TText::next(text);
    if (length == 0)
        return '\0';
    if (length == 1 && (text[0] < ' ' || '\x7F' <= text[0]))
        return text[0];
    return CpTranslator::fromUtf8(text.substr(0, length));
}

template <class Text>
inline TText::Lw TText::scrollImplT(Text text, int count, Boolean includeIncomplete) noexcept
{
    if (count > 0)
    {
        size_t i = 0, w = 0;
        while (true)
        {
            size_t i2 = i, w2 = w;
            if (!TText::next(text, i, w) || w == (size_t) count)
                break;
            if (w > (size_t) count)
            {
                if (!includeIncomplete)
                    i = i2, w = w2;
                break;
            }
        }
        return {i, w};
    }
    return {0, 0};
}

TText::Lw TText::scrollImpl(TStringView text, int count, Boolean includeIncomplete) noexcept

{
    return scrollImplT(text, count, includeIncomplete);
}

TText::Lw TText::scrollImpl(TSpan<const uint32_t> text, int count, Boolean includeIncomplete) noexcept

{
    return scrollImplT(text, count, includeIncomplete);
}

namespace ttext
{

static inline bool isZeroWidthJoiner(TStringView mbc)
// We want to avoid printing certain characters which are usually represented
// differently by different terminal applications or which can combine different
// characters together, changing the width of a whole string.
{
    return mbc == "\xE2\x80\x8D"; // U+200D ZERO WIDTH JOINER.
}

} // namespace ttext

TText::Lw TText::drawOneImpl( TSpan<TScreenCell> cells, size_t i,
                              TStringView text, size_t j ) noexcept
{
    using namespace tvision;
    using namespace ttext;
    if (j < text.size())
    {
        int policy = emojiPolicy();
        if (policy != 0) if (auto em = emojiClusterStat(text.substr(j)); em.length > 0) {
            static bool debug = !!getenv("TV_EMOJI_DEBUG");
            if (i < cells.size())
            {
                // Base codepoint drawn as wide.
                int baseLen = mblen(text.substr(j));
                if (baseLen < 1) baseLen = 1;
                cells[i]._ch.moveMultiByteChar({&text[j], (size_t) baseLen}, /*wide*/ true);

                // Append zero-width parts of the cluster (e.g., VS16, modifiers),
                // but skip ZWJ and subsequent base pictographs which would exceed cell limits
                // and can render inconsistently across terminals.
                size_t k = j + baseLen;
                while ((int)(k - j) < em.length && k < text.size()) {
                    uint32_t cp = 0; int len = mbtowc(cp, text.substr(k));
                    if (len <= 0) break;
                    if (cp == 0x200D) { // ZWJ: skip and do not append
                        k += len; // still consume within cluster
                        // Next codepoint is a base; we will consume but not append.
                    } else if (cp == 0xFE0F || isEmojiModifier(cp)) {
                        // Safe to append as zero-width bytes; fits within 15 bytes budget in most cases.
                        cells[i]._ch.appendZeroWidthChar({&text[k], (size_t) len});
                        k += len;
                    } else {
                        // Any other codepoint within the cluster: consume without appending.
                        k += len;
                    }
                }

                bool drawTrail = (i + 1 < cells.size());
                if (drawTrail)
                    cells[i + 1]._ch.moveWideCharTrail();
                if (debug) {
                    // Dump basic diagnostics for the cluster being drawn.
                    std::fprintf(stderr, "[TV_EMOJI_DEBUG] cell=%zu len=%d width=2 bytes=", i, em.length);
                    for (int b = 0; b < em.length; ++b) {
                        unsigned char uc = (unsigned char)text[j + b];
                        std::fprintf(stderr, "%s%02X", b?" ":"", uc);
                    }
                    // Print first codepoint
                    uint32_t first = 0; int fl = mbtowc(first, text.substr(j));
                    if (fl > 0)
                        std::fprintf(stderr, " cp=U+%04X\n", (unsigned)first);
                    else
                        std::fprintf(stderr, " cp=?\n");
                }
                return {(size_t) em.length, size_t(1 + drawTrail)};
            }
            return { (size_t) em.length, 0 };
        }
        if (policy == 2) {
            uint32_t wc = 0; int baseLen = mblen(text.substr(j));
            if (baseLen > 0 && mbtowc(wc, text.substr(j)) > 0 && isExtendedPictographic(wc)) {
                if (i < cells.size()) {
                    cells[i]._ch.moveMultiByteChar({&text[j], (size_t) baseLen}, /*wide*/ true);
                    bool drawTrail = (i + 1 < cells.size());
                    if (drawTrail)
                        cells[i + 1]._ch.moveWideCharTrail();
                    return { (size_t) baseLen, size_t(1 + drawTrail) };
                }
                return { (size_t) baseLen, 0 };
            }
        }
        auto mb = mbstat(text.substr(j));
        if (mb.length <= 1)
        {
            if (i < cells.size())
            {
                // We need to convert control characters here since we
                // might later try to append combining characters to them.
                if (text[j] < ' ' || '\x7F' <= text[j])
                    cells[i]._ch.moveMultiByteChar(CpTranslator::toPackedUtf8(text[j]));
                else
                    cells[i]._ch.moveChar(text[j]);
                return {1, 1};
            }
        }
        else
        {
            if (mb.width < 0)
            {
                if (i < cells.size())
                {
                    cells[i]._ch.moveMultiByteChar("�");
                    return {(size_t) mb.length, 1};
                }
            }
            else if (mb.width == 0)
            {
                TStringView zwc {&text[j], (size_t) mb.length};
                // Append to the previous cell, if present.
                if (i > 0 && !isZeroWidthJoiner(zwc))
                {
                    size_t k = i;
                    while (cells[--k]._ch.isWideCharTrail() && k > 0);
                    cells[k]._ch.appendZeroWidthChar(zwc);
                }
                return {(size_t) mb.length, 0};
            }
            else
            {
                if (i < cells.size())
                {
                    bool wide = mb.width > 1;
                    cells[i]._ch.moveMultiByteChar({&text[j], (size_t) mb.length}, wide);
                    bool drawTrail = (wide && i + 1 < cells.size());
                    if (drawTrail)
                        cells[i + 1]._ch.moveWideCharTrail();
                    return {(size_t) mb.length, size_t(1 + drawTrail)};
                }
            }
        }
    }
    return {0, 0};
}

TText::Lw TText::drawOneImpl( TSpan<TScreenCell> cells, size_t i,
                              TSpan<const uint32_t> textU32, size_t j ) noexcept
{
    using namespace tvision;
    using namespace ttext;
    if (j < textU32.size())
    {
        char utf8[4] = {};
        size_t length = utf32To8(textU32[j], utf8);
        TStringView textU8(utf8, length);
        int width = Platform::charWidth(textU32[j]);
        if (width < 0)
        {
            if (i < cells.size())
            {
                cells[i]._ch.moveMultiByteChar("�");
                return {1, 1};
            }
        }
        else if (textU32[j] != 0 && width == 0)
        {
            // Append to the previous cell, if present.
            if (i > 0 && !isZeroWidthJoiner(textU8))
            {
                size_t k = i;
                while (cells[--k]._ch.isWideCharTrail() && k > 0);
                cells[k]._ch.appendZeroWidthChar(textU8);
            }
            return {1, 0};
        }
        else
        {
            if (i < cells.size())
            {
                bool wide = width > 1;
                cells[i]._ch.moveMultiByteChar(textU8, wide);
                bool drawTrail = (wide && i + 1 < cells.size());
                if (drawTrail)
                    cells[i + 1]._ch.moveWideCharTrail();
                return {1, size_t(1 + drawTrail)};
            }
        }
    }
    return {0, 0};
}
