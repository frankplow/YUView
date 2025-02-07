/*  This file is part of YUView - The YUV player with advanced analytics toolset
 *   <https://github.com/IENT/YUView>
 *   Copyright (C) 2015  Institut für Nachrichtentechnik, RWTH Aachen University, GERMANY
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   In addition, as a special exception, the copyright holders give
 *   permission to link the code of portions of this program with the
 *   OpenSSL library under certain conditions as described in each
 *   individual source file, and distribute linked combinations including
 *   the two.
 *
 *   You must obey the GNU General Public License in all respects for all
 *   of the code used other than OpenSSL. If you modify file(s) with this
 *   exception, you may extend this exception to your version of the
 *   file(s), but you are not obligated to do so. If you do not wish to do
 *   so, delete this exception statement from your version. If you delete
 *   this exception statement from all source files in the program, then
 *   also delete it here.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QList>
#include <QMetaType>
#include <QString>

/*
 * An info item has a name, a text and an optional description. These are used to show them in the
 * fileInfoWidget. For example: ["File Name", "file.yuv"] or ["Number Frames", "123"].
 */
struct InfoItem
{
  std::string name{};
  std::string text{};
  std::string description{};

  InfoItem(std::string &&name, std::string &&text) : name(std::move(name)), text(std::move(text)) {}
  InfoItem(std::string &&name, std::string &&text, std::string &&description)
      : name(std::move(name)), text(std::move(text)), description(std::move(description))
  {
  }
  InfoItem(std::string_view name, std::string_view text) : name(name), text(text) {}
  InfoItem(std::string_view name, std::string_view text, std::string_view description)
      : name(name), text(text), description(description)
  {
  }

  bool operator==(const InfoItem &other) const
  {
    return this->name == other.name && this->text == other.text &&
           this->description == other.description;
  }
};

struct InfoData
{
  explicit InfoData(const QString &title = QString()) : title(title) {}
  bool            isEmpty() const { return title.isEmpty() && items.isEmpty(); }
  QString         title{};
  QList<InfoItem> items{};
};
Q_DECLARE_METATYPE(InfoData)
