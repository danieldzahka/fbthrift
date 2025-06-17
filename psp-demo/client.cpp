/*
* Copyright (c) Meta Platforms, Inc. and affiliates.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include <glog/logging.h>
#include <folly/init/Init.h>
#include <folly/portability/GFlags.h>
#include <folly/io/async/AsyncSSLSocket.h>
#include <folly/io/async/AsyncSocket.h>
#include <psp-demo/gen-cpp2/EchoService.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/security/AsyncPSP.h>
#include <chrono>

#include <psp_ynl.h>

DEFINE_string(addr, "",
	      "for client, address to connect; for server, address to listen");

using example::psp::EchoService;
using apache::thrift::RocketClientChannel;
using apache::thrift::PspConnector;
using ::example::psp::EchoRequest;
using ::example::psp::EchoResponse;

namespace
{
constexpr int iterations = 10000;
}

int main(int argc, char **argv)
{
	FLAGS_logtostderr = 1;
	folly::init(&argc, &argv);

	folly::EventBase *evb = folly::EventBaseManager::get()->getEventBase();
	folly::SocketAddress addr;
	addr.setFromHostPort(FLAGS_addr);

	auto pynl = psp_ynl_create();
	PspConnector connector{ pynl.get() };
	auto transport = connector.connect(addr, evb);

	RocketClientChannel::Ptr channel =
		RocketClientChannel::newChannel(std::move(transport));
	auto client = std::make_unique<apache::thrift::Client<EchoService> >(
		std::move(channel));

	if (!client)
		LOG(FATAL) << "client create failure";

	LOG(INFO) << "attempting " << iterations << " echos";

	std::vector<folly::SemiFuture<EchoResponse> > futures;

	auto start = std::chrono::system_clock::now();
	for (int i = 0; i < iterations; ++i) {
		EchoRequest req;
		*req.num() = i;
		futures.push_back(client->semifuture_echo(req));
	}

	auto collected = folly::collect(futures).via(evb).getVia(evb);

	auto end = std::chrono::system_clock::now();
	LOG(INFO) << iterations << " echos in: "
		  << std::chrono::duration_cast<std::chrono::milliseconds>(
			     end - start)
			     .count()
		  << "ms";

	int expected = 0;
	for (auto const &rsp : collected) {
		if (*rsp.num() != expected)
			LOG(FATAL) << "echo result invalid";
		++expected;
	}

	LOG(INFO) << "Echo results ok.";

	return 0;
}
