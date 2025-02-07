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

#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QMutex>
#include <QMutexLocker>
#include <QString>

#include <common/EnumMapper.h>
#include <common/InfoItemAndData.h>
#include <common/Typedef.h>

#include <filesystem>

enum class InputFormat
{
  Invalid = -1,
  AnnexBHEVC, // Raw HEVC annex B file
  AnnexBAVC,  // Raw AVC annex B file
  AnnexBVVC,  // Raw VVC annex B file
  Libav       // This is some sort of container file which we will read using libavformat
};

constexpr EnumMapper<InputFormat, 5>
    InputFormatMapper(std::make_pair(InputFormat::Invalid, "Invalid"sv),
                      std::make_pair(InputFormat::AnnexBHEVC, "AnnexBHEVC"sv),
                      std::make_pair(InputFormat::AnnexBAVC, "AnnexBAVC"sv),
                      std::make_pair(InputFormat::AnnexBVVC, "AnnexBVVC"sv),
                      std::make_pair(InputFormat::Libav, "Libav"sv));

/* The FileSource class provides functions for accessing files. Besides the reading of
 * certain blocks of the file, it also directly provides information on the file for the
 * fileInfoWidget. It also adds functions for guessing the format from the filename.
 */
class FileSource : public QObject
{
  Q_OBJECT

public:
  FileSource();

  virtual bool openFile(const std::filesystem::path &filePath);

  virtual std::vector<InfoItem> getFileInfoList() const;
  std::optional<int64_t>        getFileSize() const;
  std::string                   getAbsoluteFilePath() const;
  QFile                        *getQFile() { return &this->srcFile; }
  bool                          getAndResetFileChangedFlag();

  // Return true if the file could be opened and is ready for use.
  bool isOk() const { return this->isFileOpened; }

  virtual bool atEnd() const { return !this->isFileOpened ? true : this->srcFile.atEnd(); }
  QByteArray   readLine() { return !this->isFileOpened ? QByteArray() : this->srcFile.readLine(); }
  virtual bool seek(int64_t pos) { return !this->isFileOpened ? false : this->srcFile.seek(pos); }
  int64_t      pos() { return !this->isFileOpened ? 0 : this->srcFile.pos(); }

  // Get the file size in bytes

  // Read the given number of bytes starting at startPos into the QByteArray out
  // Resize the QByteArray if necessary. Return how many bytes were read.
  int64_t readBytes(QByteArray &targetBuffer, int64_t startPos, int64_t nrBytes);

  void updateFileWatchSetting();
  void clearFileCache();

private slots:
  void fileSystemWatcherFileChanged(const QString &) { fileChanged = true; }

protected:
  std::filesystem::path fullFilePath{};
  QFile                 srcFile;
  bool                  isFileOpened{};

private:
  QFileSystemWatcher fileWatcher{};
  bool               fileChanged{};

  QMutex readMutex;
};
