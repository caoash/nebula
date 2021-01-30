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

#include <gperftools/profiler.h>
#include <grpcpp/grpcpp.h>

#include "common/Folly.h"
#include "execution/meta/TableService.h"
#include "node/node.grpc.fb.h"
#include "node/node_generated.h"
#include "service/base/NebulaService.h"

/**
 * Define node server that does the work as nebula server asks.
 */
namespace nebula {
namespace service {
namespace node {

class NodeServerImpl final : public NodeServer::Service {
  virtual grpc::Status Echo(
    grpc::ServerContext*,
    const flatbuffers::grpc::Message<EchoPing>*,
    flatbuffers::grpc::Message<EchoReply>*) override;

  virtual grpc::Status Echos(
    grpc::ServerContext*,
    const flatbuffers::grpc::Message<ManyEchoPings>*,
    grpc::ServerWriter<flatbuffers::grpc::Message<EchoReply>>*)
    override;

  virtual grpc::Status Query(
    grpc::ServerContext*,
    const flatbuffers::grpc::Message<QueryPlan>*,
    flatbuffers::grpc::Message<BatchRows>*)
    override;

  virtual grpc::Status Poll(
    grpc::ServerContext*,
    const flatbuffers::grpc::Message<NodeStateRequest>*,
    flatbuffers::grpc::Message<NodeStateReply>*)
    override;

  virtual grpc::Status Task(
    grpc::ServerContext*,
    const flatbuffers::grpc::Message<TaskSpec>*,
    flatbuffers::grpc::Message<TaskReply>*)
    override;

public:
  NodeServerImpl()
    : tableService_{ nebula::execution::meta::TableService::singleton() },
      threadPool_{ std::thread::hardware_concurrency(), 2 } {}
  virtual ~NodeServerImpl() = default;

  folly::ThreadPoolExecutor& pool() {
    return threadPool_;
  }

private:
  std::shared_ptr<nebula::execution::meta::TableService> tableService_;

  // by default if not specified, CPUThreadPoolExecutor will use UnboundedBlockingQueue
  // so we can add as many task as we want.
  // Initialize this pool with two priority queues:
  //    higher for query execution.
  //    lower for task execution.
  folly::CPUThreadPoolExecutor threadPool_;
};

} // namespace node
} // namespace service
} // namespace nebula