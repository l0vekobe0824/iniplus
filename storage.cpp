/*************
**
** Project:      inixx
** Author:       Copyright (C) 2013 Kuzma Shapran <Kuzma.Shapran@gmail.com>
** License:      LGPLv2.1+
**
** Description: inixx is a cross-platform C++ library that provides
** the simplest support of INI files.
**
** This program or library is free software; you can redistribute it
** and/or modify it under the terms of the GNU Lesser General Public
** License as published by the Free Software Foundation; either
** version 2.1 of the License, or (at your option) any later version.
**
** This library is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
** Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General
** Public License along with this library; if not, write to the
** Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
** Boston, MA 02110-1301 USA
**
*************/


#include "storage.hpp"

#include <map>


namespace ini {

class StorageImpl
{
private:
    typedef enum Context {
        CONTEXT__NEWLINE,
        CONTEXT__COMMENT,
        CONTEXT__SECTION_START,
        CONTEXT__SECTION_NAME,
        CONTEXT__SECTION_HEX1,
        CONTEXT__SECTION_HEX2,
        CONTEXT__SECTION_END,
        CONTEXT__SECTION_CLOSE,
        CONTEXT__KEY_NAME,
        CONTEXT__KEY_HEX1,
        CONTEXT__KEY_HEX2,
        CONTEXT__KEY_END,
        CONTEXT__EQUAL,
        CONTEXT__VALUE_QUOTED,
        CONTEXT__VALUE_START,
        CONTEXT__VALUE_ESCAPED,
        CONTEXT__VALUE_HEX1,
        CONTEXT__VALUE_HEX2,
        CONTEXT__VALUE_END
    } Context;

    typedef enum CharClass {
        CHAR_CLASS__NEWLINE,      // \r \n
        CHAR_CLASS__SPACE,        // \s \t
        CHAR_CLASS__SEMICOLON,    // ;
        CHAR_CLASS__OPENBRACKET,  // [
        CHAR_CLASS__CLOSEBRACKET, // ]
        CHAR_CLASS__PERCENT,      // %
        CHAR_CLASS__HEXDIGIT,     // 0-9A-Fa-f
        CHAR_CLASS__LETTERS,      // G-Zg-z
        CHAR_CLASS__MINUS,        // _.-
        CHAR_CLASS__EQUAL,        // =
        CHAR_CLASS__QUOTE,        // "
        CHAR_CLASS__BACKSLASH,    // \\ (backslash)
        CHAR_CLASS__COMMA,        // ,
        CHAR_CLASS__VISIBLE,      // other 0x21-0x7e
        CHAR_CLASS__OTHER         // other 0x00-0x1f, 0x7f-0xff
    } CharClass;


    typedef std::map<std::string, Storage::Values> Keys;
    typedef std::map<std::string, Keys> Sections;

public:
    StorageImpl();

    ~StorageImpl();

