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
public:
    StorageImpl();

    ~StorageImpl();

    Storage::ParseResult parse(const std::string &text)
    {
        Storage::ParseResult result;

        return result;
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
                result += encodeKey(KI->first) + "=" + encodeValue(KI->second) + "\n";
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

    std::pair<bool, std::string> get_value(const std::string &section, const std::string &key, const std::string &default_value) const
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

    void set_value(const std::string &section, const std::string &key, const std::string &value)
    {
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

    static std::string encodeValue(const std::string &value)
    {
        std::string result;

        size_t m = value.length();
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

        if (result.find_first_of(" ;=,\""))
            result = std::string("\"") + result + "\"";

        return result;
    }

private:
    typedef std::map<std::string, std::string> Keys;
    typedef std::map<std::string, Keys> Sections;

    Sections m_content;
};

const char StorageImpl::hex[] = "0123456789ABCDEF";


Storage::Storage() :
    impl(new StorageImpl())
{
}

Storage::~Storage()
{
    delete impl;
}


Storage::ParseResult         Storage::parse           (const std::string &text)                                                                    { return impl->parse           (text); }
std::string                  Storage::generate        ()                                                                                     const { return impl->generate        (); }
void                         Storage::clear           ()                                                                                           {        impl->clear           (); }
std::set<std::string>        Storage::get_all_sections()                                                                                     const { return impl->get_all_sections(); }
bool                         Storage::is_section_exist(const std::string &section)                                                           const { return impl->is_section_exist(section); }
bool                         Storage::remove_section  (const std::string &section)                                                                 { return impl->remove_section  (section); }
std::set<std::string>        Storage::get_all_keys    (const std::string &section)                                                           const { return impl->get_all_keys    (section); }
bool                         Storage::is_key_exist    (const std::string &section, const std::string &key)                                         { return impl->is_key_exist    (section, key); }
std::pair<bool, std::string> Storage::get_value       (const std::string &section, const std::string &key, const std::string &default_value) const { return impl->get_value       (section, key, default_value); }
void                         Storage::set_value       (const std::string &section, const std::string &key, const std::string &value)               {        impl->set_value       (section, key, value); }
bool                         Storage::remove_key      (const std::string &section, const std::string &key)                                         { return impl->remove_key      (section, key); }

}
