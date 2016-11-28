////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2016 ArangoDB GmbH, Cologne, Germany
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
/// @author Simon Grätzer
////////////////////////////////////////////////////////////////////////////////

#include "OutgoingCache.h"
#include "IncomingCache.h"
#include "Utils.h"
#include "WorkerState.h"

#include "Basics/MutexLocker.h"
#include "Basics/StaticStrings.h"
#include "Cluster/ClusterComm.h"
#include "VocBase/LogicalCollection.h"

#include <velocypack/Iterator.h>
#include <velocypack/velocypack-aliases.h>

#include <algorithm>

using namespace arangodb;
using namespace arangodb::pregel;

template <typename M>
OutgoingCache<M>::OutgoingCache(WorkerState* state, MessageFormat<M>* format,
                                MessageCombiner<M>* combiner,
                                IncomingCache<M>* cache)
    : _state(state), _format(format), _combiner(combiner), _localCache(cache) {
  _baseUrl = Utils::baseUrl(_state->database());
}

template <typename M>
OutgoingCache<M>::~OutgoingCache() {
  clear();
}

template <typename M>
void OutgoingCache<M>::clear() {
  _map.clear();
  _containedMessages = 0;
}

template <typename M>
void OutgoingCache<M>::sendMessageTo(std::string const& toValue,
                                     M const& data) {
  std::size_t pos = toValue.find('/');
  std::string _key = toValue.substr(pos + 1, toValue.length() - pos - 1);
  std::string collectionName = toValue.substr(0, pos);
  LOG(INFO) << "Adding outgoing messages for " << collectionName << "/" << _key;

  auto collInfo = Utils::resolveCollection(_state->database(),
                                           collectionName,
                                           _state->collectionPlanIdMap());
  if (!collInfo) {
    THROW_ARANGO_EXCEPTION_MESSAGE(
        TRI_ERROR_BAD_PARAMETER,
        "Collection this messages is going to is unkown");
  }
  ShardID responsibleShard;
  Utils::resolveShard(collInfo.get(), StaticStrings::KeyString, _key, responsibleShard);
  LOG(INFO) << "Responsible shard: " << responsibleShard;

  std::vector<ShardID> const& localShards = _state->localVertexShardIDs();
  if (std::find(localShards.begin(), localShards.end(), responsibleShard) !=
      localShards.end()) {
    _localCache->setDirect(toValue, data);
    LOG(INFO) << "Worker: Got messages for myself " << toValue << " <- "
              << data;
    _sendMessages++;
  } else {
    // std::unordered_map<std::string, VPackBuilder*> vertexMap =;
    std::unordered_map<std::string, M>& vertexMap = _map[responsibleShard];
    auto it = vertexMap.find(toValue);
    if (it != vertexMap.end()) {  // more than one message
      vertexMap[toValue] = _combiner->combine(vertexMap[toValue], data);
    } else {  // first message for this vertex
      vertexMap[toValue] = data;
    }
    _containedMessages++;
    
    if (_containedMessages > 1000) {
      sendMessages();
    }
  }
}

template <typename M>
void OutgoingCache<M>::sendMessages() {
  LOG(INFO) << "Beginning to send messages to other machines";

  std::vector<ClusterCommRequest> requests;
  for (auto const& it : _map) {
    ShardID const& shard = it.first;
    std::unordered_map<std::string, M> const& vertexMessageMap = it.second;
    if (vertexMessageMap.size() == 0) {
      continue;
    }

    VPackBuilder package;
    package.openObject();
    package.add(Utils::messagesKey, VPackValue(VPackValueType::Array));
    for (auto const& vertexMessagePair : vertexMessageMap) {
      package.add(VPackValue(vertexMessagePair.first));
      _format->addValue(package, vertexMessagePair.second);
      _sendMessages++;
    }
    package.close();
    package.add(Utils::senderKey, VPackValue(ServerState::instance()->getId()));
    package.add(Utils::executionNumberKey,
                VPackValue(_state->executionNumber()));
    package.add(Utils::globalSuperstepKey,
                VPackValue(_state->globalSuperstep()));
    package.close();
    // add a request
    auto body = std::make_shared<std::string>(package.toJson());
    requests.emplace_back("shard:" + shard, rest::RequestType::POST,
                          _baseUrl + Utils::messagesPath, body);

    LOG(INFO) << "Worker: Sending data to other Shard: " << shard
              << ". Message: " << package.toJson();
  }
  size_t nrDone = 0;
  ClusterComm::instance()->performRequests(requests, 120, nrDone,
                                           LogTopic("Pregel message transfer"));
  // readResults(requests);
  for (auto const& req : requests) {
    auto& res = req.result;
    if (res.status == CL_COMM_RECEIVED) {
      LOG(INFO) << res.answer->payload().toJson();
    }
  }
  this->clear();
}

// template types to create
template class arangodb::pregel::OutgoingCache<int64_t>;
template class arangodb::pregel::OutgoingCache<float>;