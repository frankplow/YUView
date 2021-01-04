/*  This file is part of YUView - The YUV player with advanced analytics toolset
 *   <https://github.com/IENT/YUView>
 *   Copyright (C) 2015  Institut f�r Nachrichtentechnik, RWTH Aachen University, GERMANY
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

#include "ReaderHelperNew.h"

#include <iomanip>
#include <sstream>

namespace parser::reader
{

namespace
{

template <typename T> std::string formatCoding(const std::string formatName, T value)
{
  std::ostringstream stringStream;
  stringStream << formatName;
  if (formatName != "u(1)")
    stringStream << " -> u(" << value << ")";
  return stringStream.str();
}

void checkAndLog(TreeItem *         item,
                 const std::string &formatName,
                 const std::string &symbolName,
                 const Options &    options,
                 int64_t            value,
                 const std::string &code)
{
  RangeCheckResult checkResult;
  for (auto &check : options.checkList)
  {
    checkResult = check->checkValue(value);
    if (!checkResult)
      break;
  }

  if (item)
  {
    std::string meaning = options.meaningString;
    if (options.meaningMap.count(int64_t(value)) > 0)
      meaning = options.meaningMap.at(value);

    if (!checkResult)
      meaning += " " + checkResult.errorMessage;
    auto newItem = new TreeItem(item,
                                symbolName,
                                std::to_string(value),
                                formatCoding(formatName, code.size()),
                                code,
                                meaning);
    if (!checkResult)
      newItem->setError();
  }
  if (!checkResult)
    throw std::logic_error(checkResult.errorMessage);
}

void checkAndLog(TreeItem *         item,
                 const std::string &formatName,
                 const std::string &symbolName,
                 const Options &    options,
                 ByteVector         value,
                 const std::string &code)
{
  // There are no range checks for ByteVectors. Also the meaningMap does nothing.
  if (item)
  {
    if (code.size() != value.size() * 8)
      throw std::logic_error("Nr bytes and size of code does not match.");

    auto byteVectorItem = new TreeItem(item, symbolName);
    for (size_t i = 0; i < value.size(); i++)
    {
      auto              c = value.at(i);
      std::stringstream valueStream;
      valueStream << "0x" << std::setfill('0') << std::setw(2) << std::hex << c << " (" << c << ")";
      auto byteCode = code.substr(i * 8, 8);
      new TreeItem(byteVectorItem,
                   "Byte " + std::to_string(i),
                   valueStream.str(),
                   formatCoding(formatName, code.size()),
                   byteCode,
                   options.meaningString);
    }
  }
}

} // namespace

ByteVector ReaderHelperNew::convertBeginningToByteVector(QByteArray data)
{
  ByteVector ret;
  const auto maxLength = 2000u;
  const auto length    = std::min(unsigned(data.size()), maxLength);
  for (auto i = 0u; i < length; i++)
  {
    ret.push_back(data.at(i));
  }
  return ret;
}

ReaderHelperNew::ReaderHelperNew(SubByteReaderNew &reader,
                                 TreeItem *        item,
                                 std::string       new_sub_item_name)
{
  this->reader = reader;
  if (item)
  {
    if (new_sub_item_name.empty())
      this->currentTreeLevel = item;
    else
      this->currentTreeLevel = new TreeItem(item, new_sub_item_name);
  }
  this->itemHierarchy.push(this->currentTreeLevel);
}

ReaderHelperNew::ReaderHelperNew(const ByteVector &inArr,
                                 TreeItem *        item,
                                 std::string       new_sub_item_name,
                                 size_t            inOffset)
{
  this->reader = SubByteReaderNew(inArr, inOffset);
  if (item)
  {
    if (new_sub_item_name.empty())
      this->currentTreeLevel = item;
    else
      this->currentTreeLevel = new TreeItem(item, new_sub_item_name);
  }
  this->itemHierarchy.push(this->currentTreeLevel);
}

void ReaderHelperNew::addLogSubLevel(const std::string name)
{
  assert(!name.empty());
  if (itemHierarchy.top() == nullptr)
    return;
  this->currentTreeLevel = new TreeItem(this->itemHierarchy.top(), name);
  this->itemHierarchy.push(this->currentTreeLevel);
}

void ReaderHelperNew::removeLogSubLevel()
{
  if (itemHierarchy.size() <= 1)
    // Don't remove the root
    return;
  this->itemHierarchy.pop();
  this->currentTreeLevel = this->itemHierarchy.top();
}

uint64_t
ReaderHelperNew::readBits(const std::string &symbolName, int numBits, const Options &options)
{
  try
  {
    auto [value, code] = this->reader.readBits(numBits);
    checkAndLog(this->currentTreeLevel, "u(v)", symbolName, options, value, code);
    return value;
  }
  catch (const std::exception &ex)
  {
    this->logExceptionAndThrowError(ex, std::to_string(numBits) + " bit symbol " + symbolName);
  }
}

bool ReaderHelperNew::readFlag(const std::string &symbolName, const Options &options)
{
  try
  {
    auto [value, code] = this->reader.readBits(1);
    checkAndLog(this->currentTreeLevel, "u(1)", symbolName, options, value, code);
    return (value != 0);
  }
  catch (const std::exception &ex)
  {
    this->logExceptionAndThrowError(ex, "flag " + symbolName);
  }
}

uint64_t ReaderHelperNew::readUEV(const std::string &symbolName, const Options &options)
{
  try
  {
    auto [value, code] = this->reader.readUE_V();
    checkAndLog(this->currentTreeLevel, "ue(v)", symbolName, options, value, code);
    return value;
  }
  catch (const std::exception &ex)
  {
    this->logExceptionAndThrowError(ex, "UEV symbol " + symbolName);
  }
}

int64_t ReaderHelperNew::readSEV(const std::string &symbolName, const Options &options)
{
  try
  {
    auto [value, code] = this->reader.readSE_V();
    checkAndLog(this->currentTreeLevel, "se(v)", symbolName, options, value, code);
    return value;
  }
  catch (const std::exception &ex)
  {
    this->logExceptionAndThrowError(ex, "SEV symbol " + symbolName);
  }
}

ByteVector
ReaderHelperNew::readBytes(const std::string &symbolName, size_t nrBytes, const Options &options)
{
  try
  {
    if (!this->byte_aligned())
      throw std::logic_error("Trying to ready bytes while not byte aligned.");

    auto [value, code] = this->reader.readBytes(nrBytes);
    checkAndLog(this->currentTreeLevel, "se(v)", symbolName, options, value, code);
    return value;
  }
  catch (const std::exception &ex)
  {
    this->logExceptionAndThrowError(ex, " " + std::to_string(nrBytes) + " bytes.");
  }
}

void ReaderHelperNew::logExceptionAndThrowError(const std::exception &ex, const std::string &when)
{
  if (this->currentTreeLevel)
  {
    auto errorMessage = "Reading error " + std::string(ex.what());
    auto item         = new TreeItem(currentTreeLevel, "Error", "", "", "", errorMessage);
    item->setError();
  }
  throw std::logic_error("Error reading " + when);
}

} // namespace parser::reader