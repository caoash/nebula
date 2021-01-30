/*
 * Copyright 2017-present varchar.io
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <vector>

#include "common/Errors.h"

/**
 * Define a file system interface to provide common data access.
 */
namespace nebula {
namespace storage {

// layout
struct FileInfo {
  explicit FileInfo(bool isd, size_t ts, size_t sz, std::string n, std::string d)
    : isDir{ isd }, timestamp{ ts }, size{ sz }, name{ std::move(n) }, domain{ std::move(d) } {}

  bool isDir;
  size_t timestamp;
  size_t size;
  std::string name;
  std::string domain;

  inline std::string signature() const {
    return fmt::format("{0}_{1}_{2}", name, size, timestamp);
  }
};

class NFileSystem {
public:
  NFileSystem() = default;
  virtual ~NFileSystem() = default;

  // list a folder or a path to get all file info
  virtual std::vector<FileInfo> list(const std::string&) = 0;

  // read a file/object at given offset and length into buffer address provided
  virtual size_t read(const std::string&, const size_t, const size_t, char*) = 0;

  // read a file/object fully into a memory buffer with max size to fill
  virtual size_t read(const std::string&, char*, size_t) = 0;

  // return file info of given file handler
  virtual FileInfo info(const std::string&) = 0;

  // copy a file to an tmp file - most likely used for remote file download scenario
  // two path - from, to
  virtual bool copy(const std::string&, const std::string&) = 0;

  // make temp file or temp folder (only supported by local file system)
  // void for key-based cloud fs (s3, google storage)
  virtual std::string temp(bool = false) = 0;

  // sync data from folder to folder - depends on FS implementation
  virtual bool sync(const std::string&, const std::string&, bool = false) = 0;

  // rm all content of given path
  virtual void rm(const std::string&) = 0;
};
} // namespace storage
} // namespace nebula