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

#ifndef INIXX__STORAGE__INCLUDED
#define INIXX__STORAGE__INCLUDED


#include <set>
#include <string>
#include <vector>
#include <utility>


namespace ini {

class StorageImpl;

class Storage
{
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

    ParseResult parse(const std::string &text);

    std::string generate() const;


    void clear();

    std::set<std::string> get_all_sections() const;

    bool is_section_exist(const std::string &section) const;

    /// returns false is the section did not exist
    bool remove_section(const std::string &section);

    std::set<std::string> get_all_keys(const std::string &section) const;

    bool is_key_exist(const std::string &section, const std::string &key);

    /// returns pair of success flag and the value, success is false if the key did not exist and the default_value used as the returned value
    std::pair<bool, std::string> get_value(const std::string &section, const std::string &key, const std::string &default_value = std::string()) const;

    void set_value(const std::string &section, const std::string &key, const std::string &value);

    /// returns false is the key did not exist
    bool remove_key(const std::string &section, const std::string &key);

private:
    StorageImpl *impl;
};

}

#endif // INIXX__STORAGE__INCLUDED
