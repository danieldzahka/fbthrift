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
#include <psp-demo/gen-cpp2/EchoService.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/ThriftProcessor.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <wangle/acceptor/ServerSocketConfig.h>

#include <psp_ynl.h>

DEFINE_int32(echo_port, 7778, "Echo Server port");

DEFINE_string(addr, "",
	      "for client, address to connect; for server, address to listen");

using apache::thrift::ThriftServer;

namespace example::psp
{
class EchoHandler : virtual public apache::thrift::ServiceHandler<EchoService> {
    public:
	void
	sync_echo(::example::psp::EchoResponse &rsp,
		  std::unique_ptr< ::example::psp::EchoRequest> req) override
	{
		rsp.num() = *req->num();
	}
};
} // namespace example::psp

int main(int argc, char **argv)
{
	FLAGS_logtostderr = 1;
	folly::init(&argc, &argv);

	auto handler = std::make_shared<example::psp::EchoHandler>();
	auto server = std::make_shared<ThriftServer>();

	folly::SocketAddress addr;
	addr.setFromHostPort(FLAGS_addr);
	server->setAddress(addr);
	server->setInterface(handler);

	server->setSSLPolicy(apache::thrift::SSLPolicy::REQUIRED);
	auto sslConfig = std::make_shared<wangle::SSLContextConfig>();
	sslConfig->setNextProtocols({ "rs" });
	sslConfig->setCertificate("/var/facebook/x509_identities/server.pem",
				  "/var/facebook/x509_identities/server.pem",
				  "");
	sslConfig->clientVerification =
		folly::SSLContext::VerifyClientCertificate::DO_NOT_REQUEST;
	server->setSSLConfig(std::move(sslConfig));
	apache::thrift::ThriftTlsConfig thriftConfig;
	bool const have_psp = psp_ynl_create() != nullptr;
	thriftConfig.enableThriftParamsNegotiation = true;
	thriftConfig.enablePSP = have_psp;
	server->setThriftConfig(thriftConfig);
	server->setAcceptorFactory(
		std::make_shared<apache::thrift::DefaultThriftAcceptorFactory>(
			server.get()));

	LOG(INFO) << "Echo Server running on port: " << FLAGS_echo_port;
	server->serve();

	return 0;
}
