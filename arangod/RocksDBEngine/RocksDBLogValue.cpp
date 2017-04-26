////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2017 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Jan Steemann
/// @author Daniel H. Larkin
////////////////////////////////////////////////////////////////////////////////

#include "RocksDBEngine/RocksDBLogValue.h"
#include "Basics/Common.h"
#include "Basics/Exceptions.h"
#include "RocksDBEngine/RocksDBCommon.h"

using namespace arangodb;
using namespace arangodb::rocksutils;

RocksDBLogValue RocksDBLogValue::BeginTransaction(TRI_voc_tick_t dbid, TRI_voc_tid_t tid) {
  return RocksDBLogValue(RocksDBLogType::BeginTransaction, dbid, tid);
}

RocksDBLogValue RocksDBLogValue::DatabaseCreate() {
  return RocksDBLogValue(RocksDBLogType::DatabaseCreate);
}

RocksDBLogValue RocksDBLogValue::DatabaseDrop(TRI_voc_tick_t dbid) {
  return RocksDBLogValue(RocksDBLogType::DatabaseDrop, dbid);
}

RocksDBLogValue RocksDBLogValue::CollectionCreate(TRI_voc_cid_t cid) {
  return RocksDBLogValue(RocksDBLogType::CollectionCreate, cid);
}

RocksDBLogValue RocksDBLogValue::CollectionDrop(TRI_voc_tick_t dbid, TRI_voc_cid_t cid) {
  return RocksDBLogValue(RocksDBLogType::CollectionDrop, dbid, cid);
}

RocksDBLogValue RocksDBLogValue::CollectionRename(TRI_voc_tick_t dbid, TRI_voc_cid_t cid, std::string const& newName) {
  return RocksDBLogValue(RocksDBLogType::CollectionRename, dbid, cid, newName);
}

RocksDBLogValue RocksDBLogValue::CollectionChange(TRI_voc_tick_t dbid, TRI_voc_cid_t cid) {
  return RocksDBLogValue(RocksDBLogType::CollectionChange, dbid, cid);
}

RocksDBLogValue RocksDBLogValue::IndexCreate(TRI_voc_tick_t dbid, TRI_voc_cid_t cid, TRI_idx_iid_t iid) {
  return RocksDBLogValue(RocksDBLogType::IndexCreate, dbid, cid, iid);
}

RocksDBLogValue RocksDBLogValue::IndexDrop(TRI_voc_tick_t dbid, TRI_voc_cid_t cid, TRI_idx_iid_t iid) {
  return RocksDBLogValue(RocksDBLogType::IndexDrop, dbid, cid, iid);

}

RocksDBLogValue RocksDBLogValue::ViewCreate(TRI_voc_cid_t cid, TRI_idx_iid_t iid) {
  return RocksDBLogValue(RocksDBLogType::ViewCreate, cid, iid);
}

RocksDBLogValue RocksDBLogValue::ViewDrop(TRI_voc_cid_t cid, TRI_idx_iid_t iid) {
  return RocksDBLogValue(RocksDBLogType::ViewDrop, cid, iid);
}

RocksDBLogValue RocksDBLogValue::DocumentOpsPrologue(TRI_voc_cid_t cid) {
  return RocksDBLogValue(RocksDBLogType::DocumentOperationsPrologue, cid);
}

RocksDBLogValue RocksDBLogValue::DocumentRemove(arangodb::StringRef const& key) {
  return RocksDBLogValue(RocksDBLogType::DocumentRemove, key);
}

RocksDBLogValue::RocksDBLogValue(RocksDBLogType type) : _buffer() {
  switch (type) {
    case RocksDBLogType::DatabaseCreate:
      _buffer.reserve(sizeof(RocksDBLogType));
      _buffer += static_cast<char>(type);
      break;
    default:
      THROW_ARANGO_EXCEPTION(TRI_ERROR_BAD_PARAMETER);
  }
}

RocksDBLogValue::RocksDBLogValue(RocksDBLogType type, uint64_t val) : _buffer() {
  switch (type) {
    case RocksDBLogType::DatabaseDrop:
    case RocksDBLogType::CollectionCreate: {
      _buffer.reserve(sizeof(RocksDBLogType) + sizeof(uint64_t));
      _buffer += static_cast<char>(type);
      uint64ToPersistent(_buffer, val);// database or collection ID
      break;
    }
      
    default:
      THROW_ARANGO_EXCEPTION(TRI_ERROR_BAD_PARAMETER);
  }
}


RocksDBLogValue::RocksDBLogValue(RocksDBLogType type, uint64_t dbId, uint64_t cid)
    : _buffer() {
  switch (type) {
      
    case RocksDBLogType::BeginTransaction:
    case RocksDBLogType::CollectionChange: 
    case RocksDBLogType::CollectionDrop: {
      _buffer.reserve(sizeof(RocksDBLogType) + sizeof(uint64_t) * 2);
      _buffer += static_cast<char>(type);
      uint64ToPersistent(_buffer, dbId);
      uint64ToPersistent(_buffer, cid);
      break;
    }

    default:
      THROW_ARANGO_EXCEPTION(TRI_ERROR_BAD_PARAMETER);
  }
}