    bool parse(const std::string &text, Storage::Callback *callback)
    {
/*  [ A-Za-z0-9_-. %xx ] ;...
 *  s                    c
 *  s n             hxec
 *
 *  A-Za-z0-9_-. %xx = "0x21-0x7e \? \xxx" , ;...
 *  k                e v                   e c
 *  n             hx q qs          b   hxe q
 */
        clear();

        Context context = CONTEXT__NEWLINE;
        Context last_context = context; // to shut up the compiler

        std::string current_section;
        std::string current_key;
        Storage::Values current_values;
        Storage::Value current_value;

        size_t length = text.length();

        size_t cur_char = 1;
        size_t cur_line = 1;
        size_t cur_pos = 0;
        for (; cur_pos < length; ++cur_pos, ++cur_char)
        {
            const char &input = text[cur_pos];

            CharClass char_class = get_char_class(input);

            bool fail = false;

            switch (context)
            {
            case CONTEXT__NEWLINE:
                current_key.clear();
                switch (char_class)
                {
                case CHAR_CLASS__NEWLINE:
                case CHAR_CLASS__SPACE:
                    break;

                case CHAR_CLASS__SEMICOLON:
                    context = CONTEXT__COMMENT;
                    break;

                case CHAR_CLASS__OPENBRACKET:
                    current_section.clear();
                    context = CONTEXT__SECTION_START;
                    break;

                case CHAR_CLASS__HEXDIGIT:
                case CHAR_CLASS__LETTERS:
                case CHAR_CLASS__MINUS:
                    current_key += input;
                    context = CONTEXT__KEY_NAME;
                    break;

                case CHAR_CLASS__PERCENT:
                    context = CONTEXT__KEY_HEX1;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__COMMENT:
                switch (char_class)
                {
                case CHAR_CLASS__NEWLINE:
                    context = CONTEXT__NEWLINE;
                    break;

                default:;
                }
                break;

            case CONTEXT__SECTION_START:
                switch (char_class)
                {
                case CHAR_CLASS__SPACE:
                    break;

                case CHAR_CLASS__HEXDIGIT:
                case CHAR_CLASS__LETTERS:
                case CHAR_CLASS__MINUS:
                    current_section += input;
                    context = CONTEXT__SECTION_NAME;
                    break;

                case CHAR_CLASS__PERCENT:
                    context = CONTEXT__SECTION_HEX1;
                    break;

                case CHAR_CLASS__CLOSEBRACKET:
                    context = CONTEXT__SECTION_CLOSE;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__SECTION_NAME:
                switch (char_class)
                {
                case CHAR_CLASS__SPACE:
                    context = CONTEXT__SECTION_END;
                    break;

                case CHAR_CLASS__HEXDIGIT:
                case CHAR_CLASS__LETTERS:
                case CHAR_CLASS__MINUS:
                    current_section += input;
                    break;

                case CHAR_CLASS__PERCENT:
                    context = CONTEXT__SECTION_HEX1;
                    break;

                case CHAR_CLASS__CLOSEBRACKET:
                    context = CONTEXT__SECTION_CLOSE;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__SECTION_HEX1:
                switch (char_class)
                {
                case CHAR_CLASS__HEXDIGIT:
                    current_section += char_to_hex(input) << 4;
                    context = CONTEXT__SECTION_HEX2;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__SECTION_HEX2:
                switch (char_class)
                {
                case CHAR_CLASS__HEXDIGIT:
                    current_section[current_section.length() - 1] |= char_to_hex(input);
                    if (!current_section[current_section.length() - 1])
                        if (callback)
                            callback->warning(Storage::PARSE_WARNING__BINARY_ZERO_IN_SECTION_NAME, cur_char - 2, cur_line, cur_pos - 2);
                    context = CONTEXT__SECTION_NAME;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__SECTION_END:
                switch (char_class)
                {
                case CHAR_CLASS__SPACE:
                    break;

                case CHAR_CLASS__CLOSEBRACKET:
                    context = CONTEXT__SECTION_CLOSE;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__SECTION_CLOSE:
                switch (char_class)
                {
                case CHAR_CLASS__NEWLINE:
                    context = CONTEXT__NEWLINE;
                    break;

                case CHAR_CLASS__SPACE:
                    break;

                case CHAR_CLASS__SEMICOLON:
                    context = CONTEXT__COMMENT;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__KEY_NAME:
                switch (char_class)
                {
                case CHAR_CLASS__SPACE:
                    context = CONTEXT__KEY_END;
                    break;

                case CHAR_CLASS__HEXDIGIT:
                case CHAR_CLASS__LETTERS:
                case CHAR_CLASS__MINUS:
                    current_key += input;
                    break;

                case CHAR_CLASS__PERCENT:
                    context = CONTEXT__KEY_HEX1;
                    break;

                case CHAR_CLASS__EQUAL:
                    context = CONTEXT__EQUAL;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__KEY_HEX1:
                switch (char_class)
                {
                case CHAR_CLASS__HEXDIGIT:
                    current_key += char_to_hex(input) << 4;
                    context = CONTEXT__KEY_HEX2;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__KEY_HEX2:
                switch (char_class)
                {
                case CHAR_CLASS__HEXDIGIT:
                    current_key[current_key.length() - 1] |= char_to_hex(input);
                    if (!current_key[current_key.length() - 1])
                        if (callback)
                            callback->warning(Storage::PARSE_WARNING__BINARY_ZERO_IN_KEY_NAME, cur_char - 2, cur_line, cur_pos - 2);
                    context = CONTEXT__KEY_NAME;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__KEY_END:
                switch (char_class)
                {
                case CHAR_CLASS__SPACE:
                    break;

                case CHAR_CLASS__EQUAL:
                    current_values.clear();
                    current_value.clear();
                    context = CONTEXT__EQUAL;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__EQUAL:
                switch (char_class)
                {
                case CHAR_CLASS__NEWLINE:
                    current_values += current_value;
                    current_value.clear();
                    set_values(current_section, current_key, current_values);
                    context = CONTEXT__NEWLINE;
                    break;

                case CHAR_CLASS__SPACE:
                    break;

                case CHAR_CLASS__QUOTE:
                    context = CONTEXT__VALUE_QUOTED;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__VALUE_QUOTED:
                switch (char_class)
                {
                case CHAR_CLASS__QUOTE:
                    current_values += current_value;
                    current_value.clear();
                    context = CONTEXT__VALUE_END;
                    break;

                case CHAR_CLASS__BACKSLASH:
                    last_context = context;
                    context = CONTEXT__VALUE_ESCAPED;
                    break;

                default:
                    if ((input >= 0x20) && (input < 0x7f))
                        current_value += input;
                    else
                        fail = true;
                }
                break;

            case CONTEXT__VALUE_START:
                switch (char_class)
                {
                case CHAR_CLASS__NEWLINE:
                    current_values += trim(current_value);
                    current_value.clear();
                    set_values(current_section, current_key, current_values);
                    context = CONTEXT__NEWLINE;
                    break;

                case CHAR_CLASS__SPACE:
                    current_value += input;
                    break;

                case CHAR_CLASS__SEMICOLON:
                    context = CONTEXT__COMMENT;
                    break;

                case CHAR_CLASS__BACKSLASH:
                    last_context = context;
                    context = CONTEXT__VALUE_ESCAPED;
                    break;

                case CHAR_CLASS__COMMA:
                    current_values += trim(current_value);
                    current_value.clear();
                    context = CONTEXT__EQUAL;
                    break;

                default:
                    if ((input >= 0x20) && (input < 0x7f))
                        current_value += input;
                    else
                        fail = true;
                }
                break;

            case CONTEXT__VALUE_ESCAPED:
                switch (input) // !! INPUT, NOT CLASS
                {
                case '0':
                    current_value += '\0';
                    context = CONTEXT__VALUE_QUOTED;
                    break;

                case 'a':
                    current_value += '\a';
                    context = CONTEXT__VALUE_QUOTED;
                    break;

                case 'b':
                    current_value += '\b';
                    context = CONTEXT__VALUE_QUOTED;
                    break;

                case 'f':
                    current_value += '\f';
                    context = CONTEXT__VALUE_QUOTED;
                    break;

                case 'n':
                    current_value += '\n';
                    context = CONTEXT__VALUE_QUOTED;
                    break;

                case 'r':
                    current_value += '\r';
                    context = CONTEXT__VALUE_QUOTED;
                    break;

                case 't':
                    current_value += '\t';
                    context = CONTEXT__VALUE_QUOTED;
                    break;

                case 'v':
                    current_value += '\v';
                    context = CONTEXT__VALUE_QUOTED;
                    break;

                case '"':
                case '\\':
                    current_value += input;
                    context = CONTEXT__VALUE_QUOTED;
                    break;

                case 'x':
                    context = CONTEXT__VALUE_HEX1;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__VALUE_HEX1:
                switch (char_class)
                {
                case CHAR_CLASS__HEXDIGIT:
                    current_value += char_to_hex(input) << 4;
                    context = CONTEXT__VALUE_HEX2;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__VALUE_HEX2:
                switch (char_class)
                {
                case CHAR_CLASS__HEXDIGIT:
                    current_value[current_value.size() - 1] |= char_to_hex(input);
                    context = last_context;
                    break;

                default:
                    fail = true;
                }
                break;

            case CONTEXT__VALUE_END:
                switch (char_class)
                {
                case CHAR_CLASS__NEWLINE:
                    set_values(current_section, current_key, current_values);
                    context = CONTEXT__NEWLINE;
                    break;

                case CHAR_CLASS__SPACE:
                    break;

                case CHAR_CLASS__SEMICOLON:
                    set_values(current_section, current_key, current_values);
                    context = CONTEXT__COMMENT;
                    break;

                case CHAR_CLASS__COMMA:
                    context = CONTEXT__EQUAL;
                    break;

                default:
                    fail = true;
                }
                break;
            }

            if (fail)
            {
                if (callback)
                    callback->error(cur_char, cur_line, cur_pos);
                return false;
            }

            if ((input == '\r') || ((input == '\n') && (text[cur_pos + 1] != '\r')))
            {
                cur_char = 0;
                ++cur_line;
            }
        }

        switch (context)
        {
        case CONTEXT__NEWLINE:
        case CONTEXT__COMMENT:
        case CONTEXT__SECTION_CLOSE:
            return true;

        case CONTEXT__VALUE_START:
            current_values += trim(current_value);
            current_value.clear();
        // FALL THROUGH
        case CONTEXT__VALUE_END:
            set_values(current_section, current_key, current_values);
            return true;

        default:;
        }

        if (callback)
            callback->error(cur_char, cur_line, cur_pos);

        return false;
    }

    std::string generate() const
    {
        std::string result;

        Sections::const_iterator SM = m_content.end();
        for (Sections::const_iterator SI = m_content.begin(); SI != SM; ++SI)
        {
            result += std::string("[") + encodeSection(SI->first) + "]\n";
            Keys::const_iterator KM = SI->second.end();
            for (Keys::const_iterator KI = SI->second.begin(); KI != KM; ++KI)
                result += encodeKey(KI->first) + "=" + encodeValues(KI->second) + "\n";
            result += "\n";
        }

        return result;
    }

    void clear()
    {
        m_content.clear();
    }

    std::set<std::string> get_all_sections() const
    {
        std::set<std::string> result;

        Sections::const_iterator SM = m_content.end();
        for (Sections::const_iterator SI = m_content.begin(); SI != SM; ++SI)
            result.insert(SI->first);

        return result;
    }

    bool is_section_exist(const std::string &section) const
    {
        return m_content.find(section) != m_content.end();
    }

    bool remove_section(const std::string &section)
    {
        return m_content.erase(section);
    }

    std::set<std::string> get_all_keys(const std::string &section) const
    {
        std::set<std::string> result;

        Sections::const_iterator SI = m_content.find(section);
        if (SI != m_content.end())
        {
            Keys::const_iterator KM = SI->second.end();
            for (Keys::const_iterator KI = SI->second.begin(); KI != KM; ++KI)
                result.insert(KI->first);
        }

        return result;
    }

    bool is_key_exist(const std::string &section, const std::string &key)
    {
        Sections::const_iterator SI = m_content.find(section);
        if (SI != m_content.end())
            return SI->second.find(key) != SI->second.end();

        return false;
    }

    bool is_list(const std::string &section, const std::string &key)
    {
        Sections::const_iterator SI = m_content.find(section);
        if (SI != m_content.end())
        {
            Keys::const_iterator KI = SI->second.find(key);
            if (KI != SI->second.end())
                return KI->second.size() > 1;
        }

        return false;
    }

    std::pair<bool, std::string> get_string(const std::string &section, const std::string &key, const std::string &default_value) const
    {
        Storage::Values default_values;
        default_values.push_back(default_value);

        std::pair<bool, Storage::Values> result = get_values(section, key, default_values);

        return std::make_pair(result.first, static_cast<std::string>(result.second[0]));
    }

    std::pair<bool, Storage::Values> get_values(const std::string &section, const std::string &key, const Storage::Values &default_value) const
    {
        Sections::const_iterator SI = m_content.find(section);
        if (SI != m_content.end())
        {
            Keys::const_iterator KI = SI->second.find(key);
            if (KI != SI->second.end())
                return std::make_pair(true, KI->second);
        }

        return std::make_pair(false, default_value);
    }

    void set_string(const std::string &section, const std::string &key, const std::string &value)
    {
        Storage::Values values;
        values.push_back(value);

        set_values(section, key, values);
    }

    void set_values(const std::string &section, const std::string &key, const Storage::Values &value)
    {
        if (value.empty())
        {
            Storage::Values empty;
            empty.push_back(std::string());

            m_content[section][key] = empty;
        }
        else
            m_content[section][key] = value;
    }

    bool remove_key(const std::string &section, const std::string &key)
    {
        bool result = false;

        Sections::iterator SI = m_content.find(section);
        if (SI != m_content.end())
        {
            if (SI->second.erase(key))
            {
                result = true;
                if (SI->second.empty())
                    m_content.erase(SI);
            }
        }

        return result;
    }

private:
    static const char hex[];

    static std::string encodeSection(const std::string &section)
    {
        return encodeKey(section);
    }

    static std::string encodeKey(const std::string &key)
    {
        std::string result;

        size_t m = key.length();
        for (size_t i = 0; i != m; ++i)
        {
            const char &ch = key[i];
            if (((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || (ch == '_') || (ch == '-') || (ch == '.'))
                result += ch;
            else
                result += std::string("%") + hex[ch / 16] + hex[ch % 16];
        }

        return result;
    }

    static std::string encodeValues(const Storage::Values &values)
    {
        std::string result;

        size_t m = values.size();
        for (size_t i = 0; i != m; ++i)
        {
            if (i)
                result += ", ";
            result += encodeValue(values[i]);
        }

        return result;
    }

    static std::string encodeValue(const Storage::Value &value)
    {
        std::string result;

        size_t m = value.size();
        for (size_t i = 0; i != m; ++i)
        {
            const char &ch = value[i];
            switch (ch)
            {
            case '\0':
                result += "\\0";
                break;

            case '\a':
                result += "\\a";
                break;

            case '\b':
                result += "\\b";
                break;

            case '\f':
                result += "\\f";
                break;

            case '\n':
                result += "\\n";
                break;

            case '\r':
                result += "\\r";
                break;

            case '\t':
                result += "\\t";
                break;

            case '\v':
                result += "\\v";
                break;

            case '\\':
            case '"':
                result += "\\" + ch;
                break;

            default:
                if ((ch >= 0x20) && (ch < 0x7f))
                    result += ch;
                else
                    result += std::string("\\x") + hex[ch / 16] + hex[ch % 16];
            }

            if (((ch >= '0') && (ch <= '9')) || ((ch >= 'a') && (ch <= 'z')) || ((ch >= 'A') && (ch <= 'Z')) || (ch == '_') || (ch == '-') || (ch == '.'))
                result += ch;
            else
                result += std::string("%") + hex[ch / 16] + hex[ch % 16];
        }

        if (result.find_first_of(" ;=,\"") != result.npos)
            result = std::string("\"") + result + "\"";

        return result;
    }

    static CharClass get_char_class(char input)
    {
        switch (input)
        {
        case '\r':
        case '\n':
            return CHAR_CLASS__NEWLINE;

        case ' ':
        case '\t':
            return CHAR_CLASS__SPACE;

        case ';':
            return CHAR_CLASS__SEMICOLON;

        case '[':
            return CHAR_CLASS__OPENBRACKET;

        case ']':
            return CHAR_CLASS__CLOSEBRACKET;

        case '%':
            return CHAR_CLASS__PERCENT;

        case '=':
            return CHAR_CLASS__EQUAL;

        case '"':
            return CHAR_CLASS__QUOTE;

        case '\\':
            return CHAR_CLASS__BACKSLASH;

        case ',':
            return CHAR_CLASS__COMMA;

        case '-':
        case '_':
        case '.':
            return CHAR_CLASS__MINUS;

        default:
            if (((input >= '0') && (input <= '9')) || ((input >= 'A') && (input <= 'F')) || ((input >= 'a') && (input <= 'a')))
                return CHAR_CLASS__HEXDIGIT;
            else if (((input >= 'G') && (input <= 'Z')) || ((input >= 'g') && (input <= 'z')))
                return CHAR_CLASS__LETTERS;
            else if ((input >= 0x20) || (input < 0x7f))
                return CHAR_CLASS__VISIBLE;
            else
                return CHAR_CLASS__OTHER;
        }
    }

    static char char_to_hex(char input)
    {
        if ((input >= '0') && (input <= '9'))
            return input - '0';
        else if ((input >= 'A') && (input <= 'F'))
            return input - 'A' + 0x0a;
        else if ((input >= 'a') && (input <= 'f'))
            return input - 'a' + 0x0a;
        return '\0';
    }

    static std::string trim(const std::string &str)
    {
        std::string::size_type start = str.find_first_not_of(" \t");
        if (start == str.npos)
            return std::string();
        return str.substr(start, str.find_last_not_of(" \t") - start + 1);
    }

private:
    Sections m_content;
};

const char StorageImpl::hex[] = "0123456789ABCDEF";


Storage::Value::Value()
    : std::vector<char>()
{
}

Storage::Value::Value(const std::string &string)
    : std::vector<char>()
{
    size_t m = string.length();
    reserve(m);
    memcpy(&*begin(), string.c_str(), m);
//    for (size_t i = 0; i < m; ++i)
//        push_back(string[i]);
}

Storage::Value::~Value()
{
}

Storage::Value& Storage::Value::operator = (const std::string &string)
{
    clear();
    size_t m = string.length();
    reserve(m);
    memcpy(&*begin(), string.c_str(), m);
//    for (size_t i = 0; i < m; ++i)
//        push_back(string[i]);
    return *this;
}

Storage::Value& Storage::Value::operator += (const char &value)
{
    push_back(value);
    return *this;
}

Storage::Value::operator std::string()
{
    return std::string(&*begin(), size());
}


Storage::Values::Values()
    : std::vector<Storage::Value>()
{
}

Storage::Values::Values(const Storage::Value &value)
    : std::vector<Storage::Value>()
{
    push_back(value);
}

Storage::Values::~Values()
{
}

Storage::Values& Storage::Values::operator = (const Storage::Value &value)
{
    clear();
    push_back(value);
    return *this;
}

Storage::Values& Storage::Values::operator += (const Storage::Value &value)
{
    push_back(value);
    return *this;
}


Storage::Storage() :
    impl(new StorageImpl())
{
}

Storage::~Storage()
{
    delete impl;
}

bool                             Storage::parse           (const std::string &text, Callback *callback)                                               { return impl->parse           (text, callback); }
std::string                      Storage::generate        ()                                                                                     const { return impl->generate        (); }
void                             Storage::clear           ()                                                                                           {        impl->clear           (); }
std::set<std::string>            Storage::get_all_sections()                                                                                     const { return impl->get_all_sections(); }
bool                             Storage::is_section_exist(const std::string &section)                                                           const { return impl->is_section_exist(section); }
bool                             Storage::remove_section  (const std::string &section)                                                                 { return impl->remove_section  (section); }
std::set<std::string>            Storage::get_all_keys    (const std::string &section)                                                           const { return impl->get_all_keys    (section); }
bool                             Storage::is_key_exist    (const std::string &section, const std::string &key)                                         { return impl->is_key_exist    (section, key); }
bool                             Storage::is_list         (const std::string &section, const std::string &key)                                         { return impl->is_list         (section, key); }
std::pair<bool, std::string>     Storage::get_string      (const std::string &section, const std::string &key, const std::string &default_value) const { return impl->get_string      (section, key, default_value); }
std::pair<bool, Storage::Values> Storage::get_values      (const std::string &section, const std::string &key, const Values &default_values)     const { return impl->get_values      (section, key, default_values); }
void                             Storage::set_string      (const std::string &section, const std::string &key, const std::string &value)               {        impl->set_string      (section, key, value); }
void                             Storage::set_values      (const std::string &section, const std::string &key, const Values &values)                   {        impl->set_values      (section, key, values); }
bool                             Storage::remove_key      (const std::string &section, const std::string &key)                                         { return impl->remove_key      (section, key); }

}
