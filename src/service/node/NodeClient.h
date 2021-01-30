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

#include <iostream>
#include <memory>
#include <string>

#include "ConnectionPool.h"
#include "common/Folly.h"
#include "execution/ExecutionPlan.h"
#include "execution/core/NodeClient.h"
#include "meta/NNode.h"
#include "node/node.grpc.fb.h"
#include "node/node_generated.h"
#include "service/base/NebulaService.h"

/**
 * Define node client which is responsible to talk to a node server for query fan out.
 */
namespace nebula {
namespace service {
namespace node {
class NodeClient : public nebula::execution::core::NodeClient {
public:
  NodeClient(
    const nebula::meta::NNode& node,
    folly::ThreadPoolExecutor& pool,
    std::shared_ptr<nebula::api::dsl::Query> query)
    : nebula::execution::core::NodeClient(node, pool), query_{ query } {

    // create a reusable stub
    auto channel = ConnectionPool::init()->connection(node.toString());
    N_ENSURE(channel != nullptr, "requires a valid channel");
    stub_ = nebula::service::NodeServer::NewStub(channel);
  }

  virtual ~NodeClient() = default;

public:
  // echo a name back from node server
  void echo(const std::string&);

  // stream multiple responses based on count
  void echos(const std::string&, size_t);

  // execute a plan on remote node
  virtual folly::Future<nebula::surface::RowCursorPtr> execute(const nebula::execution::PlanPtr plan) override;

  // pull node state
  virtual void update() override;

  // send a task to a node
  virtual nebula::common::TaskState task(const nebula::common::Task&) override;

private:
  std::shared_ptr<nebula::api::dsl::Query> query_;
  std::unique_ptr<NodeServer::Stub> stub_;
};

} // namespace node
} // namespace service
} // namespace nebula