RocksDBLogValue::RocksDBLogValue(RocksDBLogType type, uint64_t dbId, uint64_t cid, uint64_t iid)
    : _buffer() {
  switch (type) {
    case RocksDBLogType::IndexCreate:
    case RocksDBLogType::IndexDrop: {
      _buffer.reserve(sizeof(RocksDBLogType) + sizeof(uint64_t) * 3);
      _buffer += static_cast<char>(type);
      uint64ToPersistent(_buffer, dbId);
      uint64ToPersistent(_buffer, cid);
      uint64ToPersistent(_buffer, iid);
      break;
    }
    default:
      THROW_ARANGO_EXCEPTION(TRI_ERROR_BAD_PARAMETER);
  }
}

RocksDBLogValue::RocksDBLogValue(RocksDBLogType type, uint64_t dbId, uint64_t cid, std::string const& data)
  : _buffer() {
  if (type != RocksDBLogType::CollectionRename) {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_BAD_PARAMETER);
  }
  _buffer.reserve(sizeof(RocksDBLogType) + sizeof(uint64_t) * 2 + data.length());
  _buffer += static_cast<char>(type);
  uint64ToPersistent(_buffer, dbId);
  uint64ToPersistent(_buffer, cid);
  _buffer.append(data.data(), data.length());  // primary key
}

RocksDBLogValue::RocksDBLogValue(RocksDBLogType type, StringRef const& data)
    :  _buffer() {
  switch (type) {
    case RocksDBLogType::DocumentRemove: {
      _buffer.reserve(data.length() + sizeof(RocksDBLogType));
      _buffer += static_cast<char>(type);
      _buffer.append(data.data(), data.length());  // primary key
      break;
    }

    default:
      THROW_ARANGO_EXCEPTION(TRI_ERROR_BAD_PARAMETER);
  }
}

RocksDBLogType RocksDBLogValue::type(rocksdb::Slice const& slice) {
  TRI_ASSERT(slice.size() >= sizeof(RocksDBLogType) + sizeof(uint64_t));
  return static_cast<RocksDBLogType>(slice.data()[0]);
}

TRI_voc_tick_t RocksDBLogValue::databaseId(rocksdb::Slice const& slice) {
  TRI_ASSERT(slice.size() >= sizeof(RocksDBLogType) + sizeof(uint64_t));
  RocksDBLogType type = static_cast<RocksDBLogType>(slice.data()[0]);
  TRI_ASSERT(type == RocksDBLogType::BeginTransaction ||
             type == RocksDBLogType::DatabaseCreate ||
             type == RocksDBLogType::DatabaseDrop);
  return uint64FromPersistent(slice.data() + sizeof(RocksDBLogType));
}

TRI_voc_tid_t RocksDBLogValue::transactionId(rocksdb::Slice const& slice) {
  TRI_ASSERT(slice.size() >= sizeof(RocksDBLogType) + sizeof(uint64_t));
  RocksDBLogType type = static_cast<RocksDBLogType>(slice.data()[0]);
  if (type == RocksDBLogType::BeginTransaction) {
    return uint64FromPersistent(slice.data() + sizeof(TRI_voc_tick_t)
                                + sizeof(RocksDBLogType));
  } else {
    THROW_ARANGO_EXCEPTION(TRI_ERROR_BAD_PARAMETER);
  }
}

TRI_voc_cid_t RocksDBLogValue::collectionId(rocksdb::Slice const& slice) {
  TRI_ASSERT(slice.size() >= sizeof(RocksDBLogType) + sizeof(uint64_t));
  RocksDBLogType type = static_cast<RocksDBLogType>(slice.data()[0]);
  TRI_ASSERT(type == RocksDBLogType::CollectionCreate ||
             type == RocksDBLogType::CollectionDrop ||
             type == RocksDBLogType::CollectionRename ||
             type == RocksDBLogType::CollectionChange);
  return uint64FromPersistent(slice.data() + sizeof(RocksDBLogType));
}

TRI_idx_iid_t RocksDBLogValue::indexId(rocksdb::Slice const& slice) {
  TRI_ASSERT(slice.size() >= sizeof(RocksDBLogType) + sizeof(uint64_t));
  RocksDBLogType type = static_cast<RocksDBLogType>(slice.data()[0]);
  TRI_ASSERT(type == RocksDBLogType::IndexCreate ||
             type == RocksDBLogType::IndexDrop);
  return uint64FromPersistent(slice.data() + sizeof(RocksDBLogType));
}
