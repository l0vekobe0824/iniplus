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

#ifndef INIPLUS__INCLUDED
#define INIPLUS__INCLUDED


#include <set>
#include <string>
#include <vector>
#include <utility>


namespace iniplus {

class StorageImpl;

class Storage
{
public:
    typedef enum ParseWarning {
        PARSE_WARNING__BINARY_ZERO_IN_SECTION_NAME = 0,
        PARSE_WARNING__BINARY_ZERO_IN_KEY_NAME
    } ParseWarning;

    class Callback
    {
    protected:
        Callback()
        {}

    public:
        virtual ~Callback()
        {}

        virtual void error(size_t faulty_pos, size_t faulty_line, size_t faulty_char) = 0;
        virtual void warning(ParseWarning type, size_t faulty_pos, size_t faulty_line, size_t faulty_char) = 0;
    };

    class C_Callback : public Callback
    {
    public:
        typedef void (*ErrorFunction)(size_t faulty_pos, size_t faulty_line, size_t faulty_char);
        typedef void (*WarningFunction)(ParseWarning type, size_t faulty_pos, size_t faulty_line, size_t faulty_char);

    public:
        C_Callback(ErrorFunction error_function = 0, WarningFunction warning_function = 0)
            : Callback()
            , m_error(error_function)
            , m_warning(warning_function)
        {}

        virtual ~C_Callback()
        {}

        virtual void error(size_t faulty_pos, size_t faulty_line, size_t faulty_char)
        {
            if (m_error)
                m_error(faulty_pos, faulty_line, faulty_char);
        }

        virtual void warning(ParseWarning type, size_t faulty_pos, size_t faulty_line, size_t faulty_char)
        {
            if (m_warning)
                m_warning(type, faulty_pos, faulty_line, faulty_char);
        }

    private:
        ErrorFunction m_error;
        WarningFunction m_warning;
    };

    class Value : public std::vector<char>
    {
    public:
        Value();
        Value(const std::string &);
        virtual ~Value();

        Value& operator = (const std::string &);

        Value& operator += (const char &);

        operator std::string() const;

        bool contains_binary(void) const;
    };

    class Values : public std::vector<Value>
    {
    public:
        Values();
        Values(const Value &);
        virtual ~Values();

        Values& operator = (const Value &);

        Values& operator += (const Value &);

        bool contains_binary(void) const;
    };

    typedef std::set<std::string> Strings;

public:
    Storage();
    ~Storage();

    typedef struct ParseResult
    {
        bool success;
        size_t faulty_char;
        size_t faulty_line;
        size_t faulty_pos;
    } ParseResult;

    bool parse(const std::string &text, Callback *callback = 0);

    std::string generate() const;


    void clear();

    Strings get_all_sections() const;

    bool is_section_exist(const std::string &section) const;

    /// returns false is the section did not exist
    bool remove_section(const std::string &section);

    /// returns false is the section did not exist or new_section exists
    bool rename_section(const std::string &section, const std::string &new_section);

    Strings get_all_keys(const std::string &section) const;

    bool is_key_exist(const std::string &section, const std::string &key) const;

    bool is_list(const std::string &section, const std::string &key) const;

    bool contains_binary(const std::string &section, const std::string &key) const;

    /// returns pair of success flag and the value, success is false if the key did not exist and the default_value used as the returned value
    std::pair<bool, std::string> get_string(const std::string &section, const std::string &key, const std::string &default_string = std::string()) const;
    std::pair<bool, Values> get_values(const std::string &section, const std::string &key, const Values &default_values = Values()) const;

    void set_string(const std::string &section, const std::string &key, const std::string &string);
    void set_values(const std::string &section, const std::string &key, const Values &values);

    /// returns false is the section/key did not exist
    bool remove_key(const std::string &section, const std::string &key);

    /// returns false is the section/key did not exist or new_section/new_key exists
    bool rename_key(const std::string &section, const std::string &key, const std::string &new_section, const std::string &new_key);

private:
    StorageImpl *impl;
};

}

#endif // INIPLUS__INCLUDED
